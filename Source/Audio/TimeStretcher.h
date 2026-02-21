#pragma once
#include <JuceHeader.h>

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

    /** 
     * Initialise/Reset the stretcher for a new audio stream.
     */
    void reset(double sourceSampleRate, int numChannels);

    /**
     * Process a block of audio. 
     * @param input        The source audio buffer.
     * @param output       The target audio buffer to write into.
     * @param stretchRatio The speed ratio (1.0 = normal, 2.0 = double speed/half duration).
     * @param pitchScale   The frequency multiplier (1.0 = normal, 2.0 = octave up).
     */
    void process(const juce::AudioBuffer<float>& input, 
                 juce::AudioBuffer<float>& output,
                 float stretchRatio,
                 float pitchScale = 1.0f);

private:
    double sampleRate { 44100.0 };
    int channels { 0 };
    
    // Placeholder using JUCE interpolators (one per channel)
    juce::OwnedArray<juce::LagrangeInterpolator> interpolators;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimeStretcher)
};
