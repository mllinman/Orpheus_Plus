#pragma once

#include <JuceHeader.h>

/** A basic EBU R128 LUFS Meter implementation for stereo audio. 
 *  Provides momentary (400ms) loudness.
 */
class LUFSMeter
{
public:
    LUFSMeter();
    ~LUFSMeter() = default;

    void prepare(double sampleRate, int samplesPerBlock);
    void process(const juce::AudioBuffer<float>& buffer);
    void reset();

    /// Returns the most recently computed momentary LUFS value
    float getMomentaryLUFS() const { return currentMomentaryLUFS; }

private:
    double sampleRate { 44100.0 };

    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> preFilter;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> rlbFilter;

    // Buffer for squaring/filtering samples without altering the original
    juce::AudioBuffer<float> internalBuffer;

    // For momentary calculation
    float currentMomentaryLUFS { -70.0f };
    std::vector<float> energyHistory;
    int historyIndex { 0 };
    int maxHistorySamples { 0 };
};
