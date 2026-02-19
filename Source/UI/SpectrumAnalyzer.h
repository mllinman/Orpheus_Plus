#pragma once
#include <JuceHeader.h>
#include "../Audio/AudioEngine.h"

//==============================================================================
class SpectrumAnalyzer : public juce::Component, private juce::Timer
{
public:
    explicit SpectrumAnalyzer(AudioEngine& engine);
    ~SpectrumAnalyzer() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    void pushBuffer(const juce::AudioBuffer<float>& buffer);

private:
    void timerCallback() override;

    void pushNextSampleIntoFifo(float sample);
    // void drawFrame(juce::Graphics&); // Removed unused


    static constexpr int FFT_ORDER = 11;
    static constexpr int FFT_SIZE  = 1 << FFT_ORDER; // 2048
    static constexpr int SCOPE_SIZE = 512;

    AudioEngine& audioEngine;

    juce::dsp::FFT fft { FFT_ORDER };
    juce::dsp::WindowingFunction<float> window {
        (size_t)FFT_SIZE, juce::dsp::WindowingFunction<float>::hann };

    std::array<float, FFT_SIZE * 2> fftData {};
    std::array<float, FFT_SIZE * 2> fifo    {};
    std::array<float, SCOPE_SIZE>   scopeData {};

    int  fifoIndex    = 0;
    bool nextFFTBlock = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumAnalyzer)
};
