#pragma once
#include <JuceHeader.h>
#include "../Audio/AudioEngine.h"

//==============================================================================
class TransportBar : public juce::Component,
                     public AudioEngine::Listener,
                     private juce::Timer
{
public:
    TransportBar(AudioEngine& engine, juce::ApplicationCommandManager& commands);
    ~TransportBar() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    void updatePositionDisplay();

private:
    void timerCallback() override;
    void playbackStarted() override;
    void playbackStopped() override;

    AudioEngine& audioEngine;
    juce::ApplicationCommandManager& commandManager;

    // Transport buttons
    juce::TextButton rewindButton    { "|◀" };
    juce::TextButton playButton      { "▶" };
    juce::TextButton stopButton      { "■" };
    juce::TextButton recordButton    { "●" };
    juce::TextButton loopButton      { "⟳" };

    // BPM
    juce::Label  bpmLabel         { {}, "BPM" };
    juce::Slider bpmSlider;

    // Time signature
    juce::ComboBox timeSigNumerator;
    juce::ComboBox timeSigDenominator;

    // Position display
    juce::Label positionLabel;
    juce::Label barBeatLabel;

    // Master volume
    juce::Label  masterVolumeLabel { {}, "MASTER" };
    juce::Slider masterVolumeSlider;

    // Level meters
    juce::Component meterL, meterR;
    float peakL = 0.0f, peakR = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportBar)
};
