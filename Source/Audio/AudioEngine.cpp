#include "AudioEngine.h"
#include <JuceHeader.h>
#include "PluginManager.h"
#include "ClipGeneratorProcessor.h"
#include "MidiGeneratorProcessor.h"
#include "TrackFaderProcessor.h"
#include "MixerProcessor.h"
#include "MidiLearnManager.h"
#include "../Project/AppState.h"
#include "../PitchCorrection/AutoTuneProcessor.h"
#include "../AudioCleanup/AudioCleanupProcessor.h"
// #include "../UI/SpectrumAnalyzer.h"


AudioEngine::AudioEngine()
{
    formatManager.registerBasicFormats();

    pluginManager  = std::make_unique<PluginManager>(*this);
    midiLearnManager = std::make_unique<MidiLearnManager>();
    stemSeparator  = std::make_unique<StemSeparator>();
    audioToMidi    = std::make_unique<AudioToMidiConverter>();
    // autoTune       = std::make_unique<AutoTuneProcessor>();
    // audioCleanup   = std::make_unique<AudioCleanupProcessor>();

    initialise();
}

AudioEngine::~AudioEngine()
{
    shutdown();
}

void AudioEngine::initialise()
{
    deviceManager.initialiseWithDefaultDevices(2, 2);
    deviceManager.addAudioCallback(this);

    // Enable all MIDI inputs
    for (auto& id : juce::MidiInput::getAvailableDevices())
        deviceManager.setMidiInputDeviceEnabled(id.identifier, true);
    deviceManager.addMidiInputDeviceCallback({}, this);

    midiCollector.reset(44100.0);
    audioWriterThread.startThread(juce::Thread::Priority::high); // Priority slightly above normal

    // Initialize Graph
    processorGraph.clear();
    using AudioGraphIOProcessor = juce::AudioProcessorGraph::AudioGraphIOProcessor;

    // Create IO nodes
    inputNode = processorGraph.addNode(std::make_unique<AudioGraphIOProcessor>(AudioGraphIOProcessor::audioInputNode));
    outputNode = processorGraph.addNode(std::make_unique<AudioGraphIOProcessor>(AudioGraphIOProcessor::audioOutputNode));
    
    // Create Master Node
    masterNode = processorGraph.addNode(std::make_unique<MixerProcessor>());

    // Connect Master -> Output
    if (masterNode && outputNode)
    {
        for (int ch = 0; ch < 2; ++ch)
            processorGraph.addConnection({ { masterNode->nodeID, ch }, { outputNode->nodeID, ch } });
    }
}

void AudioEngine::shutdown()
{
    if (audioWriter) audioWriter.reset();
    audioWriterThread.stopThread(2000);

    deviceManager.removeAudioCallback(this);
    deviceManager.removeMidiInputDeviceCallback({}, this);
}

//──────────────────────────────────────────────────────────────────────────────
// Transport
//──────────────────────────────────────────────────────────────────────────────
void AudioEngine::play()
{
    if (!playing.load())
    {
        playing.store(true);
        listeners.call(&Listener::playbackStarted);
    }
}

void AudioEngine::stop()
{
    playing.store(false);
    
    if (recording.load())
    {
        recording.store(false);
        // Finalize recording
        if (audioWriter)
        {
            audioWriter.reset(); // flushes and closes file
            
            // Add new AudioClip to track
            if (armedTrackIndex >= 0 && armedTrackIndex < tracks.size())
            {
                auto* clip = new AudioClip(currentRecordingFile, 0.0 /* start position */);
                tracks[armedTrackIndex]->clips.add(clip);
            }
        }
        
        if (recordedMidi.getNumEvents() > 0 && armedTrackIndex >= 0 && armedTrackIndex < tracks.size())
        {
            // Add new MidiClip
            auto* clip = new MidiClip(0.0, recordedMidi.getEndTime());
            clip->midiData = recordedMidi;
            tracks[armedTrackIndex]->clips.add(clip);
        }
    }
    
    playheadPosition.store(0.0);
    listeners.call(&Listener::playbackStopped);
}

void AudioEngine::pause()
{
    if (playing.load())
    {
        playing.store(false);
        listeners.call(&Listener::playbackStopped);
    }
}

void AudioEngine::togglePlayback()
{
    if (playing.load()) pause();
    else                 play();
}

