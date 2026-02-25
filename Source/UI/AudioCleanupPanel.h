#pragma once
#include <JuceHeader.h>
#include "../AudioCleanup/AudioCleanupProcessor.h"
#include "../Audio/AudioEngine.h"
#include "OrpheusLookAndFeel.h"

//==============================================================================
class AudioCleanupPanel : public juce::Component,
                           private juce::Timer
{
public:
    AudioCleanupPanel(AudioEngine& engine);
    ~AudioCleanupPanel() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void paintModuleCard(juce::Graphics& g, juce::Rectangle<int> bounds,
                         const juce::String& name, bool enabled);

    AudioEngine& audioEngine;
    AudioCleanupProcessor processor;

    // Master
    juce::ToggleButton masterBypass { "BYPASS" };

    // ── Noise Reduction ──
    juce::ToggleButton noiseEnable    { "Noise Reduction" };
    juce::Slider       noiseAmount;
    juce::Label        noiseAmountLabel { {}, "AMOUNT" };
    juce::Slider       noiseGateThresh;
    juce::Label        noiseGateLabel   { {}, "GATE" };
    juce::TextButton   learnNoiseBtn    { "Learn Noise Profile" };

    // ── De-Click ──
    juce::ToggleButton deClickEnable   { "De-Click" };
    juce::Slider       deClickSensitivity;
    juce::Label        deClickLabel     { {}, "SENSITIVITY" };

    // ── De-Esser ──
    juce::ToggleButton deEsserEnable   { "De-Esser" };
    juce::Slider       deEsserFreq;
    juce::Label        deEsserFreqLabel { {}, "FREQ" };
    juce::Slider       deEsserThresh;
    juce::Label        deEsserThreshLabel { {}, "THRESH" };
    juce::Slider       deEsserRange;
    juce::Label        deEsserRangeLabel { {}, "RANGE" };

    // ── Hum Removal ──
    juce::ToggleButton humEnable       { "Hum Removal" };
    juce::ComboBox     humFreqCombo;
    juce::Label        humFreqLabel     { {}, "FREQUENCY" };
    juce::Slider       humHarmonics;
    juce::Label        humHarmonicsLabel { {}, "HARMONICS" };

    // ── DC Offset ──
    juce::ToggleButton dcEnable        { "DC Offset" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioCleanupPanel)
};
