#include "MixerProcessor.h"
#include "../Mastering/MasteringModule.h"

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

    if (masteringModule)
    {
        juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32)samplesPerBlock, (juce::uint32)getTotalNumOutputChannels() };
        masteringModule->prepare(spec);
    }
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

    // Apply Mastering
    if (masteringModule)
    {
        masteringModule->processBlock(buffer, getSampleRate());
    }

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
