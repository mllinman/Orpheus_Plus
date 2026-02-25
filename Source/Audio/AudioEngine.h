#pragma once
#include <JuceHeader.h>
#include <vector>
#include <memory>
#include <atomic>
#include <array>
#include "../StemSeparation/StemSeparator.h"
#include "../AudioToMidi/AudioToMidiConverter.h"
// #include "../PitchCorrection/AutoTuneProcessor.h"
#include "../Timeline/Clip.h"
#include "../Timeline/AudioClip.h"
#include "../Timeline/MidiClip.h"

class TrackProcessor;
class MixerProcessor;
struct OrpheusTrackInfo;
class SpectrumAnalyzer;
class PluginManager;
class MidiLearnManager;


//==============================================================================
struct AutomationPoint
{
    double time;  // Seconds
    float value;  // Normalized 0.0 - 1.0 (or mapped value)

    bool operator<(const AutomationPoint& other) const { return time < other.time; }
};

struct AutomationCurve
{
    juce::String parameterID; // e.g., "vol", "pan"
    std::vector<AutomationPoint> points;
    bool active = false;

    void addPoint(double time, float value)
    {
        // Simple insert sorted
        points.push_back({time, value});
        std::sort(points.begin(), points.end());
    }
};

//==============================================================================
// Represents a single track in the engine
struct OrpheusTrackInfo
{
    enum class Type { Audio, Midi, Bus, Master };

    juce::String name;
    juce::Colour colour;
    Type type = Type::Audio;
    int generatorNodeID = -1;  // Clip rendering node
    int faderNodeID     = -1;  // Volume/Pan/Meter node
    
    float volume = 1.0f;
    float pan    = 0.0f;
    float sweetener = 0.0f;
    bool mute    = false;
    bool solo    = false;
    bool armed   = false;    // Record-armed
    juce::String inputBus;
    juce::String outputBus;
    
    static constexpr int MAX_PLUGINS = 8;
    std::array<int, MAX_PLUGINS> pluginSlots; 
    std::array<int, MAX_PLUGINS> sidechainSources; // Track index providing sidechain

    struct Take
    {
        juce::String name;
        juce::OwnedArray<Clip> clips;
    };

    juce::OwnedArray<Clip> clips; 
    juce::OwnedArray<Take> takes; 
    bool showTakes = false;       
    
    std::vector<AutomationCurve> automationCurves;
    std::vector<juce::String> visibleAutomationLanes; // e.g. "vol", "pan", "sweet"
    
    OrpheusTrackInfo() { 
        pluginSlots.fill(-1); 
        sidechainSources.fill(-1);
    }
};

//==============================================================================
class AudioEngine : private juce::AudioIODeviceCallback,
                    private juce::MidiInputCallback
{
    friend class PluginManager;
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
    bool isExporting() const { return exporting.load(); }

    double getPlayheadPosition() const { return playheadPosition.load(); }
    void   setPlayheadPosition(double posSeconds);
    double getBpm() const    { return bpm.load(); }
    void   setBpm(double b)  { bpm.store(b); }
    int    getTimeSigNumerator() const   { return timeSigNum; }
    int    getTimeSigDenominator() const { return timeSigDen; }
    void   setTimeSignature(int num, int den) { timeSigNum = num; timeSigDen = den; }

    //── Track Management ─────────────────────────────────────────────────────
    void syncWithAppState(class AppState& state);
    int  addAudioTrack(const juce::String& name = "Audio Track");
    int  addMidiTrack(const juce::String& name = "MIDI Track");
    int  addBusTrack(const juce::String& name = "Bus");
    void removeTrack(int trackIndex);
    void moveTrack(int fromIndex, int toIndex);
    int  getNumTracks() const;
    OrpheusTrackInfo& getTrackInfo(int index);
    const juce::OwnedArray<OrpheusTrackInfo>& getAllTracks() const { return tracks; }

    void setTrackVolume(int trackIndex, float vol);
    void setTrackPan(int trackIndex, float pan);
    void setTrackSweetener(int trackIndex, float amount);
    void setSidechainSource(int targetTrack, int slot, int sourceTrack);
    void setTrackMute(int trackIndex, bool mute);
    void setTrackSolo(int trackIndex, bool solo);
    void armTrack(int trackIndex, bool armed);

    //── Plugin Graph ─────────────────────────────────────────────────────────
    juce::AudioProcessorGraph& getGraph() { return processorGraph; }
    PluginManager& getPluginManager()     { return *pluginManager; }
    void updateTrackGraphConnections(int trackIndex);

    MidiLearnManager& getMidiLearn() { return *midiLearnManager; }

    //── Master Bus ───────────────────────────────────────────────────────────
    void setMasterVolume(float vol) { masterVolume.store(vol); }
    float getMasterVolume() const   { return masterVolume.load(); }

    //── AI / DSP Features ────────────────────────────────────────────────────
    void addAutoTuneToTrack(int trackIndex);
    void addAudioCleanupToTrack(int trackIndex);
    
    StemSeparator&       getStemSeparator()        { return *stemSeparator; }
    AudioToMidiConverter& getAudioToMidiConverter() { return *audioToMidi; }

    //── Export ───────────────────────────────────────────────────────────────
    void exportMix(const juce::File& outputFile,
                   int sampleRate = 48000,
                   int bitDepth   = 24);
    void exportStems(const juce::File& outputDirectory);

    //── MIDI ─────────────────────────────────────────────────────────────────
    juce::MidiMessageCollector& getMidiCollector() { return midiCollector; }
    const juce::MidiBuffer& getCurrentMidiBuffer() const { return midiBuffer; }

    //── Mono Sum ─────────────────────────────────────────────────────────────
    void setMonoSum(bool shouldSum) { monoSum.store(shouldSum); }
    bool getMonoSum() const { return monoSum.load(); }

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
    std::unique_ptr<MidiLearnManager>      midiLearnManager;
    std::unique_ptr<PluginManager>         pluginManager;
    std::unique_ptr<StemSeparator>         stemSeparator;
    std::unique_ptr<AudioToMidiConverter>  audioToMidi;
    // std::unique_ptr<AutoTuneProcessor>     autoTune;
    // std::unique_ptr<AudioCleanupProcessor> audioCleanup;

    std::atomic<bool>   playing   { false };
    std::atomic<bool>   recording { false };
    std::atomic<bool>   exporting { false };
    std::atomic<double> playheadPosition { 0.0 };
    std::atomic<double> bpm             { 120.0 };
    int timeSigNum = 4, timeSigDen = 4;

    // Metering
    std::atomic<float> masterPeakL { 0.0f };
    std::atomic<float> masterPeakR { 0.0f };
    std::atomic<bool>  monoSum { false };
    std::atomic<float> currentLUFS { -70.0f };
    std::atomic<float> masterVolume { 1.0f };

    // Record Setup
    juce::TimeSliceThread audioWriterThread { "Audio Recording Thread" };
    // Recording Buffers
    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> audioWriter;
    juce::File currentRecordingFile;
    int armedTrackIndex = -1;
    juce::MidiMessageSequence recordedMidi;

    double currentSampleRate = 44100.0;
    int    currentBlockSize  = 512;

    juce::ListenerList<Listener> listeners;
    juce::Array<SpectrumAnalyzer*> analyzers;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngine)
};
