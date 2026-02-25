#pragma once
#include <JuceHeader.h>
#include "../PitchCorrection/AutoTuneProcessor.h"
#include "../Audio/AudioEngine.h"
#include "OrpheusLookAndFeel.h"

//==============================================================================
class AutoTunePanel : public juce::Component,
                       private juce::Timer
{
public:
    AutoTunePanel(AudioEngine& engine);
    ~AutoTunePanel() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void paintPitchMeter(juce::Graphics& g, juce::Rectangle<int> bounds);
    void paintMiniKeyboard(juce::Graphics& g, juce::Rectangle<int> bounds);
    bool isNoteInScale(int noteInOctave) const;

    AudioEngine& audioEngine;
    AutoTuneProcessor processor;

    // Master
    juce::ToggleButton enableToggle { "ENABLE" };
    juce::ToggleButton bypassToggle { "BYPASS" };

    // ── Key / Scale ──
    juce::Label    keyLabel   { {}, "KEY" };
    juce::ComboBox keyCombo;
    juce::Label    scaleLabel { {}, "SCALE" };
    juce::ComboBox scaleCombo;

    // ── Speed (correction speed) ──
    juce::Slider speedKnob;
    juce::Label  speedLabel { {}, "SPEED" };
    juce::Label  speedReadout;

    // ── Formant Shift ──
    juce::Slider formantKnob;
    juce::Label  formantLabel { {}, "FORMANT" };
    juce::Label  formantReadout;

    // ── Robot Voice ──
    juce::Slider robotKnob;
    juce::Label  robotLabel { {}, "ROBOT" };
    juce::Label  robotReadout;

    // Real-time display state
    float detectedPitch  = 0.0f;
    float correctedPitch = 0.0f;
    float centDeviation  = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutoTunePanel)
};
