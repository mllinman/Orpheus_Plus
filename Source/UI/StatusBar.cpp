#include "StatusBar.h"

StatusBar::StatusBar(AudioEngine& engine)
    : audioEngine(engine)
{
    startTimerHz(4); // Update 4x per second
}

StatusBar::~StatusBar()
{
    stopTimer();
}

void StatusBar::timerCallback()
{
    cpuUsage = audioEngine.getDeviceManager().getCpuUsage() * 100.0f;
    lufsValue = audioEngine.getMasterLUFS();
    
    if (auto* device = audioEngine.getDeviceManager().getCurrentAudioDevice())
    {
        sampleRate = device->getCurrentSampleRate();
        int blockSize = device->getCurrentBufferSizeSamples();
        latencyMs = (int)(blockSize * 1000.0 / sampleRate);
    }
    
    repaint();
}

void StatusBar::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Background
    g.setColour(OrpheusLookAndFeel::bgDarkest());
    g.fillRect(bounds);
    
    // Top border
    g.setColour(OrpheusLookAndFeel::borderSubtle());
    g.drawHorizontalLine(0, 0, bounds.getWidth());

    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 10.5f, juce::Font::plain));

    auto drawMetric = [&](float x, float w, const juce::String& label, const juce::String& value, juce::Colour valueColor)
    {
        auto area = juce::Rectangle<float>(x, 0, w, bounds.getHeight());
        g.setColour(OrpheusLookAndFeel::textMuted());
        g.drawText(label, area.withTrimmedRight(area.getWidth() * 0.5f), juce::Justification::centredRight);
        g.setColour(valueColor);
        g.drawText(value, area.withTrimmedLeft(area.getWidth() * 0.5f).withTrimmedLeft(4), juce::Justification::centredLeft);
    };

    float x = 12;
    float mw = 140;

    // CPU
    juce::Colour cpuColor = cpuUsage < 50 ? OrpheusLookAndFeel::accentSuccess()
                          : cpuUsage < 80 ? OrpheusLookAndFeel::accentWarning()
                          : OrpheusLookAndFeel::accentDanger();
    drawMetric(x, mw, "CPU:", juce::String(cpuUsage, 1) + "%", cpuColor);
    x += mw;

    // LUFS
    juce::Colour lufsColor = lufsValue > -6 ? OrpheusLookAndFeel::accentDanger()
                           : lufsValue > -14 ? OrpheusLookAndFeel::accentWarning()
                           : OrpheusLookAndFeel::accentSuccess();
    drawMetric(x, mw, "LUFS:", (lufsValue < -60 ? "--" : juce::String(lufsValue, 1)), lufsColor);
    x += mw;

    // Latency
    drawMetric(x, mw, "Latency:", juce::String(latencyMs) + " ms", OrpheusLookAndFeel::textSecondary());
    x += mw;

    // Sample rate
    juce::String srStr = juce::String((int)sampleRate / 1000) + "." + juce::String(((int)sampleRate % 1000) / 100) + " kHz";
    drawMetric(x, mw, "Rate:", srStr, OrpheusLookAndFeel::textSecondary());
}
