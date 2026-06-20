#pragma once
#include <JuceHeader.h>
#include <vector>
#include <atomic>

class TextToSampleGenerator
{
public:
    TextToSampleGenerator() = default;
    ~TextToSampleGenerator() = default;

    // Trigger an offline ONNX generation (mocked for now, but architecture supports AudioLDM embeddings)
    // Returns true if generation started successfully
    bool generateSampleFromText(const juce::String& prompt, double durationSeconds, double sampleRate);

    // Call this in a timer/background thread to check completion
    bool isFinished() const { return finished.load(); }
    
    // Retrieves the generated 32-bit float audio buffer
    juce::AudioBuffer<float> getGeneratedBuffer();

private:
    std::atomic<bool> finished { false };
    juce::AudioBuffer<float> internalBuffer;
    juce::String currentPrompt;
};
