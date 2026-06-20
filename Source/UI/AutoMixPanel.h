#pragma once
#include <JuceHeader.h>
#include "DockablePanel.h"
#include "../AI/AutoMixer.h"
#include "../Mastering/MasteringModule.h"

class AutoMixPanel : public juce::Component, public juce::FileDragAndDropTarget
{
public:
    AutoMixPanel(AudioEngine* engine, MasteringModule* master);
    ~AutoMixPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // FileDragAndDropTarget overrides
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

private:
    AudioEngine* audioEngine;
    MasteringModule* masteringModule;
    AutoMixer autoMixer;

    juce::TextButton runMixButton{"Run Auto-Mix"};
    juce::Label referenceLabel{"refLabel", "Drag & Drop Reference Track Here"};
    
    juce::File currentReferenceFile;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutoMixPanel)
};
