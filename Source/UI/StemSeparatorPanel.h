#pragma once
#include <JuceHeader.h>
#include "../StemSeparation/StemSeparator.h"
#include "../Audio/AudioEngine.h"
#include "../Project/AppState.h"
#include "OrpheusLookAndFeel.h"

//==============================================================================
class StemSeparatorPanel : public juce::Component,
                            public juce::FileDragAndDropTarget,
                            public StemSeparator::Listener,
                            private juce::Timer
{
public:
    StemSeparatorPanel(AudioEngine& engine, AppState& state);
    ~StemSeparatorPanel() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    // FileDragAndDropTarget
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    void fileDragEnter(const juce::StringArray&, int, int) override { dragHover = true; repaint(); }
    void fileDragExit(const juce::StringArray&) override { dragHover = false; repaint(); }

    // StemSeparator::Listener
    void stemSeparationProgress(float progress) override;
    void stemSeparationComplete(const StemSeparationResult& result) override;
    void stemSeparationFailed(const juce::String& error) override;

private:
    void timerCallback() override;
    void startSeparation(const juce::File& file);
    void paintDropZone(juce::Graphics& g, juce::Rectangle<int> bounds);
    void paintProgressRing(juce::Graphics& g, juce::Rectangle<int> bounds);
    void paintStemCard(juce::Graphics& g, juce::Rectangle<int> bounds,
                       const juce::String& name, const juce::File& file, int index);

    AudioEngine& audioEngine;
    AppState&    appState;
    StemSeparator stemSeparator;

    // Model selector
    juce::Label    modelLabel { {}, "MODEL" };
    juce::ComboBox modelCombo;

    // Browse button
    juce::TextButton browseButton { "Browse Audio File..." };

    // Quality
    juce::Label        qualityLabel { {}, "QUALITY" };
    juce::ToggleButton highQualityToggle { "HQ Mode" };

    // Action
    juce::TextButton separateButton { "Separate Stems" };
    juce::TextButton cancelButton   { "Cancel" };

    // State
    bool  dragHover = false;
    juce::File selectedFile;
    StemSeparationResult lastResult;
    bool  hasResult = false;
    float currentProgress = 0.0f;
    bool  isProcessing = false;
    juce::String errorMessage;

    // Stem result controls
    struct StemCard {
        juce::TextButton soloButton { "S" };
        juce::TextButton muteButton { "M" };
        juce::Slider     volumeSlider;
        juce::TextButton addToTimeline { "Add to Timeline" };
    };
    std::array<StemCard, 6> stemCards;
    static constexpr const char* stemNames[6] = {
        "Vocals", "Drums", "Bass", "Guitar", "Piano", "Other"
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StemSeparatorPanel)
};
