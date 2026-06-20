#pragma once
#include <JuceHeader.h>
#include "DockablePanel.h"
#include "../Audio/AudioEngine.h"
#include "../VoiceCloning/SpeakerEmbeddingExtractor.h"

class VoiceCloningPanel : public juce::Component, public juce::Timer
{
public:
    VoiceCloningPanel(AudioEngine& engine, class MainComponent* mainComp = nullptr);
    ~VoiceCloningPanel() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void loadReferenceSong();
    void processUploadedSong(const juce::File& file);
    void paintGlassmorphicCard(juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& title);

    AudioEngine& audioEngine;
    MainComponent* mainComponent = nullptr;

    SpeakerEmbeddingExtractor embeddingExtractor;

    juce::TextButton loadBtn { "Load Reference Song" };
    juce::ToggleButton enableToggle { "Enable Voice Clone" };
    juce::ToggleButton preserveTimingToggle { "Preserve Exact Timing" };
    
    juce::Slider timbreMixSlider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    juce::Slider pitchShiftSlider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };

    juce::Label statusLabel;
    
    std::unique_ptr<juce::FileChooser> fChooser;
    juce::File currentReferenceFile;
    float extractionProgress = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoiceCloningPanel)
};
