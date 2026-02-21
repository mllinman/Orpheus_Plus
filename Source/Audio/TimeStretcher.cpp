/**
 * TimeStretcher.cpp — Architectural Placeholder
 * 
 * To integrate the actual Rubber Band Library:
 * 1. Install Rubber Band and update CMakeLists.txt to link against it.
 * 2. #include <rubberband/RubberBandStretcher.h>
 * 3. Replace the LagrangeInterpolator logic in process() with:
 *    rubberband->setPitchScale(pitchScale);
 *    rubberband->setTimeRatio(stretchRatio);
 *    rubberband->process(src, outputSamples, false);
 */
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
}

void TimeStretcher::process(const juce::AudioBuffer<float>& input, 
                            juce::AudioBuffer<float>& output,
                            float stretchRatio,
                            float pitchScale)
{
    // Total speed ratio for resampling is stretch * pitch
    // (e.g. 2x speed and 1x pitch = 2x sample usage)
    double speedRatio = (double)stretchRatio * (double)pitchScale;
    
    if (speedRatio <= 0.0) return;

    for (int ch = 0; ch < channels; ++ch)
    {
        if (ch >= input.getNumChannels() || ch >= output.getNumChannels())
            continue;
            
        auto* src = input.getReadPointer(ch);
        auto* dst = output.getWritePointer(ch);
        
        // LagrangeInterpolator::process(double speedRatio, const float* input, float* output, int numOutputSamples)
        // Here, speedRatio = sourceRate / targetRate.
        // If we want to play 2x faster, we consume 2 source samples per 1 output sample.
        // So speedRatio is indeed the factor of source consumption.
        
        interpolators[ch]->process(speedRatio, src, dst, output.getNumSamples());
    }
}
