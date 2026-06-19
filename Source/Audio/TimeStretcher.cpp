#include "TimeStretcher.h"

TimeStretcher::TimeStretcher() {}

TimeStretcher::~TimeStretcher() {}

void TimeStretcher::reset(double sourceSampleRate, int numChannels)
{
    sampleRate = sourceSampleRate;
    channels = numChannels;
    
    interpolators.clear();
    for (int i = 0; i < channels; ++i)
        interpolators.add(new juce::LagrangeInterpolator());
        
    grains.clear();
    grains.resize(maxGrains);
    
    int windowSamples = (int)((grainSizeMs / 1000.0f) * sampleRate);
    windowBuffer.setSize(1, windowSamples);
    
    // Create a Hann window
    auto* winWrite = windowBuffer.getWritePointer(0);
    for (int i = 0; i < windowSamples; ++i)
    {
        winWrite[i] = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * i / (windowSamples - 1.0f)));
    }
    
    readPosition = 0.0f;
}

void TimeStretcher::process(const juce::AudioBuffer<float>& input, 
                            juce::AudioBuffer<float>& output,
                            float stretchRatio,
                            float pitchScale)
{
    if (channels == 0 || input.getNumSamples() == 0 || stretchRatio <= 0.0f)
        return;
        
    int numOutputSamples = output.getNumSamples();
    int inputSamples = input.getNumSamples();
    int windowSamples = windowBuffer.getNumSamples();
    float grainHopSize = windowSamples * 0.5f; // 50% overlap

    // Clear output first since we are summing
    output.clear();

    for (int outIdx = 0; outIdx < numOutputSamples; ++outIdx)
    {
        // Check if we need to spawn a new grain
        bool needsNewGrain = true;
        for (auto& g : grains)
        {
            if (g.active && g.phase < grainHopSize)
            {
                needsNewGrain = false;
                break;
            }
        }
        
        if (needsNewGrain)
        {
            for (auto& g : grains)
            {
                if (!g.active)
                {
                    g.active = true;
                    g.position = readPosition;
                    g.phase = 0.0f;
                    break;
                }
            }
        }
        
        // Process active grains
        for (auto& g : grains)
        {
            if (g.active)
            {
                int winIdx = (int)g.phase;
                if (winIdx >= windowSamples)
                {
                    g.active = false;
                    continue;
                }
                
                float windowVal = windowBuffer.getSample(0, winIdx);
                
                // Read from input at grain position
                int readIdx = (int)g.position;
                if (readIdx >= 0 && readIdx < inputSamples)
                {
                    for (int ch = 0; ch < juce::jmin(channels, input.getNumChannels(), output.getNumChannels()); ++ch)
                    {
                        output.addSample(ch, outIdx, input.getSample(ch, readIdx) * windowVal);
                    }
                }
                
                g.phase += 1.0f; // Window phase always advances by 1 output sample
                g.position += pitchScale; // Grain's internal read position advances by pitchScale
            }
        }
        
        // Global read position advances by stretchRatio (time manipulation)
        readPosition += stretchRatio;
    }
    
    // Normalize slightly due to overlap
    for (int ch = 0; ch < output.getNumChannels(); ++ch)
        juce::FloatVectorOperations::multiply(output.getWritePointer(ch), 0.7f, output.getNumSamples());
        
    // Keep read position in bounds for the next block (wrapping logic handles block-to-block continuity if needed)
    // Note: A true time-stretcher would buffer audio internally. This simplistic approach assumes `input` contains 
    // the available audio starting from 0. In a real DAW, AudioClip provides the correct input block based on `readPosition`.
    // Since AudioClip calls this per block and feeds the relevant segment, we wrap readPosition relative to the block.
    readPosition -= inputSamples;
    if (readPosition < 0) readPosition = 0;
}