void AudioEngine::toggleRecord()
{
    if (!recording.load())
    {
        // START RECORDING
        armedTrackIndex = -1;
        for (int i = 0; i < tracks.size(); ++i)
        {
            if (tracks[i]->armed) { armedTrackIndex = i; break; }
        }

        if (armedTrackIndex >= 0)
        {
            if (tracks[armedTrackIndex]->type == OrpheusTrackInfo::Type::Audio)
            {
                juce::File docsDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
                juce::File projectFolder = docsDir.getChildFile("OrpheusPlus_Projects");
                projectFolder.createDirectory();
                
                currentRecordingFile = projectFolder.getNonexistentChildFile("Recording", ".wav");
                
                juce::WavAudioFormat wavFormat;
                auto stream = currentRecordingFile.createOutputStream();
                if (stream != nullptr)
                {
                    auto* writer = wavFormat.createWriterFor(stream.release(), currentSampleRate, 1, 24, {}, 0);
                    if (writer)
                        audioWriter = std::make_unique<juce::AudioFormatWriter::ThreadedWriter>(writer, audioWriterThread, 32768);
                }
            }
            else if (tracks[armedTrackIndex]->type == OrpheusTrackInfo::Type::Midi)
            {
                recordedMidi.clear();
            }
        }
        
        recording.store(true);
        if (!playing.load()) play();
    }
    else
    {
        // Stop recording
        recording.store(false);
        stop(); // this will trigger the finalize logic in stop()
    }
}

void AudioEngine::setPlayheadPosition(double posSeconds)
{
    playheadPosition.store(juce::jmax(0.0, posSeconds));
}

//──────────────────────────────────────────────────────────────────────────────
// Track management
//──────────────────────────────────────────────────────────────────────────────
void AudioEngine::syncWithAppState(AppState& state)
{
    // Clear existing tracks
    const auto trackCount = tracks.size();
    for (int i = trackCount - 1; i >= 0; --i)
        removeTrack(i);

    bpm.store(state.getBpm());
    timeSigNum = state.getTimeSigNum();
    timeSigDen = state.getTimeSigDen();
    // Add tracks from AppState
    for (int i = 0; i < state.getNumTracks(); ++i)
    {
        auto child = state.getTrackNode(i);
        if (child.isValid() && child.hasType("Track"))
        {
            juce::String type = child.getProperty("type");
            juce::String name = child.getProperty("name");
            int idx = -1;
            
            if (type == "audio" || type == "vocal")       idx = addAudioTrack(name);
            else if (type == "midi" || type == "instrument") idx = addMidiTrack(name);
            else if (type == "folder")                       idx = addFolderTrack(name);
            else if (type == "arranger")                     idx = addArrangerTrack(name);
            
            if (idx >= 0)
            {
                setTrackVolume(idx, static_cast<float>(child.getProperty("vol", 1.0f)));
                setTrackPan(idx, static_cast<float>(child.getProperty("pan", 0.0f)));
                setTrackMute(idx, static_cast<bool>(child.getProperty("mute", false)));
                setTrackSolo(idx, static_cast<bool>(child.getProperty("solo", false)));
                tracks[idx]->expanded = static_cast<bool>(child.getProperty("expanded", true));
                tracks[idx]->depth = state.getTracks()[(size_t)i].depth;
            }
        }
    }
}

int AudioEngine::addAudioTrack(const juce::String& name)
{
    auto* t = new OrpheusTrackInfo();
    t->name = name;
    t->type = OrpheusTrackInfo::Type::Audio;
    t->colour = juce::Colours::cornflowerblue.withSaturation(0.7f);

    // 1. Create Generator
    std::unique_ptr<juce::AudioProcessor> generator;
    generator = std::make_unique<ClipGeneratorProcessor>(*t, *this);

    juce::AudioProcessorGraph::Node::Ptr genNode;
    if (generator)
    {
        genNode = processorGraph.addNode(std::move(generator));
        t->generatorNodeID = (int)genNode->nodeID.uid;
    }

    // 2. Create Fader
    auto faderProc = std::make_unique<TrackFaderProcessor>();
    auto faderNode = processorGraph.addNode(std::move(faderProc));
    t->faderNodeID = (int)faderNode->nodeID.uid;

    // 3. Connect Generator -> Fader (for now, no plugins)
    if (genNode && faderNode)
    {
        for (int ch = 0; ch < 2; ++ch)
            processorGraph.addConnection({ { genNode->nodeID, ch }, { faderNode->nodeID, ch } });
    }

    // 4. Connect Fader -> Master
    if (faderNode && masterNode)
    {
        for (int ch = 0; ch < 2; ++ch)
            processorGraph.addConnection({ { faderNode->nodeID, ch }, { masterNode->nodeID, ch } });
    }
    
    tracks.add(t);

    juce::MessageManager::callAsync([this]{ listeners.call(&Listener::trackListChanged); });
    return tracks.size() - 1;
}

