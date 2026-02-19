#include "SpectrumAnalyzer.h"

SpectrumAnalyzer::SpectrumAnalyzer(AudioEngine& e) : audioEngine(e)
{
    startTimerHz(30);
}
SpectrumAnalyzer::~SpectrumAnalyzer() { stopTimer(); }

void SpectrumAnalyzer::timerCallback()
{
    if (nextFFTBlock)
    {
        fftData = fifo;
        window.multiplyWithWindowingTable(fftData.data(), FFT_SIZE);
        fft.performFrequencyOnlyForwardTransform(fftData.data());

        auto mindB = -100.0f, maxdB = 0.0f;
        for (int i = 0; i < SCOPE_SIZE; ++i)
        {
            float skewedX = std::pow((float)i / SCOPE_SIZE, 0.3f);
            int   fftIdx  = juce::jlimit(0, FFT_SIZE / 2,
                                         (int)(skewedX * (float)(FFT_SIZE / 2)));
            float level   = juce::jmap(juce::Decibels::gainToDecibels(fftData[fftIdx]),
                                       mindB, maxdB, 0.0f, 1.0f);
            scopeData[i]  = level;
        }
        nextFFTBlock = false;
        repaint();
    }
}

void SpectrumAnalyzer::resized() {}

void SpectrumAnalyzer::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    g.fillAll(juce::Colour(0xff0a0a14));

    // Grid lines at octave intervals
    g.setColour(juce::Colour(0xff1a1a2e));
    for (int freq : { 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 })
    {
        float normX = std::log10((float)freq / 20.0f) / std::log10(20000.0f / 20.0f);
        int x = (int)(normX * bounds.getWidth());
        g.drawVerticalLine(x, 0.0f, (float)bounds.getHeight());

        g.setColour(juce::Colour(0xff333355));
        g.setFont(8.0f);
        juce::String label = freq < 1000 ? juce::String(freq) : juce::String(freq / 1000) + "k";
        g.drawText(label, x + 2, bounds.getHeight() - 14, 30, 12, juce::Justification::left);
        g.setColour(juce::Colour(0xff1a1a2e));
    }

    // Spectrum curve
    juce::Path spectrum;
    spectrum.startNewSubPath(0, (float)bounds.getHeight());

    for (int i = 0; i < SCOPE_SIZE; ++i)
    {
        float x = juce::jmap((float)i, 0.0f, (float)SCOPE_SIZE - 1,
                             0.0f, (float)bounds.getWidth());
        float y = juce::jmap(scopeData[i], 0.0f, 1.0f,
                             (float)bounds.getHeight(), 0.0f);
        if (i == 0)
            spectrum.startNewSubPath(x, y);
        else
            spectrum.lineTo(x, y);
    }

    spectrum.lineTo((float)bounds.getWidth(), (float)bounds.getHeight());
    spectrum.closeSubPath();

    juce::ColourGradient grad(juce::Colour(0xff533483).withAlpha(0.7f), 0, 0,
                              juce::Colour(0xff4fc3f7).withAlpha(0.4f), (float)bounds.getWidth(), 0, false);
    g.setGradientFill(grad);
    g.fillPath(spectrum);

    g.setColour(juce::Colour(0xff7b2d8b));
    g.strokePath(spectrum, juce::PathStrokeType(1.5f));
}
