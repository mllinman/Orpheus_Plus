#include "SpectralCarver.h"

SpectralCarver::SpectralCarver()
{
    sidechainMagnitudes.assign(2048 * 2, 0.0f);
}

void SpectralCarver::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Initialize FFT windowing
    juce::ignoreUnused(sampleRate, samplesPerBlock);
}

void SpectralCarver::processSidechainMask(const juce::AudioBuffer<float>& maskBuffer)
{
    // 1. Apply window function
    // 2. Perform forward FFT on maskBuffer
    // 3. Store magnitudes into sidechainMagnitudes array to be used as suppression thresholds
    
    // Simulate AI envelope extraction: we find the RMS or peak of the mask to drive the carving
    float maskRms = maskBuffer.getRMSLevel(0, 0, maskBuffer.getNumSamples());
    
    // In a real STFT, we'd distribute this energy across frequency bins based on the FFT.
    // Here we just mock the array population for demonstration.
    std::fill(sidechainMagnitudes.begin(), sidechainMagnitudes.end(), maskRms);
}

void SpectralCarver::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    float amount = carveAmount.load();
    if (amount < 0.01f) return;

    // 1. Perform forward FFT on the main buffer
    // 2. For each frequency bin `i`:
    //    if (sidechainMagnitudes[i] > threshold) 
    //        targetMagnitudes[i] *= (1.0f - amount * sidechainMagnitudes[i]);
    // 3. Perform inverse FFT and overlap-add back to buffer
    
    // Mock the DSP block by scaling the target buffer directly proportional to the mask's energy
    float currentMaskLevel = sidechainMagnitudes[0]; // Simplified: taking bin 0 as global energy
    if (currentMaskLevel > 0.05f) // Threshold
    {
        float suppressionFactor = 1.0f - (currentMaskLevel * amount);
        suppressionFactor = juce::jlimit(0.1f, 1.0f, suppressionFactor);
        buffer.applyGain(suppressionFactor);
    }
}