int AudioEngine::addMidiTrack(const juce::String& name)
{
    auto* t = new OrpheusTrackInfo();
    t->name = name;
    t->type = OrpheusTrackInfo::Type::Midi;
    t->colour = juce::Colours::mediumpurple;

    // 1. Create Generator
    auto generator = std::make_unique<MidiGeneratorProcessor>(*t, *this);
    auto genNode = processorGraph.addNode(std::move(generator));
    t->generatorNodeID = (int)genNode->nodeID.uid;

    // 2. Create Fader
    auto faderProc = std::make_unique<TrackFaderProcessor>();
    auto faderNode = processorGraph.addNode(std::move(faderProc));
    t->faderNodeID = (int)faderNode->nodeID.uid;

    // 3. Connect Generator -> Fader
    if (genNode && faderNode)
    {
        for (int ch = 0; ch < 2; ++ch)
            processorGraph.addConnection({ { genNode->nodeID, ch }, { faderNode->nodeID, ch } });
    }

    // 4. Connect Fader -> Master
    if (faderNode && masterNode)
    {
        for (int ch = 0; ch < 2; ++ch)
            processorGraph.addConnection({ { faderNode->nodeID, ch }, { masterNode->nodeID, ch } });
    }
    
    tracks.add(t);

    juce::MessageManager::callAsync([this]{ listeners.call(&Listener::trackListChanged); });
    return tracks.size() - 1;
}

int AudioEngine::addFolderTrack(const juce::String& name)
{
    auto* t = new OrpheusTrackInfo();
    t->name = name;
    t->type = OrpheusTrackInfo::Type::Folder;
    t->colour = juce::Colour(0xfff39c12);

    tracks.add(t);
    juce::MessageManager::callAsync([this] { listeners.call(&Listener::trackListChanged); });
    return tracks.size() - 1;
}

int AudioEngine::addArrangerTrack(const juce::String& name)
{
    auto* t = new OrpheusTrackInfo();
    t->name = name;
    t->type = OrpheusTrackInfo::Type::Arranger;
    t->colour = juce::Colours::white;
    
    // Arranger is usually the first track visually
    tracks.insert(0, t);
    juce::MessageManager::callAsync([this] { listeners.call(&Listener::trackListChanged); });
    return 0;
}

int AudioEngine::addBusTrack(const juce::String& name)
{
    auto* t = new OrpheusTrackInfo();
    t->name   = name;
    t->type   = OrpheusTrackInfo::Type::Bus;
    t->colour = juce::Colours::darkorange;
    tracks.add(t);

    juce::MessageManager::callAsync([this]{ listeners.call(&Listener::trackListChanged); });
    return tracks.size() - 1;
}

void AudioEngine::removeTrack(int index)
{
    if (juce::isPositiveAndBelow(index, tracks.size()))
    {
        auto* track = tracks[index];
        if (track->generatorNodeID != -1)
            processorGraph.removeNode(juce::AudioProcessorGraph::NodeID(track->generatorNodeID));
        if (track->faderNodeID != -1)
            processorGraph.removeNode(juce::AudioProcessorGraph::NodeID(track->faderNodeID));
        
        for (int slot : track->pluginSlots)
            if (slot != -1)
                processorGraph.removeNode(juce::AudioProcessorGraph::NodeID(slot));

        tracks.remove(index);
        listeners.call(&Listener::trackListChanged);
    }
}

void AudioEngine::moveTrack(int fromIndex, int toIndex)
{
    if (juce::isPositiveAndBelow(fromIndex, tracks.size()) && 
        juce::isPositiveAndBelow(toIndex, tracks.size()) && 
        fromIndex != toIndex)
    {
        tracks.move(fromIndex, toIndex);
        juce::MessageManager::callAsync([this]{ listeners.call(&Listener::trackListChanged); });
    }
}

