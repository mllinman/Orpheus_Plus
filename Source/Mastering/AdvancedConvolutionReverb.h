#pragma once
#include <JuceHeader.h>

class AdvancedConvolutionReverb
{
public:
    AdvancedConvolutionReverb();
    ~AdvancedConvolutionReverb();

    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void processBlock(juce::AudioBuffer<float>& buffer);

    bool loadImpulseResponse(const juce::File& irFile);
    void setDryWet(float proportion);

private:
    juce::dsp::Convolution convolution;
    float dryWet = 0.5f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AdvancedConvolutionReverb)
};
