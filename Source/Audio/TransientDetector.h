#pragma once
#include <JuceHeader.h>
#include <vector>

class TransientDetector
{
public:
    static std::vector<double> detectTransients(const juce::AudioBuffer<float>& buffer,
                                                double sampleRate,
                                                float threshold = 0.5f,
                                                float minTimeBetweenTransients = 0.05f);
};
