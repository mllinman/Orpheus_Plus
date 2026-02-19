#pragma once
#include <JuceHeader.h>
// #include "../StemSeparation/StemSeparator.h"
// #include "../AudioToMidi/AudioToMidiConverter.h"
// #include "../PitchCorrection/AutoTuneProcessor.h"
#include "../Timeline/Clip.h"
#include "../Timeline/AudioClip.h"
#include "../Timeline/MidiClip.h"

class TrackProcessor;
class MixerProcessor;
struct OrpheusTrackInfo;
class SpectrumAnalyzer;
class PluginManager;


//==============================================================================
// Represents a single track in the engine
struct OrpheusTrackInfo
{
    enum class Type { Audio, Midi, Bus, Master };

    juce::String name;
    juce::Colour colour;
    Type type = Type::Audio;
    int nodeID = -1;         // AudioProcessorGraph node ID
    float volume = 1.0f;
    float pan    = 0.0f;
    bool mute    = false;
    bool solo    = false;
    bool armed   = false;    // Record-armed
    juce::String inputBus;
    juce::String outputBus;

    juce::OwnedArray<Clip> clips;
};

//==============================================================================
class AudioEngine : private juce::AudioIODeviceCallback,
                    private juce::MidiInputCallback
{
public:
    AudioEngine();
    ~AudioEngine() override;

    //── Device Management ────────────────────────────────────────────────────
    void initialise();
    void shutdown();
    juce::AudioDeviceManager& getDeviceManager() { return deviceManager; }
    juce::AudioFormatManager& getFormatManager()  { return formatManager; }

    //── Playback Control ─────────────────────────────────────────────────────
    void play();
    void stop();
    void pause();
    void togglePlayback();
    void toggleRecord();
    bool isPlaying()   const { return playing.load(); }
    bool isRecording() const { return recording.load(); }

    double getPlayheadPosition() const { return playheadPosition.load(); }
    void   setPlayheadPosition(double posSeconds);
    double getBpm() const    { return bpm.load(); }
    void   setBpm(double b)  { bpm.store(b); }
    int    getTimeSigNumerator() const   { return timeSigNum; }
    int    getTimeSigDenominator() const { return timeSigDen; }
    void   setTimeSignature(int num, int den) { timeSigNum = num; timeSigDen = den; }

    //── Track Management ─────────────────────────────────────────────────────
    int  addAudioTrack(const juce::String& name = "Audio Track");
    int  addMidiTrack(const juce::String& name = "MIDI Track");
    int  addBusTrack(const juce::String& name = "Bus");
    void removeTrack(int trackIndex);
    int  getNumTracks() const;
    OrpheusTrackInfo& getTrackInfo(int index);
    const juce::OwnedArray<OrpheusTrackInfo>& getAllTracks() const { return tracks; }

    void setTrackVolume(int trackIndex, float vol);
    void setTrackPan(int trackIndex, float pan);
    void setTrackMute(int trackIndex, bool mute);
    void setTrackSolo(int trackIndex, bool solo);
    void armTrack(int trackIndex, bool armed);

    //── Plugin Graph ─────────────────────────────────────────────────────────
    juce::AudioProcessorGraph& getGraph() { return processorGraph; }
    PluginManager& getPluginManager()     { return *pluginManager; }

    //── Master Bus ───────────────────────────────────────────────────────────
    void setMasterVolume(float vol) { masterVolume.store(vol); }
    float getMasterVolume() const   { return masterVolume.load(); }

    //── AI / DSP Features ────────────────────────────────────────────────────
    // StemSeparator&       getStemSeparator()        { return *stemSeparator; }
    // AudioToMidiConverter& getAudioToMidiConverter() { return *audioToMidi; }
    // AutoTuneProcessor&   getAutoTune()             { return *autoTune; }
    // AudioCleanupProcessor& getAudioCleanup()       { return *audioCleanup; }

    //── Export ───────────────────────────────────────────────────────────────
    void exportMix(const juce::File& outputFile,
                   int sampleRate = 48000,
                   int bitDepth   = 24);
    void exportStems(const juce::File& outputDirectory);

    //── MIDI ─────────────────────────────────────────────────────────────────
    juce::MidiMessageCollector& getMidiCollector() { return midiCollector; }
    const juce::MidiBuffer& getCurrentMidiBuffer() const { return midiBuffer; }

    //── Level Metering ───────────────────────────────────────────────────────
    float getMasterPeakLeft()  const { return masterPeakL.load(); }
    float getMasterPeakRight() const { return masterPeakR.load(); }
    float getMasterLUFS()      const { return currentLUFS.load(); }

    //── Undo/Redo ────────────────────────────────────────────────────────────
    juce::UndoManager& getUndoManager() { return undoManager; }

    // Listeners
    struct Listener
    {
        virtual ~Listener() = default;
        virtual void playbackStarted() {}
        virtual void playbackStopped() {}
        virtual void trackListChanged() {}
        virtual void bpmChanged(double) {}
    };
    void addListener(Listener* l)    { listeners.add(l); }
    void removeListener(Listener* l) { listeners.remove(l); }

    void registerAnalyzer(SpectrumAnalyzer* analyzer);
    void unregisterAnalyzer(SpectrumAnalyzer* analyzer);


private:
    //── AudioIODeviceCallback ────────────────────────────────────────────────
    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                          int numInputChannels,
                                          float* const* outputChannelData,
                                          int numOutputChannels,
                                          int numSamples,
                                          const juce::AudioIODeviceCallbackContext&) override;
    void audioDeviceAboutToStart(juce::AudioIODevice*) override;
    void audioDeviceStopped() override;

    //── MidiInputCallback ────────────────────────────────────────────────────
    void handleIncomingMidiMessage(juce::MidiInput*, const juce::MidiMessage&) override;

    void updateSoloState();
    void processAudioBlock(juce::AudioBuffer<float>& buffer);

    //── Core ─────────────────────────────────────────────────────────────────
    juce::AudioDeviceManager deviceManager;
    juce::AudioFormatManager  formatManager;
    juce::AudioProcessorGraph processorGraph;

    using Node = juce::AudioProcessorGraph::Node;
    Node::Ptr inputNode;
    Node::Ptr outputNode;
    Node::Ptr masterNode;
    juce::MidiMessageCollector midiCollector;
    juce::MidiBuffer           midiBuffer;
    juce::UndoManager          undoManager { 50 };

    juce::OwnedArray<OrpheusTrackInfo> tracks;
    std::unique_ptr<PluginManager>         pluginManager;
    // std::unique_ptr<StemSeparator>         stemSeparator;
    // std::unique_ptr<AudioToMidiConverter>  audioToMidi;
    // std::unique_ptr<AutoTuneProcessor>     autoTune;
    // std::unique_ptr<AudioCleanupProcessor> audioCleanup;

    // Transport state (lock-free)
    std::atomic<bool>   playing   { false };
    std::atomic<bool>   recording { false };
    std::atomic<double> playheadPosition { 0.0 };
    std::atomic<double> bpm             { 120.0 };
    int timeSigNum = 4, timeSigDen = 4;

    // Metering
    std::atomic<float> masterPeakL { 0.0f };
    std::atomic<float> masterPeakR { 0.0f };
    std::atomic<float> currentLUFS { -70.0f };
    std::atomic<float> masterVolume { 1.0f };

    double currentSampleRate = 44100.0;
    int    currentBlockSize  = 512;

    juce::ListenerList<Listener> listeners;
    juce::Array<SpectrumAnalyzer*> analyzers;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngine)
};
