#pragma once
#include <JuceHeader.h>

class AIPluginHost
{
public:
    AIPluginHost();
    ~AIPluginHost();

    bool loadModel(const juce::File& onnxFile);
    
    // Generic inference interface
    void processAudio(juce::AudioBuffer<float>& buffer);

private:
    bool modelLoaded = false;
    juce::String currentModelPath;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AIPluginHost)
};
