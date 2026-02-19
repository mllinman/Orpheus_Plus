#include "MultibandCompressor.h"

MultibandCompressor::MultibandCompressor()
{
    filterLP1.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
    filterHP1.setType(juce::dsp::LinkwitzRileyFilterType::highpass);
    filterLP2.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
    filterHP2.setType(juce::dsp::LinkwitzRileyFilterType::highpass);

    setLowMidCrossover(200.0f);
    setMidHighCrossover(2000.0f); // 5kHz is high, 2kHz is more common for mid/high split
    
    makeupGains.fill(1.0f);
}

MultibandCompressor::~MultibandCompressor()
{
}

void MultibandCompressor::prepare(const juce::dsp::ProcessSpec& spec)
{
    currentSampleRate = spec.sampleRate;

    filterLP1.prepare(spec);
    filterHP1.prepare(spec);
    filterLP2.prepare(spec);
    filterHP2.prepare(spec);

    for (auto& comp : compressors)
        comp.prepare(spec);

    int maxBlock = spec.maximumBlockSize;
    int numCh    = spec.numChannels;

    bufferMob.setSize(numCh, maxBlock);
    bufferHigh.setSize(numCh, maxBlock);
    bufferMid.setSize(numCh, maxBlock);
    bufferLow.setSize(numCh, maxBlock);
}

void MultibandCompressor::reset()
{
    filterLP1.reset();
    filterHP1.reset();
    filterLP2.reset();
    filterHP2.reset();
    for (auto& comp : compressors) comp.reset();
}

void MultibandCompressor::setLowMidCrossover(float freq)
{
    filterLP1.setCutoffFrequency(freq);
    filterHP1.setCutoffFrequency(freq);
}

void MultibandCompressor::setMidHighCrossover(float freq)
{
    filterLP2.setCutoffFrequency(freq);
    filterHP2.setCutoffFrequency(freq);
}

void MultibandCompressor::setThreshold(int band, float thresholdDB)
{
    if (band >= 0 && band < NUM_BANDS)
        compressors[band].setThreshold(thresholdDB);
}

void MultibandCompressor::setRatio(int band, float ratio)
{
    if (band >= 0 && band < NUM_BANDS)
        compressors[band].setRatio(ratio);
}

void MultibandCompressor::setAttack(int band, float attackMs)
{
    if (band >= 0 && band < NUM_BANDS)
        compressors[band].setAttack(attackMs);
}

void MultibandCompressor::setRelease(int band, float releaseMs)
{
    if (band >= 0 && band < NUM_BANDS)
        compressors[band].setRelease(releaseMs);
}

void MultibandCompressor::setMakeupGain(int band, float gainDB)
{
    if (band >= 0 && band < NUM_BANDS)
        makeupGains[band] = juce::Decibels::decibelsToGain(gainDB);
}

void MultibandCompressor::process(juce::AudioBuffer<float>& buffer)
{
    // Crossover Logic
    // We assume stereo or mono.
    
    // 1. Copy input to bufferMob for parallel processing
    bufferMob.makeCopyOf(buffer, true);
    
    juce::dsp::AudioBlock<float> blockInput(buffer);
    juce::dsp::AudioBlock<float> blockMob(bufferMob);
    
    juce::dsp::ProcessContextReplacing<float> contextLP1(blockInput);
    juce::dsp::ProcessContextReplacing<float> contextHP1(blockMob);
    
    // In-place processing:
    // buffer -> LP1 -> Low Band (remains in 'buffer')
    // bufferMob -> HP1 -> Mid+High (remains in 'bufferMob')
    filterLP1.process(contextLP1);
    filterHP1.process(contextHP1);
    
    // Save Low band to bufferLow
    bufferLow.makeCopyOf(buffer, true);
    
    // Now split Mid+High (bufferMob)
    bufferHigh.makeCopyOf(bufferMob, true); // Copy for High path
    
    juce::dsp::AudioBlock<float> blockHigh(bufferHigh);
    juce::dsp::ProcessContextReplacing<float> contextHP2(blockHigh);
    
    juce::dsp::ProcessContextReplacing<float> contextLP2(blockMob); // Mid path
    
    filterLP2.process(contextLP2); // bufferMob now contains Mid
    filterHP2.process(contextHP2); // bufferHigh now contains High
    
    // Copy Mid to bufferMid
    bufferMid.makeCopyOf(bufferMob, true);
    
    // Compress Low
    {
        juce::dsp::AudioBlock<float> block(bufferLow);
        juce::dsp::ProcessContextReplacing<float> ctx(block);
        compressors[0].process(ctx);
        if (makeupGains[0] != 1.0f) bufferLow.applyGain(makeupGains[0]);
    }
    
    // Compress Mid
    {
        juce::dsp::AudioBlock<float> block(bufferMid);
        juce::dsp::ProcessContextReplacing<float> ctx(block);
        compressors[1].process(ctx);
        if (makeupGains[1] != 1.0f) bufferMid.applyGain(makeupGains[1]);
    }
    
    // Compress High
    {
        juce::dsp::AudioBlock<float> block(bufferHigh);
        juce::dsp::ProcessContextReplacing<float> ctx(block);
        compressors[2].process(ctx);
        if (makeupGains[2] != 1.0f) bufferHigh.applyGain(makeupGains[2]);
    }
    
    // Sum
    buffer.clear();
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        buffer.addFrom(ch, 0, bufferLow, ch, 0, buffer.getNumSamples());
        buffer.addFrom(ch, 0, bufferMid, ch, 0, buffer.getNumSamples());
        buffer.addFrom(ch, 0, bufferHigh, ch, 0, buffer.getNumSamples());
    }
}
