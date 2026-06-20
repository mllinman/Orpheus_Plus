#pragma once
#include <JuceHeader.h>
#include "AppState.h"

class AudioEngine;

//==============================================================================
class ProjectManager
{
public:
    ProjectManager(AppState& state, AudioEngine& engine);
    ~ProjectManager();

    void newProject();
    void openProject();
    void saveProject();
    void saveProjectAs();
    bool loadProjectFile(const juce::File& file);
    bool saveProjectFile(const juce::File& file);

    juce::File getCurrentFile() const { return currentFile; }
    bool hasFile() const { return currentFile.existsAsFile(); }

    // Recent files
    juce::StringArray getRecentFiles() const;
    void addRecentFile(const juce::File& file);

    // Project Settings
    juce::File getDefaultProjectDirectory() const { return defaultProjectDir; }
    void setDefaultProjectDirectory(const juce::File& dir);

    bool getCopyAudioOnSave() const { return copyAudioOnSave; }
    void setCopyAudioOnSave(bool shouldCopy);

private:
    void loadSettings();
    void saveSettings();
    void copyAudioFilesToProjectFolder(const juce::File& projectFolder);
    AppState&    appState;
    AudioEngine& audioEngine;
    juce::File   currentFile;

    juce::File defaultProjectDir;
    bool copyAudioOnSave = true;

    static constexpr int MAX_RECENT_FILES = 10;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProjectManager)
};
