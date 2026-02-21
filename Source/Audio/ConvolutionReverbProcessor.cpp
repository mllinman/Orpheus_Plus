#include "ConvolutionReverbProcessor.h"

ConvolutionReverbProcessor::ConvolutionReverbProcessor()
{
}

ConvolutionReverbProcessor::~ConvolutionReverbProcessor()
{
}

void ConvolutionReverbProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32)samplesPerBlock;
    spec.numChannels = (juce::uint32)getTotalNumOutputChannels();
    
    convolution.prepare(spec);
}

void ConvolutionReverbProcessor::releaseResources()
{
    convolution.reset();
}

void ConvolutionReverbProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    
    // We want to apply dry/wet manually if we want better control, 
    // or use dsp::Convolution's built-in if it had one (it doesn't have a simple float param for it inside the process call, usually we blend).
    // Actually, juce::dsp::Convolution processes the block directly.
    
    if (dryWet >= 1.0f)
    {
        convolution.process(context);
    }
    else if (dryWet > 0.0f)
    {
        juce::AudioBuffer<float> dryBuffer;
        dryBuffer.makeCopyOf(buffer);
        
        convolution.process(context);
        
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            buffer.applyGain(ch, 0, buffer.getNumSamples(), dryWet);
            buffer.addFrom(ch, 0, dryBuffer, ch, 0, buffer.getNumSamples(), 1.0f - dryWet);
        }
    }
}

void ConvolutionReverbProcessor::loadImpulseResponse(const juce::File& file)
{
    if (file.existsAsFile())
    {
        convolution.loadImpulseResponse(file, 
                                        juce::dsp::Convolution::Stereo::yes, 
                                        juce::dsp::Convolution::Trim::yes, 
                                        0, 
                                        juce::dsp::Convolution::Normalise::yes);
    }
}

void ConvolutionReverbProcessor::setDryWet(float wetAmount)
{
    dryWet = juce::jlimit(0.0f, 1.0f, wetAmount);
}
