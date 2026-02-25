#include "SimpleDelayProcessor.h"

SimpleDelayProcessor::SimpleDelayProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

SimpleDelayProcessor::~SimpleDelayProcessor() {}

void SimpleDelayProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    this->mSampleRate = sampleRate;
    delayBuffer.setSize(2, (int)(sampleRate * 2.0)); // 2 seconds max
    delayBuffer.clear();
    writePos = 0;
}

void SimpleDelayProcessor::releaseResources()
{
    delayBuffer.clear();
}

bool SimpleDelayProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void SimpleDelayProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    int numSamples = buffer.getNumSamples();
    int delaySamples = (int)(mSampleRate * delayTime);
    if (delaySamples == 0) return;

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        auto* delayData = delayBuffer.getWritePointer(channel % delayBuffer.getNumChannels());
        int bufSize = delayBuffer.getNumSamples();

        int tempWritePos = writePos;
        for (int i = 0; i < numSamples; ++i)
        {
            int readPos = tempWritePos - delaySamples;
            if (readPos < 0) readPos += bufSize;

            float delayedSample = delayData[readPos];
            float input = channelData[i];

            delayData[tempWritePos] = input + delayedSample * feedback;
            channelData[i] = input * (1.0f - mix) + delayedSample * mix;

            tempWritePos++;
            if (tempWritePos >= bufSize) tempWritePos = 0;
        }
        
        // Ensure write pointer advances only once per sample block, done outside the channel loop
        if (channel == buffer.getNumChannels() - 1)
        {
            writePos = tempWritePos;
        }
    }
}
