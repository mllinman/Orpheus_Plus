#include "TextToSampleGenerator.h"
#include <random>

bool TextToSampleGenerator::generateSampleFromText(const juce::String& prompt, double durationSeconds, double sampleRate)
{
    currentPrompt = prompt;
    finished.store(false);

    int numSamples = static_cast<int>(durationSeconds * sampleRate);
    internalBuffer.setSize(1, numSamples);
    internalBuffer.clear();

    // MOCK ONNX Generation:
    // In a real implementation, this would spin up a background thread, load an AudioLDM/MusicGen ONNX model,
    // encode the text prompt to latent space, and run the diffusion steps.
    // For now, we will synthesize a basic white noise burst (like a snare/clap) to prove the pipeline works.

    std::mt19937 gen(42);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

    float* writePtr = internalBuffer.getWritePointer(0);
    for (int i = 0; i < numSamples; ++i) {
        // Apply an exponential decay envelope to make it sound like a drum hit instead of raw noise
        float env = std::exp(-5.0f * (float)i / (float)numSamples);
        writePtr[i] = dist(gen) * env * 0.5f;
    }

    finished.store(true);
    return true;
}

juce::AudioBuffer<float> TextToSampleGenerator::getGeneratedBuffer()
{
    // Return a copy so the UI/Timeline can take ownership
    return juce::AudioBuffer<float>(internalBuffer);
}
