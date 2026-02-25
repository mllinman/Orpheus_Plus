#include <JuceHeader.h>
#include "PluginManager.h"
#include "AudioEngine.h"
#include "../UI/PluginWindow.h"

PluginManager::PluginManager(AudioEngine& e) : engine(e)
{
    formatManager.addFormat(new juce::VST3PluginFormat());
   #if JUCE_MAC
    formatManager.addFormat(new juce::AudioUnitPluginFormat());
   #endif

    auto pluginListFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                            .getChildFile("OrpheusPlus/plugins.xml");
    loadPluginList(pluginListFile);
}

PluginManager::~PluginManager()
{
    auto pluginListFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                            .getChildFile("OrpheusPlus/plugins.xml");
    savePluginList(pluginListFile);
}

void PluginManager::scanForPlugins()
{
    if (scanning.load()) return;
    scanning.store(true);

    juce::Thread::launch([this]
    {
        juce::FileSearchPath paths;
        paths.addPath(getDefaultVST3Paths());
       #if JUCE_MAC
        paths.addPath(getDefaultAUPaths());
       #endif

        auto deadMansPedal = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                               .getChildFile("OrpheusPlus/deadmanspedal.txt");

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

            listeners.call(&Listener::scanComplete);
            listeners.call(&Listener::pluginListChanged);

            // Auto-save after scan
            auto pluginListFile = juce::File::getSpecialLocation(
                juce::File::userApplicationDataDirectory)
                .getChildFile("OrpheusPlus/plugins.xml");
            savePluginList(pluginListFile);
        });
    });
}

void PluginManager::cancelScan()
{
    scanning.store(false);
}

std::unique_ptr<juce::AudioPluginInstance>
PluginManager::loadPlugin(const juce::PluginDescription& desc, juce::String& errorMessage)
{
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

void PluginManager::addPluginToTrack(int trackIndex, const juce::PluginDescription& desc)
{
    juce::String err;
    auto instance = loadPlugin(desc, err);
    if (!instance)
    {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
            "Plugin Load Error", err);
        return;
    }

    // Add to graph
    auto& graph = engine.processorGraph;
    auto node = graph.addNode(std::move(instance));
    int newNodeID = (int)node->nodeID.uid;

    auto& info = engine.getTrackInfo(trackIndex);
    
    // Find empty slot (or intended slot? The signature doesn't specify slot. Assuming append or find first empty).
    int slot = -1;
    for (int i = 0; i < OrpheusTrackInfo::MAX_PLUGINS; ++i)
    {
        if (info.pluginSlots[i] == -1)
        {
            slot = i;
            info.pluginSlots[i] = newNodeID;
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
    // Stored for next scan
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
