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

void TrackProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
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

    // Apply processing via DSP module
    // juce::dsp::AudioBlock<float> block(buffer);
    // juce::dsp::ProcessContextReplacing<float> context(block);

    // Manual ramp application if needed, or use DSP classes if they support it.
    // JUCE DSP Gain/Panner usually perform per-block, but we want per-sample smoothing or small block smoothing.
    // For simplicity in this implementation, we'll apply gain manually with smoothing.
    
    // Panning
    // panner.setPan(smoothPan.getNextValue()); // Simplified: setting pan once per block for now, or we can iterate.
    // panner.process(context);

    // Gain
    // gain.setGainLinear(smoothVolume.getNextValue());
    // gain.process(context);
    
    // Manual gain for sample-accurate smoothing
    if (smoothVolume.isSmoothing())
    {
        smoothVolume.applyGain(buffer, buffer.getNumSamples());
    }
    else
    {
        buffer.applyGain(smoothVolume.getTargetValue());
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
