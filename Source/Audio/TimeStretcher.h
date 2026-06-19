#pragma once
#include <JuceHeader.h>
#include <vector>
#include <cmath>

/**
 * A wrapper class for high-quality time-stretching and pitch-shifting.
 * This version acts as an architectural placeholder using JUCE's Lagrange interpolation.
 * It is designed to be easily swapped for the Rubber Band Library.
 */
class TimeStretcher
{
public:
    TimeStretcher();
    ~TimeStretcher();

    void reset(double sourceSampleRate, int numChannels);

    void process(const juce::AudioBuffer<float>& input, 
                 juce::AudioBuffer<float>& output,
                 float stretchRatio,
                 float pitchScale = 1.0f);

private:
    double sampleRate { 44100.0 };
    int channels { 0 };
    
    // Granular State
    struct Grain {
        float position = 0.0f;
        float phase = 0.0f;
        bool active = false;
    };
    
    std::vector<Grain> grains;
    int maxGrains = 4;
    float grainSizeMs = 60.0f;
    float readPosition = 0.0f;
    
    juce::OwnedArray<juce::LagrangeInterpolator> interpolators;
    juce::AudioBuffer<float> windowBuffer;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimeStretcher)
};
