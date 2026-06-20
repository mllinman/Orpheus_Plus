#pragma once
#include <JuceHeader.h>
#include "../AI/TextToSampleGenerator.h"

class TextToSamplePanel : public juce::Component, public juce::Timer
{
public:
    TextToSamplePanel();
    ~TextToSamplePanel() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    void triggerGeneration();

    juce::TextEditor promptEditor;
    juce::TextButton generateButton { "Generate (ONNX)" };
    juce::Label statusLabel;

    TextToSampleGenerator generator;
    bool isGenerating { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TextToSamplePanel)
};
