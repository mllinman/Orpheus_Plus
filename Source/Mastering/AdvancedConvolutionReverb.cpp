#include "AdvancedConvolutionReverb.h"

AdvancedConvolutionReverb::AdvancedConvolutionReverb()
{
}

AdvancedConvolutionReverb::~AdvancedConvolutionReverb()
{
}

void AdvancedConvolutionReverb::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 2;

    convolution.prepare(spec);
}

void AdvancedConvolutionReverb::processBlock(juce::AudioBuffer<float>& buffer)
{
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    // Naive dry/wet
    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer);

    convolution.process(context);

    // Mix
    buffer.applyGain(dryWet);
    dryBuffer.applyGain(1.0f - dryWet);
    
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        buffer.addFrom(ch, 0, dryBuffer, ch, 0, buffer.getNumSamples());
}

bool AdvancedConvolutionReverb::loadImpulseResponse(const juce::File& irFile)
{
    if (irFile.existsAsFile())
    {
        convolution.loadImpulseResponse(irFile, juce::dsp::Convolution::Stereo::yes, juce::dsp::Convolution::Trim::no, irFile.getSize());
        return true;
    }
    return false;
}

void AdvancedConvolutionReverb::setDryWet(float proportion)
{
    dryWet = juce::jlimit(0.0f, 1.0f, proportion);
}
