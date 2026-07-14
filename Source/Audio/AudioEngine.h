#pragma once
#include <JuceHeader.h>
#include <vector>
#include <memory>
#include <atomic>
#include <array>
#include <cmath>
#include <algorithm>
#include "../StemSeparation/StemSeparator.h"
#include "../AudioToMidi/AudioToMidiConverter.h"
// #include "../PitchCorrection/AutoTuneProcessor.h"
#include "MidiLearnManager.h"
#include "ModulationSource.h"
#include "TempoFollower.h"
#include "SurroundPanner.h"
#include "../UI/LoudnessMeter.h"
#include "../Timeline/Clip.h"
#include "../Timeline/AudioClip.h"
#include "../Timeline/MidiClip.h"

class TrackProcessor;
class MixerProcessor;
struct OrpheusTrackInfo;
class SpectrumAnalyzer;
class PluginManager;
class MidiLearnManager;
class MasteringModule;
class VocalSuiteProcessor;


//==============================================================================
struct AutomationPoint
{
    double time;  // Seconds
    float value;  // Normalized 0.0 - 1.0 (or mapped value)

    bool operator<(const AutomationPoint& other) const { return time < other.time; }
};

struct AutomationCurve
{
    juce::String parameterID; // e.g., "vol", "pan", "pitch", "resonance"
    std::vector<AutomationPoint> points;
    bool active = false;

    void addPoint(double time, float value)
    {
        // For high-resolution, we can just push back and sort later if we know it's append-only
        // Or we can find and update if time is very close
        auto it = std::lower_bound(points.begin(), points.end(), AutomationPoint{time, 0.0f});
        if (it != points.end() && std::abs(it->time - time) < 0.001) {
            it->value = value;
        } else {
            points.insert(it, {time, value});
        }
    }

    void removePointsInRange(double startTime, double endTime)
    {
        points.erase(
            std::remove_if(points.begin(), points.end(),
                [=](const AutomationPoint& p) { return p.time >= startTime && p.time <= endTime; }),
            points.end());
    }

    void smoothPointsInRange(double startTime, double endTime, int windowSize = 5)
    {
        if (points.size() < (size_t)windowSize) return;
        std::vector<float> smoothed(points.size());
        for (size_t i = 0; i < points.size(); ++i) {
            if (points[i].time >= startTime && points[i].time <= endTime) {
                float sum = 0.0f;
                int count = 0;
                for (int j = -(windowSize/2); j <= (windowSize/2); ++j) {
                    if (i + j >= 0 && i + j < points.size()) {
                        sum += points[i + j].value;
                        count++;
                    }
                }
                smoothed[i] = sum / (float)count;
            } else {
                smoothed[i] = points[i].value;
            }
        }
        for (size_t i = 0; i < points.size(); ++i) {
            if (points[i].time >= startTime && points[i].time <= endTime) {
                points[i].value = smoothed[i];
            }
        }
    }
};

//==============================================================================
// Represents a single track in the engine
struct OrpheusTrackInfo
{
    enum class Type { Audio, Midi, Bus, Folder, Arranger, Master, Chord };

    juce::String name;
    juce::Colour colour;
    Type type = Type::Audio;
    int generatorNodeID = -1;  // Clip rendering node
    int faderNodeID     = -1;  // Volume/Pan/Meter node
    
    float volume = 1.0f;
    float pan    = 0.0f;
    float sweetener = 0.0f;
    bool  spatialEnabled = false;
    float panAzimuth = 0.0f;
    float panElevation = 0.0f;
    float panDistance = 1.0f;
    bool mute    = false;
    bool solo    = false;
    bool isMutedBySolo = false;
    bool armed   = false;    // Record-armed
    bool expanded = true;    // Folder state
    bool visible  = true;    // Hierarchy visibility
    int  depth   = 0;        // Nesting level
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
    void finalizeRecording();
    bool isPlaying()   const { return playing.load(); }
    bool isRecording() const { return recording.load(); }
    bool isExporting() const { return exporting.load(); }
    void setExporting(bool e) { exporting.store(e); }

