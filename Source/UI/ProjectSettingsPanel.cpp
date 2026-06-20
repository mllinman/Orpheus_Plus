#include "ProjectSettingsPanel.h"

//==============================================================================
ProjectSettingsPanel::ProjectSettingsPanel(ProjectManager& pm)
    : projectManager(pm)
{
    addAndMakeVisible(titleLabel);
    titleLabel.setText("Project Settings", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(24.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centredLeft);

    addAndMakeVisible(directoryLabel);
    directoryLabel.setText("Default Project Directory:", juce::dontSendNotification);

    addAndMakeVisible(directoryPathEditor);
    directoryPathEditor.setReadOnly(true);
    directoryPathEditor.setText(projectManager.getDefaultProjectDirectory().getFullPathName());

    addAndMakeVisible(browseButton);
    browseButton.setButtonText("Browse...");
    browseButton.onClick = [this] {
        auto chooser = std::make_shared<juce::FileChooser>("Select Default Project Directory",
            projectManager.getDefaultProjectDirectory(),
            "");
        
        chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
            [this, chooser](const juce::FileChooser& fc) {
                if (fc.getResult().exists()) {
                    directoryPathEditor.setText(fc.getResult().getFullPathName());
                    projectManager.setDefaultProjectDirectory(fc.getResult());
                }
            });
    };

    addAndMakeVisible(copyAudioToggle);
    copyAudioToggle.setButtonText("Automatically copy used audio files to project's Audio folder on Save");
    copyAudioToggle.setToggleState(projectManager.getCopyAudioOnSave(), juce::dontSendNotification);
    copyAudioToggle.onClick = [this] {
        projectManager.setCopyAudioOnSave(copyAudioToggle.getToggleState());
    };
    
    setSize(600, 300);
}

ProjectSettingsPanel::~ProjectSettingsPanel()
{
}

void ProjectSettingsPanel::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void ProjectSettingsPanel::resized()
{
    auto area = getLocalBounds().reduced(20);
    
    titleLabel.setBounds(area.removeFromTop(40));
    area.removeFromTop(20);
    
    auto dirArea = area.removeFromTop(30);
    directoryLabel.setBounds(dirArea.removeFromLeft(150));
    browseButton.setBounds(dirArea.removeFromRight(100));
    dirArea.removeFromRight(10); // spacing
    directoryPathEditor.setBounds(dirArea);
    
    area.removeFromTop(20);
    copyAudioToggle.setBounds(area.removeFromTop(30));
}
