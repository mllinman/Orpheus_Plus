#pragma once
#include <JuceHeader.h>
#include "../AudioCleanup/ADRProcessor.h"
#include "../Audio/AudioEngine.h"

class ADRPanel : public juce::Component
{
public:
    ADRPanel(AudioEngine& engine);
    ~ADRPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    AudioEngine& audioEngine;
    ADRProcessor adrProcessor;

    juce::TextButton analyzeLocationToneBtn{"Analyze Room Tone"};
    juce::TextButton applyADRMatchBtn{"Level & Match ADR"};
    juce::Label statusLabel;

    void analyzeLocationToneClicked();
    void applyADRMatchClicked();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ADRPanel)
};
