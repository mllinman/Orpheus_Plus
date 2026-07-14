#include <JuceHeader.h>
#include "PluginManager.h"
#include "AudioEngine.h"
#include "../UI/PluginWindow.h"
#include "../Util/OrpheusLogger.h"

PluginManager::PluginManager(AudioEngine& e) : engine(e)
{
    formatManager.addFormat(new juce::VST3PluginFormat());
   #if JUCE_MAC
    formatManager.addFormat(new juce::AudioUnitPluginFormat());
   #endif

    auto pluginListFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                            .getChildFile("Orpheus Plus/plugins.xml");
    loadPluginList(pluginListFile);
    loadCustomPaths();
}

PluginManager::~PluginManager()
{
    auto pluginListFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                            .getChildFile("Orpheus Plus/plugins.xml");
    savePluginList(pluginListFile);
}

void PluginManager::scanForPlugins()
{
    if (scanning.load()) return;
    scanning.store(true);
    OrpheusLogger::logInfo("Plugin scan started.");

    juce::Thread::launch([this]
    {
        juce::FileSearchPath paths;
        paths.addPath(getDefaultVST3Paths());
       #if JUCE_MAC
        paths.addPath(getDefaultAUPaths());
       #endif

        // Add user-specified custom directories
        paths.addPath(customSearchPaths);

        auto deadMansPedal = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                               .getChildFile("Orpheus Plus/deadmanspedal.txt");

        if (formatManager.getNumFormats() == 0)
        {
            scanning.store(false);
            juce::MessageManager::callAsync([this] {
                listeners.call(&Listener::scanComplete);
            });
            return;
        }

        // Scan into a temporary list to avoid corrupting knownPlugins on crash
        juce::KnownPluginList tempList;
        auto tempScanner = std::make_unique<juce::PluginDirectoryScanner>(
            tempList, *formatManager.getFormat(0), paths, true, deadMansPedal);

        juce::String currentPluginName;
        while (scanning.load())
        {
            bool moreToScan = false;
            try
            {
                moreToScan = tempScanner->scanNextFile(true, currentPluginName);
            }
            catch (...)
            {
                // Broken plugin — skip it and continue
                OrpheusLogger::logError("Plugin scan crashed on: " + currentPluginName);
                juce::MessageManager::callAsync([this, currentPluginName]
                {
                    listeners.call(&Listener::scanProgress, -1.0f,
                                   "SKIPPED (crash): " + currentPluginName);
                });
                continue;
            }

            if (!moreToScan) break;

            float progress = tempScanner->getProgress();
            juce::MessageManager::callAsync([this, progress, currentPluginName]
            {
                listeners.call(&Listener::scanProgress, progress, currentPluginName);
            });
        }

        // Merge results safely on the message thread
        auto scannedTypes = tempList.getTypes();
        scanning.store(false);

        juce::MessageManager::callAsync([this, scannedTypes]
        {
            for (const auto& desc : scannedTypes)
                knownPlugins.addType(desc);

            OrpheusLogger::logInfo("Plugin scan complete. " + juce::String(knownPlugins.getNumTypes()) + " plugins known.");
            listeners.call(&Listener::scanComplete);
            listeners.call(&Listener::pluginListChanged);

            // Auto-save after scan
            auto pluginListFile = juce::File::getSpecialLocation(
                juce::File::userApplicationDataDirectory)
                .getChildFile("Orpheus Plus/plugins.xml");
            savePluginList(pluginListFile);
        });
    });
}

void PluginManager::cancelScan()
{
    scanning.store(false);
}

std::unique_ptr<juce::AudioPluginInstance>
PluginManager::loadPlugin(const juce::PluginDescription& desc, juce::String& errorMessage, bool useSandbox)
{
    if (useSandbox)
    {
        // Mock Sandbox execution
        auto sandbox = std::make_unique<PluginSandboxHost>();
        if (sandbox->launchSandboxProcess(desc.fileOrIdentifier))
        {
            OrpheusLogger::logInfo("Plugin loaded in Sandbox: " + desc.name);
            // Ideally we return a ProxyAudioPluginInstance that wraps the IPC calls.
            // For now, we fallback to normal instantiation but keep the sandbox running.
            // sandboxes[nodeID] = std::move(sandbox); // Done in addPluginToTrack
        }
        else
        {
            errorMessage = "Failed to launch Sandbox Process.";
            return nullptr;
        }
    }

    return formatManager.createPluginInstance(
        desc, engine.getDeviceManager().getCurrentAudioDevice()
                ? engine.getDeviceManager().getCurrentAudioDevice()->getCurrentSampleRate() : 44100.0,
        512, errorMessage);
}





