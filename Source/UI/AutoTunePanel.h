#pragma once
#include <JuceHeader.h>
#include "../PitchCorrection/VocalSuiteProcessor.h"
#include "../Audio/AudioEngine.h"
#include "OrpheusLookAndFeel.h"

//==============================================================================
class AutoTunePanel : public juce::Component,
                       private juce::Timer
{
public:
    AutoTunePanel(AudioEngine& engine);
    ~AutoTunePanel() override;

    VocalSuiteProcessor& getProcessor() { return processor; }

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void paintPitchMeter(juce::Graphics& g, juce::Rectangle<int> bounds);
    void paintMiniKeyboard(juce::Graphics& g, juce::Rectangle<int> bounds);
    bool isNoteInScale(int noteInOctave) const;

    AudioEngine& audioEngine;
    VocalSuiteProcessor processor;

    // Master
    juce::ToggleButton enableToggle { "ENABLE" };
    juce::ToggleButton bypassToggle { "BYPASS" };
    juce::ToggleButton neuralModeToggle { "NEURAL INTENT" };

    // ── Key / Scale ──
    juce::Label    keyLabel   { {}, "KEY" };
    juce::ComboBox keyCombo;
    juce::Label    scaleLabel { {}, "SCALE" };
    juce::ComboBox scaleCombo;

    struct ParameterControl {
        juce::Slider knob;
        juce::Label label;
        juce::Label readout;
    };

    ParameterControl pitchCtrl;
    ParameterControl volumeCtrl;
    ParameterControl toneCtrl;
    ParameterControl paceCtrl;
    ParameterControl rhythmCtrl;
    ParameterControl articulationCtrl;
    ParameterControl resonanceCtrl;
    ParameterControl inflectionCtrl;
    ParameterControl emphasisCtrl;
    ParameterControl projectionCtrl;

    // Real-time display state
    float detectedPitch  = 0.0f;
    float correctedPitch = 0.0f;
    float centDeviation  = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutoTunePanel)
};
