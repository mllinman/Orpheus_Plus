#pragma once
#include <JuceHeader.h>
#include "../AudioCleanup/DistributionPrepProcessor.h"
#include "../Project/AppState.h"

class AudioEngine;
class MainComponent;

//==============================================================================
// DistributionPrepPanel — UI for metadata stripping, loudness normalization,
// and platform-specific format compliance.
//==============================================================================
class DistributionPrepPanel : public juce::Component,
                               public juce::FileDragAndDropTarget,
                               public juce::Timer
{
public:
    DistributionPrepPanel(AudioEngine& engine, AppState& state, MainComponent* mc);
    ~DistributionPrepPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void timerCallback() override;

    // FileDragAndDropTarget overrides
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

private:
    AudioEngine& audioEngine;
    AppState& appState;
    MainComponent* mainComponent;

    DistributionPrepProcessor processor;

    //── Platform selector ────────────────────────────────────────────────────
    juce::ComboBox platformCombo;
    juce::Label    platformLabel;

    //── Metadata fields ──────────────────────────────────────────────────────
    juce::TextEditor titleField;
    juce::Label      titleLabel;
    juce::TextEditor artistField;
    juce::Label      artistLabel;
    juce::ToggleButton stripAllToggle { "Strip All Metadata" };

    //── Spec display ─────────────────────────────────────────────────────────
    juce::Label lufsDisplay;
    juce::Label truePeakDisplay;
    juce::Label sampleRateDisplay;
    juce::Label bitDepthDisplay;

    //── Actions ──────────────────────────────────────────────────────────────
    juce::TextButton prepareButton { "Prepare for Distribution" };
    juce::TextButton analyzeButton { "Analyze File" };

    //── State ────────────────────────────────────────────────────────────────
    bool isProcessing { false };
    bool isDragHover  { false };
    juce::String statusMessage;
    juce::File lastDroppedFile;

    void updateSpecDisplay(DistributionPrepProcessor::Platform platform);
    DistributionPrepProcessor::Platform getSelectedPlatform() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DistributionPrepPanel)
};