void PluginManager::openPluginEditor(int trackIndex, int pluginSlot)
{
    auto& info = engine.getTrackInfo(trackIndex);
    if (pluginSlot < 0 || pluginSlot >= OrpheusTrackInfo::MAX_PLUGINS) return;

    int nodeID = info.pluginSlots[pluginSlot];
    if (nodeID == -1) return;

    auto node = engine.processorGraph.getNodeForId(juce::AudioProcessorGraph::NodeID(nodeID));
    if (!node) return;

    auto* processor = node->getProcessor();
    if (!processor) return;

    // Check availability of editor
    auto* editor = processor->createEditor();
    
    // Create window (self-deletes on close)
    auto* window = new PluginWindow(*processor, editor);
    window->onClose = [window] {  };
}

juce::String PluginManager::getPluginName(int nodeID) const
{
    if (nodeID == -1) return {};
    auto node = engine.processorGraph.getNodeForId(juce::AudioProcessorGraph::NodeID(nodeID));
    if (node && node->getProcessor())
        return node->getProcessor()->getName();
    return {};
}

void PluginManager::addPluginToTrack(int trackIndex, const juce::PluginDescription& desc, bool useSandbox)
{
    juce::String err;
    auto instance = loadPlugin(desc, err, useSandbox);
    if (!instance)
    {
        OrpheusLogger::logError("Failed to load plugin: " + desc.name + " — " + err);
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
            "Plugin Load Error", err);
        return;
    }

    OrpheusLogger::logInfo("Plugin loaded: " + desc.name + " on track " + juce::String(trackIndex));

    // Add to graph
    auto& graph = engine.processorGraph;
    auto node = graph.addNode(std::move(instance));
    int newNodeID = (int)node->nodeID.uid;

    auto& info = engine.getTrackInfo(trackIndex);
    
    // Find empty slot
    int slot = -1;
    for (int i = 0; i < OrpheusTrackInfo::MAX_PLUGINS; ++i)
    {
        if (info.pluginSlots[i] == -1)
        {
            slot = i;
            info.pluginSlots[i] = newNodeID;
            
            if (useSandbox)
            {
                auto sandbox = std::make_unique<PluginSandboxHost>();
                sandbox->launchSandboxProcess(desc.fileOrIdentifier);
                sandboxes[newNodeID] = std::move(sandbox);
            }
            
            break;
        }
    }

    if (slot == -1)
    {
        graph.removeNode(node->nodeID); // Full
        return; // TODO: Alert user
    }

    // CONNECTIONS removed - now handled by updateTrackGraphConnections
    engine.updateTrackGraphConnections(trackIndex);
    listeners.call(&Listener::pluginListChanged); // Refresh UI
}

void PluginManager::removePluginFromTrack(int trackIndex, int pluginSlot)
{
    if (trackIndex < 0 || trackIndex >= engine.getNumTracks()) return;
    auto& info = engine.getTrackInfo(trackIndex);
    
    if (pluginSlot < 0 || pluginSlot >= OrpheusTrackInfo::MAX_PLUGINS) return;
    int nodeID = info.pluginSlots[pluginSlot];
    
    if (nodeID == -1) return;
    
    auto& graph = engine.processorGraph;
    
    // Identify Prev and Next to stitch them together
    juce::AudioProcessorGraph::NodeID sourceID = juce::AudioProcessorGraph::NodeID(info.generatorNodeID);
    juce::AudioProcessorGraph::NodeID destID   = juce::AudioProcessorGraph::NodeID(info.faderNodeID);

    juce::AudioProcessorGraph::NodeID prevID = sourceID;
    for (int k = pluginSlot - 1; k >= 0; --k)
    {
        if (info.pluginSlots[k] != -1)
        {
            prevID = juce::AudioProcessorGraph::NodeID(info.pluginSlots[k]);
            break;
        }
    }

    juce::AudioProcessorGraph::NodeID nextID = destID;
    for (int k = pluginSlot + 1; k < OrpheusTrackInfo::MAX_PLUGINS; ++k)
    {
        if (info.pluginSlots[k] != -1)
        {
            nextID = juce::AudioProcessorGraph::NodeID(info.pluginSlots[k]);
            break;
        }
    }
    
    // Disconnect Plugin
    graph.disconnectNode(juce::AudioProcessorGraph::NodeID(nodeID));
    graph.removeNode(juce::AudioProcessorGraph::NodeID(nodeID));
    
    // Remove sandbox if exists
    sandboxes.erase(nodeID);

    info.pluginSlots[pluginSlot] = -1;
    engine.updateTrackGraphConnections(trackIndex);
    listeners.call(&Listener::pluginListChanged);
}

