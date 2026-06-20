#pragma once

#include <JuceHeader.h>
#include "../Project/ProjectManager.h"

//==============================================================================
class ProjectSettingsPanel  : public juce::Component
{
public:
    ProjectSettingsPanel(ProjectManager& pm);
    ~ProjectSettingsPanel() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    ProjectManager& projectManager;

    juce::Label titleLabel;
    juce::Label directoryLabel;
    juce::TextEditor directoryPathEditor;
    juce::TextButton browseButton;
    juce::ToggleButton copyAudioToggle;
    juce::TextButton saveSettingsButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProjectSettingsPanel)
};
