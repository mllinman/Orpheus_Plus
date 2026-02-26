#include "MixerProcessor.h"

MixerProcessor::MixerProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

MixerProcessor::~MixerProcessor()
{
}

void MixerProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    smoothVolume.reset(sampleRate, 0.05);
    smoothVolume.setCurrentAndTargetValue(masterVolume.load());
}

void MixerProcessor::releaseResources()
{
}

void MixerProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Apply master volume
    smoothVolume.setTargetValue(masterVolume.load());
    
    if (smoothVolume.isSmoothing())
        smoothVolume.applyGain(buffer, buffer.getNumSamples());
    else
        buffer.applyGain(smoothVolume.getTargetValue());

    // Calculate RMS for metering
    if (buffer.getNumChannels() > 0)
        rmsLeft.store(buffer.getRMSLevel(0, 0, buffer.getNumSamples()));
    
    if (buffer.getNumChannels() > 1)
        rmsRight.store(buffer.getRMSLevel(1, 0, buffer.getNumSamples()));
}

void MixerProcessor::setMasterVolume(float vol)
{
    masterVolume.store(vol);
}
