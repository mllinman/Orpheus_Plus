#pragma once
#include <JuceHeader.h>
#include "../AudioCleanup/AIHumanizerProcessor.h"
#include "../Project/AppState.h"

class AudioEngine;
class MainComponent;

class AIHumanizerPanel : public juce::Component,
                         public juce::FileDragAndDropTarget,
                         public juce::Timer
{
public:
    AIHumanizerPanel(AudioEngine& engine, AppState& state, MainComponent* mc);
    ~AIHumanizerPanel() override;

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
    
    // An offline processor instance just for this panel (if not using the insert)
    AIHumanizerProcessor offlineProcessor;

    juce::Slider warmthSlider;
    juce::Slider flutterSlider;
    juce::Slider noiseFloorSlider;
    juce::Slider deChatterSlider;

    juce::Label warmthLabel;
    juce::Label flutterLabel;
    juce::Label noiseFloorLabel;
    juce::Label deChatterLabel;

    juce::TextButton applyToTrackButton { "Add as Realtime Insert to Selected Track" };

    bool isProcessing = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AIHumanizerPanel)
};
