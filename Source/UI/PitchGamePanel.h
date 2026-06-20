#pragma once
#include <JuceHeader.h>
#include "../PitchCorrection/VocalSuiteProcessor.h"
#include "../Audio/AudioEngine.h"
#include "OrpheusLookAndFeel.h"

//==============================================================================
// PitchGamePanel — Real-time vocal pitch training system
// Shows detected pitch vs. correct key/scale target with scrolling display,
// green/red color coding, key/scale selectors, scale degree piano,
// session scoring, and a toggle switch with activity LED.
//==============================================================================
class PitchGamePanel : public juce::Component,
                       private juce::Timer
{
public:
    PitchGamePanel(AudioEngine& engine);
    ~PitchGamePanel() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;

    void setProcessor(VocalSuiteProcessor* proc) { processor = proc; }

private:
    void timerCallback() override;

    // Drawing helpers
    void paintBackground(juce::Graphics& g, juce::Rectangle<int> bounds);
    void paintHeaderBar(juce::Graphics& g, juce::Rectangle<int> bounds);
    void paintKeyInfoBar(juce::Graphics& g, juce::Rectangle<int> bounds);
    void paintScalePiano(juce::Graphics& g, juce::Rectangle<int> bounds);
    void paintPitchLadder(juce::Graphics& g, juce::Rectangle<int> bounds);
    void paintPitchIndicator(juce::Graphics& g, juce::Rectangle<int> bounds);
    void paintNoteLabels(juce::Graphics& g, juce::Rectangle<int> bounds);
    void paintAccuracyMeter(juce::Graphics& g, juce::Rectangle<int> bounds);
    void paintSessionScore(juce::Graphics& g, juce::Rectangle<int> bounds);

    // Pitch utilities
    static juce::String midiNoteToName(int midiNote);
    static juce::String noteNameOnly(int noteInOctave);
    static float hzToMidiNote(float hz);
    float getClosestTargetNote(float detectedMidi) const;
    bool isNoteInScale(int noteInOctave) const;
    int getScaleDegree(int noteInOctave) const;
    void buildScaleNotes();

    AudioEngine& audioEngine;
    VocalSuiteProcessor* processor = nullptr;

    // Key/Scale selectors (child components)
    juce::ComboBox keyCombo;
    juce::ComboBox scaleCombo;
    juce::Label keyLabel    { {}, "KEY" };
    juce::Label scaleLabel  { {}, "SCALE" };

    // Toggle state
    bool gameActive = false;
    juce::Rectangle<int> switchBounds;

    // Pitch tracking history for scrolling display
    struct PitchSample {
        float detectedMidi = 0.0f;
        float targetMidi   = 0.0f;
        float centError    = 0.0f;
        bool  hasVoice     = false;
    };

    static constexpr int HISTORY_SIZE = 150;
    std::array<PitchSample, HISTORY_SIZE> pitchHistory;
    int historyWriteIndex = 0;

    // Smoothed display values
    float smoothedDetectedMidi = 0.0f;
    float smoothedCentError    = 0.0f;
    float smoothedAccuracy     = 0.0f;

    // LED glow animation
    float ledGlow = 0.0f;
    float indicatorGlow = 0.0f;

    // Scale/key state
    int currentKey   = 0;   // 0=C, 1=C#, ... 11=B
    int currentScale = 1;   // 0=Chromatic, 1=Major, 2=Minor

    // Precomputed scale notes (MIDI note-in-octave values in the current key)
    std::vector<int> scaleNoteList;  // e.g. for C Major: {0, 2, 4, 5, 7, 9, 11}

    // Session scoring
    int   sessionTotalSamples = 0;
    float sessionAccumAccuracy = 0.0f;
    int   streakCount = 0;
    int   bestStreak  = 0;
    float sessionScore = 0.0f;  // Rolling average accuracy

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchGamePanel)
};
