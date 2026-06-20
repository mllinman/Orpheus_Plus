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

    struct MacroTarget {
        int trackIndex;
        juce::String paramID;
        float depth;
        juce::AudioProcessorParameter* cachedParameter { nullptr }; // Fast lookups
    };

    void addTarget(int trackIndex, const juce::String& paramID, float depth, juce::AudioProcessorParameter* param = nullptr);
    void removeTarget(int trackIndex, const juce::String& paramID);

    juce::String getName() const { return name; }
    
    // Automation sync
    void syncWithAudioEngine(class AudioEngine* engine);

private:
    juce::String name;
    float currentValue = 0.0f;
    std::vector<MacroTarget> targets;
};
