#pragma once
#include <JuceHeader.h>
#include <vector>
#include <functional>

// Forward declaration of AppState if needed
class AppState;

// Extracts a target speaker embedding (voice profile) from a given audio file
class SpeakerEmbeddingExtractor
{
public:
    SpeakerEmbeddingExtractor();
    ~SpeakerEmbeddingExtractor();

    // Trigger background training process. Calls onComplete with the exported ONNX model path when done.
    void trainProfileAsync(const juce::File& vocalAudioFile, 
                           std::function<void(bool success, const juce::String& onnxModelPath)> onComplete);

    bool isProcessing() const { return processing.load(); }
    float getProgress() const { return progress.load(); }

private:
    void runTraining(const juce::File& file, std::function<void(bool, const juce::String&)> onComplete);

    std::atomic<bool> processing { false };
    std::atomic<float> progress { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpeakerEmbeddingExtractor)
};
