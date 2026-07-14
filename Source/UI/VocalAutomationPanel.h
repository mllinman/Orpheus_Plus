#pragma once
#include <JuceHeader.h>
#include "../PitchCorrection/VocalSuiteProcessor.h"
#include "../Audio/AudioEngine.h"
#include "OrpheusLookAndFeel.h"

class MainComponent;

//==============================================================================
// VocalAutomationPanel — Dedicated glassmorphic vocal control surface.
// Provides granular knobs with live waveform feedback for all 10 vocal parameters.
//==============================================================================
class VocalAutomationPanel : public juce::Component,
                              private juce::Timer
{
public:
    VocalAutomationPanel(AudioEngine& engine, MainComponent* mainComp = nullptr);
    ~VocalAutomationPanel() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void recordAutomation(const juce::String& paramID, float value);
    void timerCallback() override;
    void paintGlassmorphicCard(juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& title);
    void paintVocalWaveform(juce::Graphics& g, juce::Rectangle<int> bounds);
    void paintParameterArc(juce::Graphics& g, juce::Rectangle<int> bounds,
                           float value, float minVal, float maxVal,
                           juce::Colour col, const juce::String& label, const juce::String& readout);

    AudioEngine& audioEngine;
    MainComponent* mainComponent = nullptr;

    // ── Enable Toggle ──
    juce::ToggleButton enableToggle { "ENABLE" };

    // ── Key / Scale ──
    juce::ComboBox keyCombo;
    juce::ComboBox scaleCombo;

    // ── Parameter Knobs ──
    struct VocalKnob {
        juce::Slider slider;
        juce::String name;
        juce::String unit;
        juce::Colour colour;
        float displayValue = 0.0f;
    };

    VocalKnob pitchKnob;
    VocalKnob volumeKnob;
    VocalKnob toneKnob;
    VocalKnob paceKnob;
    VocalKnob rhythmKnob;
    VocalKnob articulationKnob;
    VocalKnob resonanceKnob;
    VocalKnob inflectionKnob;
    VocalKnob emphasisKnob;
    VocalKnob projectionKnob;

    // ── Retune Speed ──
    juce::Slider retuneSpeedSlider;

    // ── Doubler / Harmony ──
    juce::Slider doublerSlider;
    juce::Slider harmonySlider;

    juce::TextButton smoothBtn{"Smooth Automation"};
    juce::TextButton clearBtn{"Clear Automation"};

    juce::LinearSmoothedValue<float> detectedPitch { 0.0f };
    juce::LinearSmoothedValue<float> correctedPitch { 0.0f };
    std::array<float, 128> waveformBuffer;
    int waveformWritePos = 0;
    int lastTrackIndex = -1;

    VocalSuiteProcessor* getActiveProcessor();
    void setupKnob(VocalKnob& vk, const juce::String& name, const juce::String& unit,
                   juce::Colour col, double min, double max, double def, double step);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VocalAutomationPanel)
};