int AudioEngine::getNumTracks() const { return tracks.size(); }

OrpheusTrackInfo& AudioEngine::getTrackInfo(int index)
{
    if (!juce::isPositiveAndBelow(index, tracks.size()))
    {
        // Prevent SIGSEGV — log the bad access and return a safe dummy
        static OrpheusTrackInfo dummy;
        juce::Logger::writeToLog("ERROR: getTrackInfo(" + juce::String(index) 
            + ") out of bounds (size=" + juce::String(tracks.size()) + ")");
        jassertfalse;
        return dummy;
    }
    return *tracks[index];
}

void AudioEngine::setTrackVolume(int i, float vol) { 
    tracks[i]->volume = vol; 
    if (auto* node = processorGraph.getNodeForId(juce::AudioProcessorGraph::NodeID(tracks[i]->faderNodeID)))
        if (auto* fader = dynamic_cast<TrackFaderProcessor*>(node->getProcessor()))
            fader->setVolume(vol);
}

void AudioEngine::setTrackPan(int i, float pan) { 
    tracks[i]->pan = pan; 
    if (auto* node = processorGraph.getNodeForId(juce::AudioProcessorGraph::NodeID(tracks[i]->faderNodeID)))
        if (auto* fader = dynamic_cast<TrackFaderProcessor*>(node->getProcessor()))
            fader->setPan(pan);
}

void AudioEngine::setTrackSweetener(int i, float amount)
{
    tracks[i]->sweetener = amount;
    if (auto* node = processorGraph.getNodeForId(juce::AudioProcessorGraph::NodeID(tracks[i]->faderNodeID)))
        if (auto* fader = dynamic_cast<TrackFaderProcessor*>(node->getProcessor()))
            fader->setSweetener(amount);
}

void AudioEngine::setTrackMute(int i, bool mute)   { 
    tracks[i]->mute = mute; 
    if (auto* node = processorGraph.getNodeForId(juce::AudioProcessorGraph::NodeID(tracks[i]->faderNodeID)))
        if (auto* fader = dynamic_cast<TrackFaderProcessor*>(node->getProcessor()))
            fader->setMute(mute);
    updateSoloState(); 
}
void AudioEngine::setTrackSolo(int i, bool solo)   { tracks[i]->solo   = solo; updateSoloState(); }
void AudioEngine::armTrack(int i, bool armed)      { tracks[i]->armed  = armed; }

void AudioEngine::updateSoloState()
{
    bool anySolo = false;
    for (auto* t : tracks)
        if (t->solo) { anySolo = true; break; }

    for (auto* t : tracks)
        t->mute = anySolo ? !t->solo : false;
}

//──────────────────────────────────────────────────────────────────────────────
// Insert FX
//──────────────────────────────────────────────────────────────────────────────

void AudioEngine::addAutoTuneToTrack(int trackIndex)
{
    // TODO: Phase 2 - Insert into graph between generator and fader
}

void AudioEngine::addAudioCleanupToTrack(int trackIndex)
{
    // TODO: Phase 2 - Insert into graph between generator and fader
}

//──────────────────────────────────────────────────────────────────────────────
// Audio callback
//──────────────────────────────────────────────────────────────────────────────
void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    currentSampleRate = device->getCurrentSampleRate();
    currentBlockSize  = device->getCurrentBufferSizeSamples();
    midiCollector.reset(currentSampleRate);
    processorGraph.prepareToPlay(currentSampleRate, currentBlockSize);
}

void AudioEngine::audioDeviceStopped()
{
    processorGraph.releaseResources();
}

