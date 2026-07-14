#pragma once
#include <JuceHeader.h>
#include "../Audio/AudioEngine.h"
#include "OrpheusLookAndFeel.h"

//==============================================================================
// StatusBar — Thin bar at the bottom showing CPU, RAM, LUFS, latency, sample rate.
//==============================================================================
class StatusBar : public juce::Component, private juce::Timer
{
public:
    StatusBar(AudioEngine& engine);
    ~StatusBar() override;

    void paint(juce::Graphics&) override;
    void resized() override {}

    // Display a temporary status message (auto-clears after ~3 seconds)
    void setStatus(const juce::String& message)
    {
        statusMessage = message;
        statusClearCountdown = 6; // ~3 seconds at 500ms timer interval
        repaint();
    }

private:
    void timerCallback() override;

    AudioEngine& audioEngine;
    float cpuUsage = 0.0f;
    float lufsValue = -70.0f;
    int latencyMs = 0;
    double sampleRate = 48000.0;
    juce::String statusMessage;
    int statusClearCountdown = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StatusBar)
};
