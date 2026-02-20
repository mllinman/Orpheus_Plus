#include "LinearPhaseEQ.h"

LinearPhaseEQ::LinearPhaseEQ()
{
}

void LinearPhaseEQ::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    filterChain.prepare(spec);

    for (int i = 0; i < NumBands; ++i)
        updateFilter(i);
}

void LinearPhaseEQ::process(juce::dsp::ProcessContextReplacing<float> context)
{
    filterChain.process(context);
}

void LinearPhaseEQ::reset()
{
    filterChain.reset();
}

void LinearPhaseEQ::setBandParameters(int index, float frequency, float q, float gainDecibels)
{
    if (index >= 0 && index < NumBands)
    {
        bands[index].freq = frequency;
        bands[index].q = q;
        bands[index].gainDecibels = gainDecibels;
        updateFilter(index);
    }
}

void LinearPhaseEQ::updateFilter(int index)
{
    auto freq = juce::jlimit(20.0f, (float)(sampleRate / 2.0 - 1.0), bands[index].freq);
    auto q = juce::jlimit(0.1f, 10.0f, bands[index].q);
    auto gain = bands[index].gainDecibels;

    using Coefficients = juce::dsp::IIR::Coefficients<float>;
    using FilterCoeffs = juce::ReferenceCountedObjectPtr<juce::dsp::IIR::Coefficients<float>>;
    
    FilterCoeffs coeffs;

    if (index == 0) // Low Shelf
        coeffs = Coefficients::makeLowShelf(sampleRate, freq, q, juce::Decibels::decibelsToGain(gain));
    else if (index == NumBands - 1) // High Shelf
        coeffs = Coefficients::makeHighShelf(sampleRate, freq, q, juce::Decibels::decibelsToGain(gain));
    else // Peak
        coeffs = Coefficients::makePeakFilter(sampleRate, freq, q, juce::Decibels::decibelsToGain(gain));

    if (coeffs != nullptr)
    {
        switch (index)
        {
            case 0: filterChain.template get<0>().state = coeffs; break;
            case 1: filterChain.template get<1>().state = coeffs; break;
            case 2: filterChain.template get<2>().state = coeffs; break;
            case 3: filterChain.template get<3>().state = coeffs; break;
            case 4: filterChain.template get<4>().state = coeffs; break;
            case 5: filterChain.template get<5>().state = coeffs; break;
            case 6: filterChain.template get<6>().state = coeffs; break;
            case 7: filterChain.template get<7>().state = coeffs; break;
        }
    }
}