void AudioEngine::audioDeviceIOCallbackWithContext(
    const float* const* inputChannelData, int numInputChannels,
    float* const* outputChannelData, int numOutputChannels,
    int numSamples,
    const juce::AudioIODeviceCallbackContext&)
{
    // Clear outputs
    for (int ch = 0; ch < numOutputChannels; ++ch)
        juce::FloatVectorOperations::clear(outputChannelData[ch], numSamples);

    if (!playing.load()) return;

    juce::AudioBuffer<float> buffer(outputChannelData, numOutputChannels, numSamples);
    
    // Recording Logic
    if (recording.load())
    {
        if (audioWriter != nullptr)
        {
            // Just record from input 0 for now (mono mic)
            if (numInputChannels > 0 && inputChannelData[0] != nullptr)
            {
                const float* inputPtrs[1] = { inputChannelData[0] };
                audioWriter->write(inputPtrs, numSamples);
            }
        }
    }

    processAudioBlock(buffer);

    // Apply master volume
    buffer.applyGain(masterVolume.load());

    // Apply Mono Sum
    if (monoSum.load() && numOutputChannels >= 2)
    {
        auto* chL = buffer.getWritePointer(0);
        auto* chR = buffer.getWritePointer(1);
        for (int i = 0; i < numSamples; ++i)
        {
            float mono = (chL[i] + chR[i]) * 0.5f;
            chL[i] = mono;
            chR[i] = mono;
        }
    }

    // Advance playhead
    double advance = numSamples / currentSampleRate;
    playheadPosition.fetch_add(advance);

    // Update meters
    if (numOutputChannels >= 1)
        masterPeakL.store(buffer.getMagnitude(0, 0, numSamples));
    if (numOutputChannels >= 2)
        masterPeakR.store(buffer.getMagnitude(1, 0, numSamples));
}

void AudioEngine::processAudioBlock(juce::AudioBuffer<float>& buffer)
{
    // Collect MIDI
    midiBuffer.clear();
    midiCollector.removeNextBlockOfMessages(midiBuffer, buffer.getNumSamples());

    // Apply Automation before processing
    if (playing.load())
    {
        double time = playheadPosition.load();
        
        for (auto* track : tracks)
        {
            // Update Playhead in Generator
            if (auto* node = processorGraph.getNodeForId(juce::AudioProcessorGraph::NodeID(track->generatorNodeID)))
                if (auto* gen = dynamic_cast<ClipGeneratorProcessor*>(node->getProcessor()))
                    gen->setPlayhead(time);

            if (track->faderNodeID == -1) continue;
            
            // Find Fader processor
            auto* node = processorGraph.getNodeForId(juce::AudioProcessorGraph::NodeID(track->faderNodeID));
            if (!node) continue;
            
            auto* fader = dynamic_cast<TrackFaderProcessor*>(node->getProcessor());
            if (!fader) continue;

            for (const auto& curve : track->automationCurves)
            {
                if (curve.points.empty()) continue;
                
                // Simple Linear Interpolation
                float value = 0.0f;
                
                // Case: Time before first point
                if (time <= curve.points.front().time)
                {
                    value = curve.points.front().value;
                }
                // Case: Time after last point
                else if (time >= curve.points.back().time)
                {
                    value = curve.points.back().value;
                }
                // Case: Between points
                else
                {
                    // Find segment
                    for (size_t i = 0; i < curve.points.size() - 1; ++i)
                    {
                        const auto& p1 = curve.points[i];
                        const auto& p2 = curve.points[i+1];
                        
                        if (time >= p1.time && time < p2.time)
                        {
                            double t = (time - p1.time) / (p2.time - p1.time);
                            value = p1.value + (p2.value - p1.value) * (float)t;
                            break;
                        }
                    }
                }
                
                // Apply
                if (curve.parameterID == "vol")
                {
                    fader->setVolume(value);
                    track->volume = value; // Update model for UI
                }
                else if (curve.parameterID == "pan")
                {
                    fader->setPan(value);
                    track->pan = value;    // Update model for UI
                }
                else if (curve.parameterID == "sweet")
                {
                    fader->setSweetener(value);
                    track->sweetener = value;
                }
            }
        }
    }

    // Process Graph
    // For now, we are only handling output buffer processing.
    // Ideally, we would pass input data if we had recording enabled.
    processorGraph.processBlock(buffer, midiBuffer);

    // Update analyzers
    // for (auto* analyzer : analyzers)
    // {
    //     if (analyzer)
    //         analyzer->pushBuffer(buffer);
    // }
}

