#include "ADRProcessor.h"
#include <cmath>

ADRProcessor::ADRProcessor()
{
    matchEQCurve.resize(512, 1.0f);
}

ADRProcessor::~ADRProcessor()
{
}

void ADRProcessor::analyzeLocationAudio(const juce::AudioBuffer<float>& locationAudio)
{
    if (locationAudio.getNumChannels() == 0 || locationAudio.getNumSamples() == 0)
        return;

    // Faux analysis: extract spectral footprint (simulated)
    for (size_t i = 0; i < matchEQCurve.size(); ++i)
    {
        // Simulate an arbitrary EQ curve representing room tone
        matchEQCurve[i] = 0.8f + (std::sin(i * 0.05f) * 0.2f);
    }
    
    hasLocationFingerprint = true;
}

void ADRProcessor::processADRClip(juce::AudioBuffer<float>& adrAudio)
{
    if (adrAudio.getNumChannels() == 0 || adrAudio.getNumSamples() == 0)
        return;

    applyDialogueLeveling(adrAudio);
    
    if (hasLocationFingerprint)
    {
        applyMatchEQ(adrAudio);
    }
}

void ADRProcessor::applyDialogueLeveling(juce::AudioBuffer<float>& buffer)
{
    // A rudimentary automatic gain control/compressor
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    // Calculate RMS
    float sumSquares = 0.0f;
    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* channelData = buffer.getReadPointer(channel);
        for (int i = 0; i < numSamples; ++i)
        {
            sumSquares += channelData[i] * channelData[i];
        }
    }
    
    float currentRms = std::sqrt(sumSquares / (numChannels * numSamples));
    
    if (currentRms > 0.0001f)
    {
        float gainFactor = targetRMSLevel / currentRms;
        // Limit max gain to prevent exploding noise
        gainFactor = juce::jmin(gainFactor, 4.0f); 
        
        buffer.applyGain(gainFactor);
    }
}

void ADRProcessor::applyMatchEQ(juce::AudioBuffer<float>& buffer)
{
    // A faux Match EQ application using time-domain approximation for demonstration
    // Real implementation would use FFT convolution
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        for (int i = 0; i < numSamples; ++i)
        {
            // Just roll off highs as a primitive faux "room" sound
            if (i > 0)
                channelData[i] = channelData[i] * 0.6f + channelData[i - 1] * 0.4f;
        }
    }
}
