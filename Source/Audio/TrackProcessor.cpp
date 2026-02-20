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
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();

    compressor.prepare(spec);
    highShelf.prepare(spec);
    lowShelf.prepare(spec);

    updateSweetener(); // initialize states

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
        
    // First, clear input channels since we don't have a global input right now
    for (auto i = 0; i < totalNumInputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Render timeline clips directly into this track's buffer
    if (renderAudioCallback)
    {
        renderAudioCallback(buffer, currentPlayhead.load());
    }

    // Update smoothed parameters
    smoothVolume.setTargetValue(currentVolume.load());
    smoothPan.setTargetValue(currentPan.load());

    // 1. Process Channel Strip Sweetener
    float sweetAmt = sweetenerAmount.load();
    if (sweetAmt > 0.0f)
    {
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        
        lowShelf.process(context);
        compressor.process(context);
        highShelf.process(context);
    }

    // 2. Process Insert FX Chain
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
    
    // 4. Calculate Peaks (attack/decay envelope follower)
    float currentPeakL = peakL.load();
    float currentPeakR = peakR.load();
    
    float attackCoef = 0.9f;
    float releaseCoef = 0.999f;
    
    if (buffer.getNumChannels() > 0)
    {
        auto* chL = buffer.getReadPointer(0);
        for (int i=0; i < buffer.getNumSamples(); ++i)
        {
            float envIn = std::abs(chL[i]);
            if (currentPeakL < envIn)
                currentPeakL += attackCoef * (envIn - currentPeakL);
            else
                currentPeakL *= releaseCoef;
        }
    }
    if (buffer.getNumChannels() > 1)
    {
        auto* chR = buffer.getReadPointer(1);
        for (int i=0; i < buffer.getNumSamples(); ++i)
        {
            float envIn = std::abs(chR[i]);
            if (currentPeakR < envIn)
                currentPeakR += attackCoef * (envIn - currentPeakR);
            else
                currentPeakR *= releaseCoef;
        }
    }
    
    peakL.store(currentPeakL);
    peakR.store(currentPeakR);
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

void TrackProcessor::updateSweetener()
{
    float amt = sweetenerAmount.load();
    
    // Compressor ramps depending on sweetener
    // 0 amt: Ratio 1.0 (no comp), Thresh 0dB
    // 1 amt: Ratio 4.0, Thresh -20dB (heavy comp)
    compressor.setRatio(1.0f + amt * 3.0f);
    compressor.setThreshold(0.0f - amt * 20.0f);
    compressor.setAttack(15.0f);
    compressor.setRelease(150.0f);

    double sr = getSampleRate();
    if (sr <= 0) sr = 44100.0;

    // High Shelf (air) from 0 to +6dB
    float highGain = juce::Decibels::decibelsToGain(amt * 6.0f);
    *highShelf.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(sr, 8000.0f, 0.707f, highGain);

    // Low Shelf (warmth) from 0 to +3dB
    float lowGain = juce::Decibels::decibelsToGain(amt * 3.0f);
    *lowShelf.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(sr, 150.0f, 0.707f, lowGain);
}
