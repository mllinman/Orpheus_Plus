#pragma once
#include <JuceHeader.h>
#include "Audio/AudioEngine.h"
#include "Timeline/TimelineComponent.h"
#include "PianoRoll/PianoRollComponent.h"
#include "Mastering/MasteringModule.h"
#include "UI/OrpheusLookAndFeel.h"
#include "UI/TransportBar.h"
#include "UI/MixerPanel.h"
#include "UI/SpectrumAnalyzer.h"
#include "UI/PluginBrowser.h"
#include "UI/TrackSettingsPanel.h"
#include "UI/StemSeparatorPanel.h"
#include "UI/AudioCleanupPanel.h"
#include "UI/AutoTunePanel.h"
#include "Project/ProjectManager.h"
#include "Project/AppState.h"

//==============================================================================
class MainComponent : public juce::Component,
                      public juce::DragAndDropContainer,
                      public juce::MenuBarModel,
                      public juce::ApplicationCommandTarget,
                      public juce::ChangeListener,
                      private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

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
        cmdOpenSettings,
        cmdToggleMixer,
        cmdShowTimeline,
        cmdAudioCleanup,
        cmdAbout,
        cmdQuit,
        cmdOpenAutoTune,
        cmdToggleTrackSettings,
        cmdAddVocalTrack,
        cmdAddInstrumentTrack,
        cmdAddFolderTrack,
        cmdAddArrangerTrack,
        cmdCaptureMidi,
        cmdToggleTempoFollower,
        cmdSwitchTheme,
        cmdShowMixingAssistant,
        cmdFreezeTrack,
        cmdScratchPadSave,
        cmdScratchPadLoad,
        cmdImportAudio,
    };

private:
    void timerCallback() override;
    void showStemSeparationDialog();
    void showAudioToMidiDialog();
    void showExportDialog();
    void showSettingsDialog();
    void showAudioCleanupDialog();
    void showAboutDialog();
    void updateLayout();
    void switchToView(int viewIndex);

    // Core systems
    OrpheusLookAndFeel orpheusLookAndFeel;
    AppState appState;
    std::unique_ptr<AudioEngine> audioEngine;
    std::unique_ptr<ProjectManager> projectManager;
    juce::ApplicationCommandManager commandManager;

    // Main UI panels
    std::unique_ptr<juce::MenuBarComponent> menuBar;
    std::unique_ptr<TransportBar> transportBar;
    std::unique_ptr<TimelineComponent> timeline;
    std::unique_ptr<MixerPanel> mixerPanel;
    std::unique_ptr<SpectrumAnalyzer> spectrumAnalyzer;
    std::unique_ptr<PianoRollComponent> pianoRoll;
    std::unique_ptr<MasteringModule> masteringModule;
    std::unique_ptr<PluginBrowser> pluginBrowser;

    // New panels
    std::unique_ptr<TrackSettingsPanel> trackSettingsPanel;
    std::unique_ptr<StemSeparatorPanel> stemSeparatorPanel;
    std::unique_ptr<AudioCleanupPanel> audioCleanupPanel;
    std::unique_ptr<AutoTunePanel> autoTunePanel;

    // View tab bar
    juce::TextButton tabTimeline   { "Timeline" };
    juce::TextButton tabPianoRoll  { "Piano Roll" };
    juce::TextButton tabMastering  { "Mastering" };
    juce::TextButton tabStemSep    { "Stem Sep" };
    juce::TextButton tabCleanup    { "Cleanup" };
    juce::TextButton tabAutoTune   { "AutoTune" };

    // Layout state
    bool showMixer          = true;
    bool showMastering      = false;
    bool showPianoRoll      = false;
    bool showPluginBrowser  = false;
    bool showTrackSettings  = false;
    int  currentView        = 0; // 0=Timeline, 1=PianoRoll, 2=Mastering, 3=StemSep, 4=Cleanup, 5=AutoTune
    int  mixerHeight        = 200;
    int  sidebarWidth       = 280;

    juce::StretchableLayoutManager verticalLayout;
    juce::StretchableLayoutManager horizontalLayout;
    
    std::unique_ptr<juce::StretchableLayoutResizerBar> verticalResizerBar;
    std::unique_ptr<juce::StretchableLayoutResizerBar> horizontalResizerBar;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
