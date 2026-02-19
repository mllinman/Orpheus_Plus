#include "AudioEngine.h"
#include "PluginManager.h"
#include "TrackProcessor.h"
#include "MixerProcessor.h"
// #include "../UI/SpectrumAnalyzer.h"


AudioEngine::AudioEngine()
{
    formatManager.registerBasicFormats();

    pluginManager  = std::make_unique<PluginManager>(*this);
    // stemSeparator  = std::make_unique<StemSeparator>();
    // audioToMidi    = std::make_unique<AudioToMidiConverter>();
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
    recording.store(false);
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
    if (!playing.load()) play();
    recording.store(!recording.load());
}

void AudioEngine::setPlayheadPosition(double posSeconds)
{
    playheadPosition.store(juce::jmax(0.0, posSeconds));
}

//──────────────────────────────────────────────────────────────────────────────
// Track management
//──────────────────────────────────────────────────────────────────────────────
int AudioEngine::addAudioTrack(const juce::String& name)
{
    OrpheusTrackInfo info;
    info.name   = name;
    info.type   = OrpheusTrackInfo::Type::Audio;
    info.colour = juce::Colours::cornflowerblue.withSaturation(0.7f);

    // Create Track Processor
    auto trackProc = std::make_unique<TrackProcessor>();
    auto node = processorGraph.addNode(std::move(trackProc));
    info.nodeID = (int)node->nodeID.uid;

    // Connect Track -> Master
    if (node && masterNode)
    {
        for (int ch = 0; ch < 2; ++ch)
            processorGraph.addConnection({ { node->nodeID, ch }, { masterNode->nodeID, ch } });
    }

    tracks.add(info);

    juce::MessageManager::callAsync([this]{ listeners.call(&Listener::trackListChanged); });
    return tracks.size() - 1;
}

int AudioEngine::addMidiTrack(const juce::String& name)
{
    OrpheusTrackInfo info;
    info.name   = name;
    info.type   = OrpheusTrackInfo::Type::Midi;
    info.colour = juce::Colours::mediumpurple;
    tracks.add(info);

    juce::MessageManager::callAsync([this]{ listeners.call(&Listener::trackListChanged); });
    return tracks.size() - 1;
}

int AudioEngine::addBusTrack(const juce::String& name)
{
    OrpheusTrackInfo info;
    info.name   = name;
    info.type   = OrpheusTrackInfo::Type::Bus;
    info.colour = juce::Colours::darkorange;
    tracks.add(info);

    juce::MessageManager::callAsync([this]{ listeners.call(&Listener::trackListChanged); });
    return tracks.size() - 1;
}

void AudioEngine::removeTrack(int index)
{
    if (juce::isPositiveAndBelow(index, tracks.size()))
    {
        auto& track = tracks.getReference(index);
        if (track.nodeID != -1)
            processorGraph.removeNode(juce::AudioProcessorGraph::NodeID(track.nodeID));

        tracks.remove(index);
        listeners.call(&Listener::trackListChanged);
    }
}

int AudioEngine::getNumTracks() const { return tracks.size(); }

OrpheusTrackInfo& AudioEngine::getTrackInfo(int index) { return tracks.getReference(index); }

void AudioEngine::setTrackVolume(int i, float vol) { tracks.getReference(i).volume = vol; }
void AudioEngine::setTrackPan(int i, float pan)    { tracks.getReference(i).pan    = pan; }
void AudioEngine::setTrackMute(int i, bool mute)   { tracks.getReference(i).mute   = mute; updateSoloState(); }
void AudioEngine::setTrackSolo(int i, bool solo)   { tracks.getReference(i).solo   = solo; updateSoloState(); }
void AudioEngine::armTrack(int i, bool armed)      { tracks.getReference(i).armed  = armed; }

void AudioEngine::updateSoloState()
{
    bool anySolo = false;
    for (auto& t : tracks)
        if (t.solo) { anySolo = true; break; }

    for (auto& t : tracks)
        t.mute = anySolo ? !t.solo : false;
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
    processAudioBlock(buffer);

    // Advance playhead
    double advance = numSamples / currentSampleRate;
    playheadPosition.fetch_add(advance);

    // Update meters
    if (numOutputChannels >= 1)
        masterPeakL.store(buffer.getMagnitude(0, 0, numSamples) * masterVolume.load());
    if (numOutputChannels >= 2)
        masterPeakR.store(buffer.getMagnitude(1, 0, numSamples) * masterVolume.load());
}

void AudioEngine::processAudioBlock(juce::AudioBuffer<float>& buffer)
{
    // Collect MIDI
    midiBuffer.clear();
    midiCollector.removeNextBlockOfMessages(midiBuffer, buffer.getNumSamples());

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
    midiCollector.addMessageToQueue(message);
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
    // Offline render: collect all clips, process through full graph, write to file
    juce::WavAudioFormat wavFormat;
    auto stream = outputFile.createOutputStream();
    if (!stream) return;

    std::unique_ptr<juce::AudioFormatWriter> writer(
        wavFormat.createWriterFor(stream.release(), sampleRate, 2, bitDepth, {}, 0));

    if (!writer) return;

    const int blockSize = 2048;
    juce::AudioBuffer<float> buffer(2, blockSize);
    const double totalDuration = 180.0; // placeholder - should derive from clip lengths
    const int64_t totalSamples = (int64_t)(totalDuration * sampleRate);

    processorGraph.prepareToPlay(sampleRate, blockSize);

    for (int64_t pos = 0; pos < totalSamples; pos += blockSize)
    {
        int thisBlock = (int)juce::jmin((int64_t)blockSize, totalSamples - pos);
        buffer.clear();
        // TODO: fill buffer from timeline clips at position pos
        writer->writeFromAudioSampleBuffer(buffer, 0, thisBlock);
    }

    processorGraph.releaseResources();
}

void AudioEngine::exportStems(const juce::File& outputDirectory)
{
    outputDirectory.createDirectory();
    for (int i = 0; i < tracks.size(); ++i)
    {
        auto stemFile = outputDirectory.getChildFile(
            juce::File::createLegalFileName(tracks[i].name) + ".wav");
        exportMix(stemFile); // TODO: solo individual track
    }
}
