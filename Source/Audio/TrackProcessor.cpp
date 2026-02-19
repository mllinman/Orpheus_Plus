#include <JuceHeader.h>
#include "TrackProcessor.h"

TrackProcessor::TrackProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    // panner.setRule(juce::dsp::PannerRule::linear);
}

TrackProcessor::~TrackProcessor()
{
}

void TrackProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // juce::dsp::ProcessSpec spec;
    // spec.sampleRate = sampleRate;
    // spec.maximumBlockSize = samplesPerBlock;
    // spec.numChannels = getTotalNumOutputChannels();

    // gain.prepare(spec);
    // panner.prepare(spec);

    smoothVolume.reset(sampleRate, 0.05); // 50ms ramp
    smoothPan.reset(sampleRate, 0.05);
}

void TrackProcessor::releaseResources()
{
}

void TrackProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    if (muted.load())
    {
        buffer.clear();
        return;
    }

    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Update smoothed parameters
    smoothVolume.setTargetValue(currentVolume.load());
    smoothPan.setTargetValue(currentPan.load());

    // 1. Process Insert FX Chain
    for (auto* fx : insertFX)
    {
        // Simple internal bypass check if the processor supports it, or just process
        fx->processBlock(buffer, midiMessages);
    }
    
    // 2. Track Volume
    if (smoothVolume.isSmoothing())
    {
        smoothVolume.applyGain(buffer, buffer.getNumSamples());
    }
    else
    {
        buffer.applyGain(smoothVolume.getTargetValue());
    }
    
    // 3. Track Pan (basic L/R multiplier for simplicity, actual panning requires a Panner or math)
    float pan = smoothPan.getCurrentValue();
    if (buffer.getNumChannels() == 2)
    {
        float leftGain = (pan <= 0.0f) ? 1.0f : 1.0f - pan;
        float rightGain = (pan >= 0.0f) ? 1.0f : 1.0f + pan;
        buffer.applyGain(0, 0, buffer.getNumSamples(), leftGain);
        buffer.applyGain(1, 0, buffer.getNumSamples(), rightGain);
    }
}

void TrackProcessor::setVolume(float vol)
{
    currentVolume.store(vol);
}

void TrackProcessor::setPan(float pan)
{
    currentPan.store(pan);
}

void TrackProcessor::setMute(bool shouldMute)
{
    muted.store(shouldMute);
}

void TrackProcessor::setSolo(bool shouldSolo)
{
    soloed.store(shouldSolo);
}