    void setLooping(bool loop) { looping.store(loop); }
    bool isLooping() const { return looping.load(); }

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
    int  addChordTrack(const juce::String& name = "Chord Track");
    int  addBusTrack(const juce::String& name = "Bus");
    int  addFolderTrack(const juce::String& name = "Folder");
    int  addArrangerTrack(const juce::String& name = "Arranger");
    void removeTrack(int trackIndex);
    void moveTrack(int fromIndex, int toIndex);
    int  getNumTracks() const;
    OrpheusTrackInfo& getTrackInfo(int index);
    const juce::OwnedArray<OrpheusTrackInfo>& getAllTracks() const { return tracks; }

    void setTrackVolume(int trackIndex, float vol);
    void setTrackPan(int trackIndex, float pan);
    void setTrackSpatialMode(int trackIndex, bool enabled);
    void setTrackSpatialPosition(int trackIndex, float azimuth, float elevation, float distance);
    void setTrackSweetener(int trackIndex, float amount);
    void setSidechainSource(int targetTrack, int slot, int sourceTrack);
    void setTrackMute(int trackIndex, bool mute);
    void setTrackSolo(int trackIndex, bool solo);
    void armTrack(int trackIndex, bool armed);
    
    // High-resolution Automation
    void recordAutomationPoint(int trackIndex, const juce::String& paramID, double time, float value);
    void deleteAutomationRange(int trackIndex, const juce::String& paramID, double startTime, double endTime);
    void smoothAutomationRange(int trackIndex, const juce::String& paramID, double startTime, double endTime);

    //── Plugin Graph ─────────────────────────────────────────────────────────
    juce::AudioProcessorGraph& getGraph() { return processorGraph; }
    PluginManager& getPluginManager()     { return *pluginManager; }
    void updateTrackGraphConnections(int trackIndex);

    MidiLearnManager& getMidiLearn() { return *midiLearnManager; }

    //── Master Bus ───────────────────────────────────────────────────────────
    void setMasterVolume(float vol) { masterVolume.store(vol); }
    float getMasterVolume() const   { return masterVolume.load(); }

    //── AI / DSP Features ────────────────────────────────────────────────────
    void addVocalSuiteToTrack(int trackIndex);
    VocalSuiteProcessor* getVocalSuiteForTrack(int trackIndex);
    void addAudioCleanupToTrack(int trackIndex);
    void alignAllTracksPhase(); // Global Phase Align
    void alignTrackPhase(int trackIndex, int referenceTrackIndex); // Track-specific Phase Align
    
    StemSeparator&       getStemSeparator()        { return *stemSeparator; }
    AudioToMidiConverter& getAudioToMidiConverter() { return *audioToMidi; }

    void setMasteringModule(MasteringModule* m);
    MasteringModule* getMasteringModule() const;

    //── Export ───────────────────────────────────────────────────────────────
    void exportMix(const juce::File& outputFile,
                   int sampleRate = 48000,
                   int bitDepth   = 24);
    void exportStems(const juce::File& outputDirectory);

    // Track Getters
    int getTrackCount() const { return tracks.size(); }
    OrpheusTrackInfo* getTrack(int index) const 
    { 
        if (index >= 0 && index < tracks.size()) return tracks[index]; 
        return nullptr; 
    }

    //── MIDI ─────────────────────────────────────────────────────────────────
    juce::MidiMessageCollector& getMidiCollector() { return midiCollector; }
    const juce::MidiBuffer& getCurrentMidiBuffer() const { return midiBuffer; }

    enum class ScaleLock { Off, Major, Minor, Pentatonic, Blues };
    void setScaleLock(ScaleLock scale, int rootNote) { 
        scaleLock_ = scale; 
        scaleRoot_ = rootNote; 
    }
    ScaleLock getScaleLock() const { return scaleLock_; }
    int getScaleRoot() const { return scaleRoot_; }

