#pragma once
#include <JuceHeader.h>
#include "../Audio/AudioEngine.h"

class ModulationMatrixPanel : public juce::Component, public juce::Timer
{
public:
    ModulationMatrixPanel(AudioEngine& engine);
    ~ModulationMatrixPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    
    void timerCallback() override;

private:
    void drawLFOBox(juce::Graphics& g, juce::Rectangle<int> bounds, const LFOSource& lfo, int index);
    void drawMappingsList(juce::Graphics& g, juce::Rectangle<int> bounds);
    
    AudioEngine& audioEngine;
    
    juce::TextButton addLfoButton { "Add LFO" };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationMatrixPanel)
};