void AudioEngine::handleIncomingMidiMessage(juce::MidiInput*, const juce::MidiMessage& message)
{
    if (message.isController())
    {
        int channel = message.getChannel();
        int cc = message.getControllerNumber();
        float val = message.getControllerValue() / 127.0f;

        if (midiLearnManager->isLearning())
        {
            midiLearnManager->handleCC(channel, cc);
        }
        else
        {
            midiLearnManager->updateCCValue(channel, cc, val);
            for (const auto& m : midiLearnManager->getMappings())
            {
                if ((m.channel == channel || m.channel == 0) && m.cc == cc)
                {
                    if (m.targetParam == "vol" && m.trackIndex >= 0)
                        setTrackVolume(m.trackIndex, val * 1.5f);
                    else if (m.targetParam == "pan" && m.trackIndex >= 0)
                        setTrackPan(m.trackIndex, val * 2.0f - 1.0f);
                    else if (m.targetParam == "sweet" && m.trackIndex >= 0)
                        setTrackSweetener(m.trackIndex, val);
                }
            }
        }
    }
    midiCollector.addMessageToQueue(message);
    
    if (recording.load() && armedTrackIndex >= 0)
    {
        if (tracks[armedTrackIndex]->type == OrpheusTrackInfo::Type::Midi)
        {
            // Timestamp based on playhead position
            double timeInBeats = playheadPosition.load() * (bpm.load() / 60.0);
            recordedMidi.addEvent(message, timeInBeats);
        }
    }
}

//──────────────────────────────────────────────────────────────────────────────
void AudioEngine::registerAnalyzer(SpectrumAnalyzer* analyzer)
{
    // analyzers.add(analyzer);
}

void AudioEngine::unregisterAnalyzer(SpectrumAnalyzer* analyzer)
{
    // analyzers.remove(analyzer);
}

//──────────────────────────────────────────────────────────────────────────────
// Export
//──────────────────────────────────────────────────────────────────────────────
void AudioEngine::exportMix(const juce::File& outputFile, int sampleRate, int bitDepth)
{
    // Offline render: write to file
    juce::WavAudioFormat wavFormat;
    auto stream = outputFile.createOutputStream();
    if (!stream) return;

    std::unique_ptr<juce::AudioFormatWriter> writer(
        wavFormat.createWriterFor(stream.release(), sampleRate, 2, bitDepth, {}, 0));

    if (!writer) return;

    const int blockSize = 2048;
    juce::AudioBuffer<float> buffer(2, blockSize);
    juce::MidiBuffer midiBuf; // empty for offline graph processing right now unless we inject timeline events

    // Find the total duration
    double maxDuration = 0.0;
    for (auto* track : tracks)
    {
        for (auto* clip : track->clips)
        {
            if (clip->startTime + clip->duration > maxDuration)
                maxDuration = clip->startTime + clip->duration;
        }
    }
    
    // Add 1 second tail
    const double totalDuration = maxDuration > 0.0 ? maxDuration + 1.0 : 1.0; 
    const int64_t totalSamples = (int64_t)(totalDuration * sampleRate);

    processorGraph.prepareToPlay(sampleRate, blockSize);
    exporting.store(true);

    for (int64_t pos = 0; pos < totalSamples; pos += blockSize)
    {
        int thisBlock = (int)juce::jmin((int64_t)blockSize, totalSamples - pos);
        buffer.clear();
        
        // This invokes all track processors, plugins, and master bus
        processorGraph.processBlock(buffer, midiBuf);
        
        writer->writeFromAudioSampleBuffer(buffer, 0, thisBlock);
    }

    exporting.store(false);
    processorGraph.releaseResources();
}

void AudioEngine::exportStems(const juce::File& outputDirectory)
{
    outputDirectory.createDirectory();
    for (int i = 0; i < tracks.size(); ++i)
    {
        auto stemFile = outputDirectory.getChildFile(
            juce::File::createLegalFileName(tracks[i]->name) + ".wav");
        exportMix(stemFile); // TODO: solo individual track
    }
}

void AudioEngine::setSidechainSource(int targetTrack, int slot, int sourceTrack)
{
    if (targetTrack >= 0 && targetTrack < (int)tracks.size() && slot >= 0 && slot < OrpheusTrackInfo::MAX_PLUGINS)
    {
        tracks[targetTrack]->sidechainSources[slot] = sourceTrack;
        updateTrackGraphConnections(targetTrack);
    }
}

