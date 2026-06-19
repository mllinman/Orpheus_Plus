#include "SpeakerEmbeddingExtractor.h"

SpeakerEmbeddingExtractor::SpeakerEmbeddingExtractor()
{
}

SpeakerEmbeddingExtractor::~SpeakerEmbeddingExtractor()
{
}

void SpeakerEmbeddingExtractor::trainProfileAsync(const juce::File& vocalAudioFile, 
                                                   std::function<void(bool, const juce::String&)> onComplete)
{
    if (processing.load())
        return;

    processing.store(true);
    progress.store(0.0f);

    juce::Thread::launch([this, vocalAudioFile, onComplete] {
        runTraining(vocalAudioFile, onComplete);
    });
}

void SpeakerEmbeddingExtractor::runTraining(const juce::File& file, 
                                             std::function<void(bool, const juce::String&)> onComplete)
{
    // Simulate training process: Launch background python script `train_rvc.py --input <file>`
    for (int i = 0; i < 15; ++i)
    {
        juce::Thread::sleep(200); // Wait time for simulation
        progress.store((i + 1) * (1.0f / 15.0f));
    }

    // In a real environment, this python script would output an .onnx file
    juce::String mockOnnxPath = file.getParentDirectory().getChildFile("trained_voice_profile.onnx").getFullPathName();

    processing.store(false);
    
    if (onComplete) {
        juce::MessageManager::callAsync([onComplete, mockOnnxPath] {
            onComplete(true, mockOnnxPath);
        });
    }
}
