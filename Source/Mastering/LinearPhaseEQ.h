#pragma once

#include <JuceHeader.h>

/** A simple visual-friendly EQ that acts as a placeholder for a true Linear Phase EQ.
 *  It provides up to 8 parametric bands.
 */
class LinearPhaseEQ
{
public:
    LinearPhaseEQ();
    ~LinearPhaseEQ() = default;

    void prepare(const juce::dsp::ProcessSpec& spec);
    void process(juce::dsp::ProcessContextReplacing<float> context);
    void reset();

    // 0 = low shelf, 6 = high shelf, 1-5 = peak
    void setBandParameters(int index, float frequency, float q, float gainDecibels);

private:
    void updateFilter(int index);

    static constexpr int NumBands = 8;
    juce::dsp::ProcessorChain<
        juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>,
        juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>,
        juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>,
        juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>,
        juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>,
        juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>,
        juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>,
        juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>
    > filterChain;

    struct BandState {
        float freq = 1000.0f;
        float q = 0.707f;
        float gainDecibels = 0.0f;
    };
    std::array<BandState, NumBands> bands;
    double sampleRate = 44100.0;
};
