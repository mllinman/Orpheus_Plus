#pragma once
#include <JuceHeader.h>
#include "../Project/AppState.h"
#include "../Audio/AudioEngine.h"

class LibraryPanel : public juce::Component,
                     public juce::FileBrowserListener
{
public:
    LibraryPanel(AudioEngine& engine, AppState& state);
    ~LibraryPanel() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    // FileBrowserListener
    void selectionChanged() override;
    void fileClicked (const juce::File& file, const juce::MouseEvent& e) override;
    void fileDoubleClicked (const juce::File& file) override;
    void browserRootChanged (const juce::File& newRoot) override;

private:
    AudioEngine& audioEngine;
    AppState&    appState;

    juce::TimeSliceThread thread { "Library Scanner Thread" };
    juce::WildcardFileFilter filter { "*", "*", "Library Files" };
    juce::DirectoryContentsList directoryList { &filter, thread };
    juce::FileTreeComponent fileTree { directoryList };

    juce::Label titleLabel { "LibraryTitle", "Library" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LibraryPanel)
};
