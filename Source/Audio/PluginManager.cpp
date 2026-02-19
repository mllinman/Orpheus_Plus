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
        // Connect InputNode -> [New] -> TrackProcessor
        // Previous connection was InputNode -> TrackProcessor.
        
        // Strategy: 
        // 1. Disconnect Prev -> Dest.
        // 2. Connect Prev -> New.
        // 3. Connect New -> Dest.
        
        // Determine Actual Previous Node
        juce::AudioProcessorGraph::NodeID actualPrevID;
        int prevOutIndex = 0; // Usually 0?
        
        if (slot == 0)
        {
            // Slot 0 input is Engine Input (for now) or nothing if generator?
            // Wait, tracks should interpret Input. 
            // In AudioEngine::addAudioTrack, we connect node(TrackProcessor) to Master.
            // We do NOT currently have a "Track Input" node in the graph for each track unless it's the global input.
            // But we can just assume the Chain ends at TrackProcessor?
            // "TrackProcessor" is the specific node for volume/pan.
            
            // If we want plugins BEFORE the fader (TrackProcessor), we insert them before TrackProcessor.
            // But where does the signal come from?
            // 1. Audio Clips (rendered directly into buffer?) -> If so, plugins should run ON buffer.
            //    Currently AudioEngine::processAudioBlock collects MIDI and processes graph.
            //    The graph nodes are just "TrackProcessor".
            //    The timeline rendering (clips) happens WHERE?
            //    Ah, AudioEngine::processAudioBlock calls processorGraph.processBlock.
            //    But who fills the buffer with clip data?
            //    In `AudioEngine.cpp`, we see `processAudioBlock`:
            //       midiCollector.removeNextBlock();
            //       processorGraph.processBlock(buffer, midiBuffer);
            //    Wait, where is the file reading?
            //    I don't see any code that renders clips into the graph inputs!
            //    `TrackProcessor` has `processBlock`.
            //    Does `TrackProcessor` read clips? NO.
            //    `TrackProcessor` is just vol/pan.
            
            //    MAJOR ISSUE: We have no sound source for clips in the graph yet!
            //    The clips are just data in `OrpheusTrackInfo`.
            //    We need a "ClipPlayerNode" or `TrackProcessor` needs to play clips.
            //    The `TrackProcessor` I viewed earlier:
            //       processBlock checks `muted`, clears buffer if muted.
            //       Applies volume/pan.
            //       It does NOT read clips.
            
            //    So currently, the app produces silence even with clips?
            //    Yes. I missed that "Render Clips" step in AudioEngine.
            
            //    For now, let's assume `TrackProcessor` IS the source (it should read clips?).
            //    OR we need a SourceNode before it.
            //    Let's make `TrackProcessor` capable of reading clips?
            //    Or better: The plugins go AFTER `TrackProcessor`? No, fader should be post-FX usually.
            //    Unless it's "Insert" effects.
            
            //    Standard chain: [Clip Player] -> [Inserts] -> [Fader/Pan] -> [Master]
            
            //    So we need a [Track Source Node].
            //    For this task ("Plugin Hosting UI"), I will implement the connections Assuming there is a Source Node, 
            //    OR I will connect Input -> Plugin -> TrackProcessor (Input being global input, which is wrong for playback but okay for live monitoring).
            
            //    To make this work without refactoring the whole engine:
            //    Let's assume the Plugins are Post-Fader? No that's bad.
            //    Pred-Fader is better.
            
            //    Let's just chain them: 
            //    [Pre-Fader Input] -> Plugin -> [Fader]
            
            //    If we don't have a Pre-Fader Input Node, we can't connect the first plugin's input.
            //    So the first plugin effectively becomes a generator if it doesn't receive input.
            //    That's fine for Synthesizers (VSTi).
            
            //    For Audio FX, we need a source.
            
            //    Update: I will implement the chaining logic.
            //    PrevNode = (slot==0) ? engine.inputNode->nodeID : info.pluginSlots[slot-1];
            
            //    Wait, engine.inputNode is global audio input.
            //    That's fine for now (Live input monitoring).
            //    Later I will add Clip Players.
            
            if (slot == 0)
                actualPrevID = engine.inputNode->nodeID;
            else
                actualPrevID = juce::AudioProcessorGraph::NodeID(info.pluginSlots[slot - 1]);
        }
        else
        {
             actualPrevID = juce::AudioProcessorGraph::NodeID(info.pluginSlots[slot - 1]);
        }
        
        // Dest
        juce::AudioProcessorGraph::NodeID destID = juce::AudioProcessorGraph::NodeID(info.nodeID); // TrackProcessor
        // If there are plugins AFTER this slot (which isn't the case for append-only logic here), we'd need to find the next one.
        // But since we just filled the FIRST empty slot, and we fill linearly 0..3...
        // Any slot > ThisSlot is empty (-1).
        // So Dest is always TrackProcessor (or next populated slot if we allowed sparse filling).
        
        // Check for next populated slot?
        for (int k = slot + 1; k < OrpheusTrackInfo::MAX_PLUGINS; ++k) {
            if (info.pluginSlots[k] != -1) {
                destID = juce::AudioProcessorGraph::NodeID(info.pluginSlots[k]);
                break;
            }
        }
        
        // 1. Disconnect Prev -> Dest
        for (int ch = 0; ch < 2; ++ch)
            graph.removeConnection({ { actualPrevID, ch }, { destID, ch } });
            
        // 2. Connect Prev -> New
        for (int ch = 0; ch < 2; ++ch)
            graph.addConnection({ { actualPrevID, ch }, { node->nodeID, ch } });
            
        // 3. Connect New -> Dest
        for (int ch = 0; ch < 2; ++ch)
            graph.addConnection({ { node->nodeID, ch }, { destID, ch } });
    }
    
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
    juce::AudioProcessorGraph::NodeID prevID;
    if (pluginSlot == 0)
        prevID = engine.inputNode->nodeID;
    else
        prevID = juce::AudioProcessorGraph::NodeID(info.pluginSlots[pluginSlot - 1]);
        
    juce::AudioProcessorGraph::NodeID nextID = juce::AudioProcessorGraph::NodeID(info.nodeID); // TrackProcessor
    for (int k = pluginSlot + 1; k < OrpheusTrackInfo::MAX_PLUGINS; ++k) {
        if (info.pluginSlots[k] != -1) {
            nextID = juce::AudioProcessorGraph::NodeID(info.pluginSlots[k]);
            break;
        }
    }
    
    // Disconnect Plugin
    graph.disconnectNode(juce::AudioProcessorGraph::NodeID(nodeID));
    graph.removeNode(juce::AudioProcessorGraph::NodeID(nodeID));
    
    // Stitch Prev -> Next
    if (prevID.uid != 0 && nextID.uid != 0) // Valid IDs
    {
         for (int ch = 0; ch < 2; ++ch)
            graph.addConnection({ { prevID, ch }, { nextID, ch } });
    }
    
    info.pluginSlots[pluginSlot] = -1;
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
