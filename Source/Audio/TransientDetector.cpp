#include "TransientDetector.h"
#include <JuceHeader.h>

std::vector<double> TransientDetector::detectTransients(const juce::AudioBuffer<float>& buffer,
                                                        double sampleRate,
                                                        float threshold,
                                                        float minTimeBetweenTransients)
{
    std::vector<double> transients;
    if (buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0)
        return transients;

    int numSamples = buffer.getNumSamples();
    int minSamplesBetween = (int)(minTimeBetweenTransients * sampleRate);
    
    // Simple envelope follower
    const int windowSize = (int)(0.01 * sampleRate); // 10ms window
    float envelope = 0.0f;
    float attack = std::exp(-1.0f / (0.005f * sampleRate)); // 5ms attack
    float release = std::exp(-1.0f / (0.050f * sampleRate)); // 50ms release
    
    int lastTransientSample = -minSamplesBetween;

    // Use channel 0 for simplicity (mono representation)
    auto* data = buffer.getReadPointer(0);

    for (int i = 0; i < numSamples; ++i)
    {
        float sample = std::abs(data[i]);
        if (sample > envelope)
            envelope = attack * (envelope - sample) + sample;
        else
            envelope = release * (envelope - sample) + sample;

        // Peak detection logic
        if (envelope > threshold)
        {
            if (i - lastTransientSample > minSamplesBetween)
            {
                transients.push_back((double)i / sampleRate);
                lastTransientSample = i;
            }
        }
    }

    return transients;
}
