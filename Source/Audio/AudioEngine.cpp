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
    midiLearnManager = std::make_unique<MidiLearnManager>(*this);
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
    
    auto vt = state.getValueTree();
    for (int i = 0; i < vt.getNumChildren(); ++i)
    {
        auto child = vt.getChild(i);
        if (child.hasType("Track"))
        {
            juce::String type = child.getProperty("type");
            juce::String name = child.getProperty("name");
            int idx = -1;
            
            if (type == "audio") idx = addAudioTrack(name);
            else if (type == "midi") idx = addMidiTrack(name);
            
            if (idx >= 0)
            {
                setTrackVolume(idx, static_cast<float>(child.getProperty("vol", 1.0f)));
                setTrackPan(idx, static_cast<float>(child.getProperty("pan", 0.0f)));
                setTrackMute(idx, static_cast<bool>(child.getProperty("mute", false)));
                setTrackSolo(idx, static_cast<bool>(child.getProperty("solo", false)));
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
    midiLearnManager->handleIncomingMidi(message);
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
    // We iterate through all nodes and remove any connections that start AND end within this chain.
    for (const auto& conn : processorGraph.getConnections())
    {
        bool sourceInChain = std::find(chain.begin(), chain.end(), conn.source.nodeID) != chain.end();
        bool destInChain = std::find(chain.begin(), chain.end(), conn.destination.nodeID) != chain.end();

        if (sourceInChain && destInChain)
            processorGraph.removeConnection(conn);
    }

    // 3. Connect them in the new linear order
    for (size_t i = 0; i < chain.size() - 1; ++i)
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            processorGraph.addConnection({ { chain[i], ch }, { chain[i+1], ch } });
        }
    }

    // 4. Ensure Fader -> Master connection exists
    if (track->faderNodeID != -1 && masterNode)
    {
        juce::AudioProcessorGraph::NodeID faderID(track->faderNodeID);
        for (int ch = 0; ch < 2; ++ch)
        {
            processorGraph.addConnection({ { faderID, ch }, { masterNode->nodeID, ch } });
        }
    }
}