void AudioEngine::updateTrackGraphConnections(int trackIndex)
{
    if (!juce::isPositiveAndBelow(trackIndex, (int)tracks.size())) return;
    auto* track = tracks[trackIndex];

    // 1. Collect all nodes in the track's internal chain in order
    // Order: Generator -> Plugin[0] -> ... -> Plugin[N] -> Fader
    std::vector<juce::AudioProcessorGraph::NodeID> chain;
    
    if (track->generatorNodeID != -1)
        chain.push_back(juce::AudioProcessorGraph::NodeID(track->generatorNodeID));

    for (int slotNodeID : track->pluginSlots)
    {
        if (slotNodeID != -1)
            chain.push_back(juce::AudioProcessorGraph::NodeID(slotNodeID));
    }

    if (track->faderNodeID != -1)
        chain.push_back(juce::AudioProcessorGraph::NodeID(track->faderNodeID));

    // 2. Disconnect all nodes in this specific chain from each other
    for (const auto& conn : processorGraph.getConnections())
    {
        bool sourceInChain = std::find(chain.begin(), chain.end(), conn.source.nodeID) != chain.end();
        bool destInChain = std::find(chain.begin(), chain.end(), conn.destination.nodeID) != chain.end();

        // Also disconnect if source is Fader (so we can re-route bus/master)
        if (sourceInChain && destInChain)
            processorGraph.removeConnection(conn);
        if (conn.source.nodeID.uid == track->faderNodeID)
            processorGraph.removeConnection(conn);
    }

    // 3. Connect them in the new linear order
    for (size_t i = 0; i < chain.size() - 1; ++i)
    {
        auto nodeA = processorGraph.getNodeForId(chain[i]);
        auto nodeB = processorGraph.getNodeForId(chain[i+1]);
        
        if (nodeA && nodeB)
        {
            auto* procA = nodeA->getProcessor();
            auto* procB = nodeB->getProcessor();
            
            // Connect MIDI if applicable
            if (procA->producesMidi() && procB->acceptsMidi())
            {
                processorGraph.addConnection({ { chain[i], juce::AudioProcessorGraph::midiChannelIndex }, 
                                               { chain[i+1], juce::AudioProcessorGraph::midiChannelIndex } });
            }

            // Connect Audio if applicable
            int numOutsA = procA->getTotalNumOutputChannels();
            int numInsB  = procB->getTotalNumInputChannels();
            int audioChans = std::min({numOutsA, numInsB, 2});

            for (int ch = 0; ch < audioChans; ++ch)
            {
                processorGraph.addConnection({ { chain[i], ch }, { chain[i+1], ch } });
            }
        }
    }

    // 4. Connect Fader to Output (Master or Bus)
    if (track->faderNodeID != -1)
    {
        juce::AudioProcessorGraph::Node::Ptr destNode = masterNode;
        
        if (track->outputBus.isNotEmpty())
        {
            // Find bus track
            for (auto* t : tracks)
            {
                if (t->type == OrpheusTrackInfo::Type::Bus && t->name == track->outputBus)
                {
                    // Find first available node in the bus track's chain
                    bool found = false;
                    for (int pluginSlot : t->pluginSlots)
                    {
                        if (pluginSlot != -1)
                        {
                            destNode = processorGraph.getNodeForId(juce::AudioProcessorGraph::NodeID(pluginSlot));
                            found = true;
                            break;
                        }
                    }
                    if (!found && t->faderNodeID != -1)
                        destNode = processorGraph.getNodeForId(juce::AudioProcessorGraph::NodeID(t->faderNodeID));
                    break;
                }
            }
        }
        
        if (destNode)
        {
            juce::AudioProcessorGraph::NodeID faderID(track->faderNodeID);
            for (int ch = 0; ch < 2; ++ch)
            {
                processorGraph.addConnection({ { faderID, ch }, { destNode->nodeID, ch } });
            }
        }
    }

    // 5. Connect Sidechains
    for (int i = 0; i < OrpheusTrackInfo::MAX_PLUGINS; ++i)
    {
        int scSourceIdx = track->sidechainSources[(size_t)i];
        if (scSourceIdx != -1 && track->pluginSlots[(size_t)i] != -1 && scSourceIdx < (int)tracks.size())
        {
            auto* srcTrack = tracks[scSourceIdx];
            if (srcTrack->faderNodeID != -1)
            {
                juce::AudioProcessorGraph::NodeID scSrcID(srcTrack->faderNodeID);
                juce::AudioProcessorGraph::NodeID destPluginID(track->pluginSlots[(size_t)i]);
                
                // Typically sidechain connects to channels 2,3 (assuming 0,1 are main)
                auto destNode = processorGraph.getNodeForId(destPluginID);
                if (destNode && destNode->getProcessor()->getTotalNumInputChannels() >= 4)
                {
                    processorGraph.addConnection({ { scSrcID, 0 }, { destPluginID, 2 } });
                    processorGraph.addConnection({ { scSrcID, 1 }, { destPluginID, 3 } });
                }
            }
        }
    }
}

