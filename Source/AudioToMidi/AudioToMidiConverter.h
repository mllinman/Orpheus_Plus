#pragma once
#include <JuceHeader.h>

class AppState;

//==============================================================================
struct AudioToMidiResult
{
    juce::MidiFile midiFile;
    juce::File     midiFileOnDisk;
    double         detectedBPM = 120.0;
    int            detectedKey = 0; // 0 = C, 1 = C#, etc.
};

//==============================================================================
class AudioToMidiConverter
{
public:
    enum class Mode
    {
        Monophonic,    // CREPE - single melody line
        Polyphonic,    // Basic Pitch - chords and harmony
        Drums,         // Onset detection to MIDI drum map
        Chords,        // Chord recognition to MIDI
    };

    AudioToMidiConverter();
    ~AudioToMidiConverter();

    void setMode(Mode m)   { mode = m; }
    Mode getMode() const   { return mode; }

    void setSensitivity(float s)    { sensitivity = s; }   // 0-1
    void setMinNoteLength(float ms) { minNoteMs = ms; }    // minimum note gate
    void setVelocityFromLoudness(bool v) { velFromLoud = v; }

    void convert(const juce::File& audioFile,
                 AppState& appState,
                 std::function<void(AudioToMidiResult)> onComplete = {});

    void cancel();
    bool isRunning()    const { return running.load(); }
    float getProgress() const { return progress.load(); }

    struct Listener
    {
        virtual ~Listener() = default;
        virtual void conversionProgress(float) {}
        virtual void conversionComplete(const AudioToMidiResult&) {}
        virtual void conversionFailed(const juce::String&) {}
    };
    void addListener(Listener* l)    { listeners.add(l); }
    void removeListener(Listener* l) { listeners.remove(l); }

private:
    bool runBasicPitch(const juce::File& audio, const juce::File& outMidi);
    bool runCrepe(const juce::File& audio, const juce::File& outMidi);
    bool runOnsetDetection(const juce::File& audio, const juce::File& outMidi);

    AudioToMidiResult loadMidiResult(const juce::File& midiFile);

    Mode  mode        = Mode::Polyphonic;
    float sensitivity = 0.5f;
    float minNoteMs   = 50.0f;
    bool  velFromLoud = true;

    std::atomic<bool>  running  { false };
    std::atomic<float> progress { 0.0f  };

    juce::ListenerList<Listener> listeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioToMidiConverter)
};
