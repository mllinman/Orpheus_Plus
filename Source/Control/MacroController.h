#pragma once
#include <JuceHeader.h>
#include <vector>

class MacroController
{
public:
    MacroController(const juce::String& name);
    ~MacroController() = default;

    void setValue(float newValue);
    float getValue() const { return currentValue; }

    struct Target {
        juce::AudioProcessorParameter* parameter;
        float minScaling;
        float maxScaling;
        // Optionally a custom curve could go here
    };

    void addTarget(juce::AudioProcessorParameter* param, float minScaling = 0.0f, float maxScaling = 1.0f);
    void removeTarget(juce::AudioProcessorParameter* param);

    juce::String getName() const { return name; }

private:
    juce::String name;
    float currentValue = 0.0f;
    std::vector<Target> targets;
};