//==============================================================================
// MIDI Capture (Retroactive Recording)
//==============================================================================
void AudioEngine::captureMidi(int trackIndex)
{
    if (trackIndex < 0 || trackIndex >= (int)tracks.size()) return;
    if (midiCaptureBuffer_.getNumEvents() == 0) return;

    auto* track = tracks[trackIndex];

    // Create a new MidiClip from the captured buffer
    double startTime = playheadPosition.load();
    double captureLength = (double)midiCaptureSeconds_;

    auto* clip = new MidiClip(juce::jmax(0.0, startTime - captureLength), captureLength);

    // Copy events from the capture buffer, offsetting timestamps
    for (int i = 0; i < midiCaptureBuffer_.getNumEvents(); ++i)
    {
        auto* evt = midiCaptureBuffer_.getEventPointer(i);
        clip->midiData.addEvent(evt->message);
    }
    clip->midiData.updateMatchedPairs();

    track->clips.add(clip);

    // Clear the capture buffer
    midiCaptureBuffer_.clear();
}

//==============================================================================
// Surround Panner
//==============================================================================
SurroundPanner& AudioEngine::getTrackPanner(int trackIdx)
{
    while ((int)trackPanners_.size() <= trackIdx)
        trackPanners_.emplace_back();
    return trackPanners_[(size_t)trackIdx];
}

//==============================================================================
// Track Freeze
//==============================================================================
void AudioEngine::freezeTrack(int trackIndex)
{
    if (trackIndex < 0 || trackIndex >= (int)tracks.size()) return;
    while ((int)frozenTracks_.size() <= trackIndex)
    {
        frozenTracks_.push_back(false);
        frozenBuffers_.emplace_back();
    }
    frozenTracks_[(size_t)trackIndex] = true;
    // Pre-render would happen here in a full implementation
}

void AudioEngine::unfreezeTrack(int trackIndex)
{
    if (trackIndex >= 0 && trackIndex < (int)frozenTracks_.size())
    {
        frozenTracks_[(size_t)trackIndex] = false;
        frozenBuffers_[(size_t)trackIndex] = juce::AudioBuffer<float>();
    }
}

bool AudioEngine::isTrackFrozen(int trackIndex) const
{
    if (trackIndex >= 0 && trackIndex < (int)frozenTracks_.size())
        return frozenTracks_[(size_t)trackIndex];
    return false;
}

//==============================================================================
// Delay Compensation
//==============================================================================
void AudioEngine::recalculateDelayCompensation()
{
    int numTracks = (int)tracks.size();
    trackLatencies_.resize((size_t)numTracks, 0);
    delayCompBuffers_.resize((size_t)numTracks);

    int maxLatency = 0;

    // Query each track's plugin chain for latency
    for (int i = 0; i < numTracks; ++i)
    {
        int trackLatency = 0;
        auto* track = tracks[i];
        for (auto slot : track->pluginSlots)
        {
            if (slot > 0)
            {
                auto node = processorGraph.getNodeForId(juce::AudioProcessorGraph::NodeID((uint32)slot));
                if (node && node->getProcessor())
                    trackLatency += node->getProcessor()->getLatencySamples();
            }
        }
        trackLatencies_[(size_t)i] = trackLatency;
        maxLatency = juce::jmax(maxLatency, trackLatency);
    }

    // Set compensation delays so all tracks align to the maximum latency
    for (int i = 0; i < numTracks; ++i)
    {
        int compensationDelay = maxLatency - trackLatencies_[(size_t)i];
        if (tracks[i]->faderNodeID != -1)
        {
            auto node = processorGraph.getNodeForId(juce::AudioProcessorGraph::NodeID(tracks[i]->faderNodeID));
            if (node)
            {
                if (auto* fader = dynamic_cast<TrackFaderProcessor*>(node->getProcessor()))
                {
                    fader->setDelaySamples(compensationDelay);
                }
            }
        }
    }
}

int AudioEngine::getTrackLatencySamples(int trackIndex) const
{
    if (trackIndex >= 0 && trackIndex < (int)trackLatencies_.size())
        return trackLatencies_[(size_t)trackIndex];
    return 0;
}