void PluginManager::movePlugin(int trackIndex, int oldSlot, int newSlot)
{
    if (trackIndex < 0 || trackIndex >= engine.getNumTracks()) return;
    auto& info = engine.getTrackInfo(trackIndex);

    if (oldSlot < 0 || oldSlot >= OrpheusTrackInfo::MAX_PLUGINS) return;
    if (newSlot < 0 || newSlot >= OrpheusTrackInfo::MAX_PLUGINS) return;
    if (oldSlot == newSlot) return;

    int nodeIDToMove = info.pluginSlots[oldSlot];
    if (nodeIDToMove == -1) return;

    // Shift logic: Move plugin from oldSlot to newSlot, shifting others
    // If we want a SWAP logic:
    // std::swap(info.pluginSlots[oldSlot], info.pluginSlots[newSlot]);
    
    // Shift logic is usually better for chains:
    int pluginToMove = info.pluginSlots[oldSlot];
    
    if (oldSlot < newSlot) {
        for (int i = oldSlot; i < newSlot; ++i)
            info.pluginSlots[i] = info.pluginSlots[i+1];
    } else {
        for (int i = oldSlot; i > newSlot; --i)
            info.pluginSlots[i] = info.pluginSlots[i-1];
    }
    info.pluginSlots[newSlot] = pluginToMove;

    engine.updateTrackGraphConnections(trackIndex);
    listeners.call(&Listener::pluginListChanged);
}


void PluginManager::savePluginList(const juce::File& file)
{
    if (auto xml = knownPlugins.createXml())
    {
        file.getParentDirectory().createDirectory();
        xml->writeTo(file);
    }
}

void PluginManager::loadPluginList(const juce::File& file)
{
    if (file.existsAsFile())
    {
        if (auto xml = juce::XmlDocument::parse(file))
            knownPlugins.recreateFromXml(*xml);
    }
}

void PluginManager::addSearchPath(const juce::File& path)
{
    if (!path.isDirectory())
    {
        OrpheusLogger::logError("PluginManager: addSearchPath — not a valid directory: " + path.getFullPathName());
        return;
    }

    // Check if already in the list
    for (int i = 0; i < customSearchPaths.getNumPaths(); ++i)
    {
        if (customSearchPaths[i] == path)
        {
            OrpheusLogger::logInfo("PluginManager: Path already registered: " + path.getFullPathName());
            return;
        }
    }

    customSearchPaths.add(path);
    saveCustomPaths();
    OrpheusLogger::logInfo("PluginManager: Added custom VST path: " + path.getFullPathName());
}

void PluginManager::removeSearchPath(const juce::File& path)
{
    juce::FileSearchPath newPaths;
    bool found = false;
    for (int i = 0; i < customSearchPaths.getNumPaths(); ++i)
    {
        if (customSearchPaths[i] == path)
            found = true;
        else
            newPaths.add(customSearchPaths[i]);
    }

    if (found)
    {
        customSearchPaths = newPaths;
        saveCustomPaths();
        OrpheusLogger::logInfo("PluginManager: Removed custom VST path: " + path.getFullPathName());
    }
}

void PluginManager::saveCustomPaths()
{
    auto pathsFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                       .getChildFile("Orpheus Plus/customPaths.xml");
    pathsFile.getParentDirectory().createDirectory();

    juce::XmlElement root("CustomPluginPaths");
    for (int i = 0; i < customSearchPaths.getNumPaths(); ++i)
    {
        auto* pathEl = root.createNewChildElement("Path");
        pathEl->setAttribute("dir", customSearchPaths[i].getFullPathName());
    }
    root.writeTo(pathsFile);
}

void PluginManager::loadCustomPaths()
{
    auto pathsFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                       .getChildFile("Orpheus Plus/customPaths.xml");
    if (!pathsFile.existsAsFile()) return;

    if (auto xml = juce::XmlDocument::parse(pathsFile))
    {
        for (auto* child : xml->getChildWithTagNameIterator("Path"))
        {
            juce::File dir(child->getStringAttribute("dir"));
            if (dir.isDirectory())
                customSearchPaths.add(dir);
        }
        OrpheusLogger::logInfo("PluginManager: Loaded " + juce::String(customSearchPaths.getNumPaths()) + " custom VST paths.");
    }
}

juce::FileSearchPath PluginManager::getDefaultVST3Paths()
{
    juce::FileSearchPath paths;
   #if JUCE_WINDOWS
    paths.add(juce::File("C:\\Program Files\\Common Files\\VST3"));
    paths.add(juce::File("C:\\Program Files (x86)\\Common Files\\VST3"));
   #elif JUCE_MAC
    paths.add(juce::File("/Library/Audio/Plug-Ins/VST3"));
    paths.add(juce::File("~/Library/Audio/Plug-Ins/VST3"));
   #elif JUCE_LINUX
    paths.add(juce::File("/usr/lib/vst3"));
    paths.add(juce::File("/usr/local/lib/vst3"));
    paths.add(juce::File("~/.vst3"));
   #endif
    return paths;
}

juce::FileSearchPath PluginManager::getDefaultAUPaths()
{
    juce::FileSearchPath paths;
   #if JUCE_MAC
    paths.add(juce::File("/Library/Audio/Plug-Ins/Components"));
    paths.add(juce::File("~/Library/Audio/Plug-Ins/Components"));
   #endif
    return paths;
}
