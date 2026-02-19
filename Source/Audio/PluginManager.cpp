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

        if (formatManager.getNumFormats() > 0)
        {
            scanner.reset(new juce::PluginDirectoryScanner(
                knownPlugins, *formatManager.getFormat(0), paths, true, deadMansPedal));
        }

        juce::String currentPluginName;
        while (scanner->scanNextFile(true, currentPluginName))
        {
            float progress = scanner->getProgress();
            juce::MessageManager::callAsync([this, progress, currentPluginName]
            {
                listeners.call(&Listener::scanProgress, progress, currentPluginName);
            });

            if (!scanning.load()) break;
        }

        scanning.store(false);
        juce::MessageManager::callAsync([this]
        {
            listeners.call(&Listener::scanComplete);
            listeners.call(&Listener::pluginListChanged);
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



void PluginManager::removePluginFromTrack(int /*trackIndex*/, int /*pluginSlot*/)
{
    // TODO: remove from graph
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
    new PluginWindow(*processor, editor);
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

    // CONNECTIONS
    // Chain: [Source] -> [Plug0] -> [Plug1] -> [TrackProc] -> [Master]
    // We need to insert newNode into chain.
    // Simplifying assumption:
    // If slot 0: Connect Source/Input -> NewNode -> (NextPlug/TrackProc)
    // If slot N: Connect (PrevPlug) -> NewNode -> (NextPlug/TrackProc)
    
    // BUT, we need to know what "Source" is.
    // Currently AudioEngine doesn't seem to have per-track sources in the graph explicitly named in OrpheusTrackInfo?
    // Tracks usually have an input node if recording, or they are just players.
    // Let's assume for now we just connect NewNode -> TrackProcessor.
    // If there were previous plugins, we'd chain them.
    
    // Re-evaluate connections for this track
    // Disconnect everything feeding specific nodes? No, that's messy.
    
    // Strategy:
    // 1. Identify "Destination" node (TrackProcessor).
    // 2. Identify "Previous" node.
    //    If slot == 0: ?? (Maybe no input for now, or we define a TrackInputNode later)
    //    If slot > 0: Previous is info.pluginSlots[slot-1].
    
    // 3. Connect Previous -> Current.
    // 4. Connect Current -> Next (or Destination).
    
    int destNodeID = info.nodeID; // TrackProcessor
    
    // If not last slot, dest might be next plugin?
    // But we just filled the first empty slot, which is effectively the last used slot.
    // So Next is TrackProcessor.
    
    // Previous?
    int prevNodeID = -1;
    if (slot > 0)
        prevNodeID = info.pluginSlots[slot - 1];
        
    // If prevNodeID is -1 (slot 0), we don't have a stable "Source" node in OrpheusTrackInfo yet.
    // So let's skip input connection for slot 0 for now (or connect to nothing).
    // BUT we MUST connect to Output (TrackProcessor).
    
    if (destNodeID != -1)
    {
        // If there was a connection from Prev -> Dest, remove it?
        // Yes, if we are inserting.
        // But if we are appending (slot N), the previous last plugin (or source) was connected to Dest.
        
        // Case: Appending at slot 0.
        // Prev: None/Source. Dest: TrackProc.
        // Old: Source -> TrackProc.
        // New: Source -> NewNode -> TrackProc.
        // We need to find what feeds TrackProc and move it to NewNode.
        
        // Helper: Rewire connection feeding 'Dest' to feed 'NewNode' instead?
        // graph.getConnections() ...
        
        for (int ch = 0; ch < 2; ++ch)
             graph.addConnection({ { (juce::AudioProcessorGraph::NodeID)newNodeID, ch }, 
                                   { (juce::AudioProcessorGraph::NodeID)destNodeID, ch } });
                                   
        // If there was a previous plugin, play nice
        if (prevNodeID != -1)
        {
             // Disconnect Prev -> TrackProc
             for (int ch = 0; ch < 2; ++ch)
                 graph.removeConnection({ { (juce::AudioProcessorGraph::NodeID)prevNodeID, ch }, 
                                          { (juce::AudioProcessorGraph::NodeID)destNodeID, ch } });
                                          
             // Connect Prev -> NewNode
             for (int ch = 0; ch < 2; ++ch)
                 graph.addConnection({ { (juce::AudioProcessorGraph::NodeID)prevNodeID, ch }, 
                                       { (juce::AudioProcessorGraph::NodeID)newNodeID, ch } });
        }
    }
    
    engine.listeners.call(&AudioEngine::Listener::trackListChanged);
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