    //── MIDI Capture (Retroactive Recording) ─────────────────────────────────
    void captureMidi(int trackIndex);
    void setMidiCaptureEnabled(bool e) { midiCaptureEnabled_.store(e); }
    bool isMidiCaptureEnabled() const  { return midiCaptureEnabled_.load(); }
    int  getMidiCaptureSeconds() const { return midiCaptureSeconds_; }
    void setMidiCaptureSeconds(int s)  { midiCaptureSeconds_ = s; }

    //── Tempo Follower ───────────────────────────────────────────────────────
    TempoFollower& getTempoFollower() { return tempoFollower_; }

    //── Modulation System ────────────────────────────────────────────────────
    std::vector<LFOSource>&            getLFOs()       { return lfoSources_; }
    std::vector<ModulationMapping>&    getModMappings(){ return modMappings_; }
    int addLFO() { lfoSources_.emplace_back(); return (int)lfoSources_.size() - 1; }

    //── Routing ──────────────────────────────────────────────────────────────
    struct RoutingPoint { int srcTrack; int srcCh; int dstTrack; int dstCh; float gain = 1.0f; };
    std::vector<RoutingPoint>& getRoutingPoints() { return routingPoints_; }
    void addRoutingPoint(RoutingPoint rp) { routingPoints_.push_back(rp); }

    //── Surround / Atmos ─────────────────────────────────────────────────────
    SurroundPanner& getTrackPanner(int trackIdx);

    //── Track Freeze ─────────────────────────────────────────────────────────
    void freezeTrack(int trackIndex);
    void unfreezeTrack(int trackIndex);
    bool isTrackFrozen(int trackIndex) const;

    //── Delay Compensation ───────────────────────────────────────────────────
    void recalculateDelayCompensation();
    int  getTrackLatencySamples(int trackIndex) const;

    //── Zero-Latency Engine ──────────────────────────────────────────────────
    void setPredictiveBuffering(bool enabled) { usePredictiveBuffering.store(enabled); }
    bool isPredictiveBuffering() const { return usePredictiveBuffering.load(); }

    //── Pro-Level Metering ───────────────────────────────────────────────────
    LoudnessMeter& getMasterMeter() { return masterMeter_; }

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
    std::atomic<bool>   looping   { false };
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
    juce::CriticalSection analyzerLock;
    juce::Array<SpectrumAnalyzer*> analyzers;

    // ── MIDI Capture Ring Buffer ──
    juce::MidiMessageSequence midiCaptureBuffer_;
    std::atomic<bool>         midiCaptureEnabled_ { true };
    int                       midiCaptureSeconds_ = 30;

    // ── Tempo Follower ──
    TempoFollower tempoFollower_;

    // ── Modulation ──
    std::vector<LFOSource>          lfoSources_;
    std::vector<ModulationMapping>  modMappings_;

    // ── Routing ──
    std::vector<RoutingPoint>       routingPoints_;

    // ── Surround Panners ──
    std::vector<SurroundPanner>     trackPanners_;

    // ── Zero-Latency Engine ──
    std::atomic<bool> usePredictiveBuffering { false };
    juce::AudioBuffer<float> predictiveBuffer;
    bool predictiveBufferReady { false };

    // ── Pro Meter ──
    LoudnessMeter masterMeter_;

    // ── Track Freeze ──
    std::vector<bool> frozenTracks_;
    
    // ── Lock-Free MIDI Capture ──
    juce::AbstractFifo lockFreeMidiFifo { 1024 };
    std::array<juce::uint8, 4096> lockFreeMidiData; // Pre-allocated ring buffer

    ScaleLock scaleLock_ { ScaleLock::Off };
    int scaleRoot_ { 0 };
    std::vector<int> trackLatencies_;

    std::vector<juce::AudioBuffer<float>> frozenBuffers_;
    std::vector<juce::AudioBuffer<float>> delayCompBuffers_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngine)
};
