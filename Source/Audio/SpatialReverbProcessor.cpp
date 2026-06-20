#include "SpatialReverbProcessor.h"

SpatialReverbProcessor::SpatialReverbProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::discreteChannels(12), true)
                     .withOutput("Output", juce::AudioChannelSet::discreteChannels(12), true))
{
    reverbParams.roomSize = 0.5f;
    reverbParams.damping = 0.5f;
    reverbParams.wetLevel = 0.33f;
    reverbParams.dryLevel = 0.4f;
    reverbParams.width = 1.0f;
    reverbParams.freezeMode = 0.0f;

    for (auto& rev : reverbs)
        rev.setParameters(reverbParams);
}

SpatialReverbProcessor::~SpatialReverbProcessor()
{
}

void SpatialReverbProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1;

    for (auto& rev : reverbs)
        rev.prepare(spec);
}

void SpatialReverbProcessor::releaseResources()
{
}

void SpatialReverbProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    int numChannels = std::min(buffer.getNumChannels(), 12);
    int numSamples = buffer.getNumSamples();

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto block = juce::dsp::AudioBlock<float>(buffer).getSingleChannelBlock(ch);
        juce::dsp::ProcessContextReplacing<float> context(block);
        reverbs[ch].process(context);
    }
}

void SpatialReverbProcessor::setRoomSize(float size)
{
    reverbParams.roomSize = juce::jlimit(0.0f, 1.0f, size);
    for (auto& rev : reverbs) rev.setParameters(reverbParams);
}

void SpatialReverbProcessor::setDamping(float damping)
{
    reverbParams.damping = juce::jlimit(0.0f, 1.0f, damping);
    for (auto& rev : reverbs) rev.setParameters(reverbParams);
}

void SpatialReverbProcessor::setWetLevel(float wet)
{
    reverbParams.wetLevel = juce::jlimit(0.0f, 1.0f, wet);
    for (auto& rev : reverbs) rev.setParameters(reverbParams);
}

void SpatialReverbProcessor::setDryLevel(float dry)
{
    reverbParams.dryLevel = juce::jlimit(0.0f, 1.0f, dry);
    for (auto& rev : reverbs) rev.setParameters(reverbParams);
}
