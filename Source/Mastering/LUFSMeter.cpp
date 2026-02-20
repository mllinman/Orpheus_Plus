#include "LUFSMeter.h"

LUFSMeter::LUFSMeter()
{
}

void LUFSMeter::prepare(double newSampleRate, int samplesPerBlock)
{
    sampleRate = newSampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = newSampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 2; // Fixed to stereo for now

    // Stage 1: Pre-filter (high shelf)
    // EBU R128 specifies: +4.0 dB at 1500 Hz, Q = 0.7071
    auto shelfCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(
        sampleRate, 1500.0f, 0.7071f, juce::Decibels::decibelsToGain(4.0f));

    preFilter.state = shelfCoeffs;
    preFilter.prepare(spec);

    // Stage 2: RLB weighting (high pass)
    // EBU R128 specifies: Fc = 38 Hz, Q = 0.5
    auto rlbCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(
        sampleRate, 38.0f, 0.5f);

    rlbFilter.state = rlbCoeffs;
    rlbFilter.prepare(spec);

    internalBuffer.setSize(2, samplesPerBlock);

    // 400ms history for Momentary LUFS
    maxHistorySamples = static_cast<int>(sampleRate * 0.4);
    energyHistory.assign(maxHistorySamples, 0.0f);
    historyIndex = 0;
    
    reset();
}

void LUFSMeter::reset()
{
    preFilter.reset();
    rlbFilter.reset();
    std::fill(energyHistory.begin(), energyHistory.end(), 0.0f);
    currentMomentaryLUFS = -70.0f;
}

void LUFSMeter::process(const juce::AudioBuffer<float>& buffer)
{
    auto numSamples = buffer.getNumSamples();
    auto numChannels = buffer.getNumChannels();
    
    // Resize internal storage if needed
    if (internalBuffer.getNumSamples() < numSamples)
        internalBuffer.setSize(2, numSamples, false, false, true);
    
    // Copy input to internal buffer
    for (int ch = 0; ch < std::min(numChannels, 2); ++ch)
        internalBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
    
    // Process through K-weighting filters
    juce::dsp::AudioBlock<float> block(internalBuffer.getArrayOfWritePointers(), 
                                       std::min(numChannels, 2), 
                                       numSamples);
    juce::dsp::ProcessContextReplacing<float> context(block);

    preFilter.process(context);
    rlbFilter.process(context);

    // Compute squared sum and update momentary history
    for (int i = 0; i < numSamples; ++i)
    {
        float sumSquares = 0.0f;
        for (int ch = 0; ch < std::min(numChannels, 2); ++ch)
        {
            float samp = internalBuffer.getSample(ch, i);
            sumSquares += samp * samp;
        }

        // Store sum of squared samples
        energyHistory[historyIndex] = sumSquares;
        historyIndex = (historyIndex + 1) % maxHistorySamples;
    }

    // Every block, update the average energy over 400ms
    float totalEnergy = 0.0f;
    for (float e : energyHistory)
        totalEnergy += e;
        
    float meanEnergy = totalEnergy / (float)maxHistorySamples;

    // Convert to LUFS ( -0.691 constant offset )
    if (meanEnergy > 1e-10f)
        currentMomentaryLUFS = -0.691f + 10.0f * std::log10(meanEnergy);
    else
        currentMomentaryLUFS = -70.0f;
}
