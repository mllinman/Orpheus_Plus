#pragma once
#include <JuceHeader.h>
#include "Audio/AudioEngine.h"
#include "Timeline/TimelineComponent.h"
// #include "Timeline/TransportController.h"
// #include "PianoRoll/PianoRollComponent.h"
// #include "Mastering/MasteringModule.h"
#include "UI/OrpheusLookAndFeel.h"
// ...
    // Core systems

#include "UI/TransportBar.h"
// #include "UI/MixerPanel.h"
// #include "UI/SpectrumAnalyzer.h"
// #include "UI/PluginBrowser.h"
#include "Project/ProjectManager.h"
#include "Project/AppState.h"

//==============================================================================
class MainComponent : public juce::Component,
                      public juce::MenuBarModel,
                      public juce::ApplicationCommandTarget,
                      private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    bool hasUnsavedChanges() const;

    // Menu bar
    juce::StringArray getMenuBarNames() override;
    juce::PopupMenu getMenuForIndex(int menuIndex, const juce::String& menuName) override;
    void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;

    // Commands
    juce::ApplicationCommandTarget* getNextCommandTarget() override { return nullptr; }
    void getAllCommands(juce::Array<juce::CommandID>&) override;
    void getCommandInfo(juce::CommandID, juce::ApplicationCommandInfo&) override;

    bool perform(const InvocationInfo&) override;

    juce::ApplicationCommandManager& getCommandManager();

    enum CommandIDs
    {
        cmdNewProject = 1,
        cmdOpenProject,
        cmdSaveProject,
        cmdSaveProjectAs,
        cmdUndo,
        cmdRedo,
        cmdPlay,
        cmdStop,
        cmdRecord,
        cmdAddAudioTrack,
        cmdAddMidiTrack,
        cmdOpenMastering,
        cmdOpenStemSeparation,
        cmdAudioToMidi,
        cmdOpenPianoRoll,
        cmdExportMix,
        cmdExportStems,
        cmdOpenPluginBrowser,
    };

private:
    void timerCallback() override;
    void showStemSeparationDialog();
    void showAudioToMidiDialog();
    void showExportDialog();
    void updateLayout();

    // Core systems
    // OrpheusLookAndFeel lookAndFeel;
    OrpheusLookAndFeel orpheusLookAndFeel;
    AppState appState;
    std::unique_ptr<AudioEngine> audioEngine;
    std::unique_ptr<ProjectManager> projectManager;
    juce::ApplicationCommandManager commandManager;

    // Main UI panels
    std::unique_ptr<juce::MenuBarComponent> menuBar;
    std::unique_ptr<TransportBar> transportBar;
    std::unique_ptr<TimelineComponent> timeline;
    // std::unique_ptr<MixerPanel> mixerPanel;
    // std::unique_ptr<SpectrumAnalyzer> spectrumAnalyzer;
    // std::unique_ptr<PianoRollComponent> pianoRoll;
    // std::unique_ptr<MasteringModule> masteringModule;
    // std::unique_ptr<PluginBrowser> pluginBrowser;

    // Layout state
    bool showMixer        = true;
    bool showMastering    = false;
    bool showPianoRoll    = false;
    bool showPluginBrowser = false;

    juce::StretchableLayoutManager mainLayout;
    juce::StretchableLayoutResizerBar* resizerBar = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
