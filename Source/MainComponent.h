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
#include "UI/PluginWorkspacePanel.h"
#include "UI/TrackSettingsPanel.h"
#include "UI/StemSeparatorPanel.h"
#include "UI/AudioCleanupPanel.h"
#include "UI/AutoTunePanel.h"
#include "UI/AIHumanizerPanel.h"
#include "UI/TablaturePanel.h"
#include "UI/LibraryPanel.h"
#include "UI/SessionViewPanel.h"
#include "UI/ModulationMatrixPanel.h"
#include "UI/VocalAutomationPanel.h"
#include "UI/UserManualPanel.h"
#include "UI/PitchGamePanel.h"
#include "UI/MacroControlPanel.h"
#include "UI/ShortcutsSettingsPanel.h"
#include "Audio/AudioEngine.h"
#include "Export/AudioExportManager.h"
#include "Project/ProjectManager.h"
#include "Project/AppState.h"

#include "UI/DockablePanel.h"

class VoiceCloningPanel;

//==============================================================================
class MainComponent : public juce::Component,
                      public juce::DragAndDropContainer,
                      public juce::MenuBarModel,
                      public juce::ApplicationCommandTarget,
                      public juce::ChangeListener,
                      public DockablePanel::Listener,
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

    AutoTunePanel* getAutoTunePanel() const { return autoTune; }
    class VoiceCloningPanel* getVoiceCloningPanel() const { return voiceCloning; }
    AppState* getAppState() { return &appState; }

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
        cmdToggleLibraryPanel,
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
        cmdShowUserManual,
        cmdOpenAIHumanizer,
        cmdProjectSettings
    };

private:
    void timerCallback() override;
    void showStemSeparationDialog();
    void showAudioToMidiDialog();
    void showExportDialog();
    void showSettingsDialog();
    void showAudioCleanupDialog();
    void showAIHumanizerDialog();
    void showProjectSettingsDialog();
    void showAboutDialog();
    void updateLayout();
    void switchToView(int viewIndex);

    // Core systems
    OrpheusLookAndFeel orpheusLookAndFeel;
    AppState appState;
    std::unique_ptr<AudioEngine> audioEngine;
    std::unique_ptr<AudioExportManager> exportManager;
    std::unique_ptr<ProjectManager> projectManager;
    juce::ApplicationCommandManager commandManager;

    // Core UI Panels
    std::unique_ptr<juce::MenuBarComponent> menuBar;
    std::unique_ptr<TransportBar> transportBar;

    // View tab bar replacement
    juce::TabbedComponent mainTabbedView { juce::TabbedButtonBar::TabsAtTop };

    // DockablePanel::Listener
    void panelUndocked(DockablePanel* panel) override;
    void panelRedocked(DockablePanel* panel) override;

    // Wrapped panels (owners)
    std::unique_ptr<DockablePanel> timelinePanel;
    std::unique_ptr<DockablePanel> pianoRollPanel;
    std::unique_ptr<DockablePanel> masteringPanel;
    std::unique_ptr<DockablePanel> stemSeparatorPanel;
    std::unique_ptr<DockablePanel> audioCleanupPanel;
    std::unique_ptr<DockablePanel> autoTunePanel;
    std::unique_ptr<DockablePanel> pluginWorkspacePanel;
    std::unique_ptr<DockablePanel> tablaturePanel;
    std::unique_ptr<DockablePanel> sessionViewPanel;
    std::unique_ptr<DockablePanel> modulationPanel;
    std::unique_ptr<DockablePanel> vocalAutomationPanel;
    std::unique_ptr<DockablePanel> voiceCloningPanel;
    std::unique_ptr<DockablePanel> userManualDockablePanel;
    std::unique_ptr<DockablePanel> pitchGameDockablePanel;
    std::unique_ptr<DockablePanel> macroControlPanel;
    std::unique_ptr<DockablePanel> shortcutsPanel;
    std::unique_ptr<DockablePanel> aiHumanizerDockablePanel;

    // Component pointers (raw pointers for easy access)
    TimelineComponent* timeline = nullptr;
    PianoRollComponent* pianoRoll = nullptr;
    MasteringModule* masteringModule = nullptr;
    StemSeparatorPanel* stemSeparator = nullptr;
    AudioCleanupPanel* audioCleanup = nullptr;
    AutoTunePanel* autoTune = nullptr;
    PluginWorkspacePanel* pluginWorkspace = nullptr;
    TablaturePanel* tablature = nullptr;
    SessionViewPanel* sessionView = nullptr;
    ModulationMatrixPanel* modulationMatrix = nullptr;
    VocalAutomationPanel* vocalAutomation = nullptr;
    class VoiceCloningPanel* voiceCloning = nullptr;
    UserManualPanel* userManual = nullptr;
    PitchGamePanel* pitchGame = nullptr;
    MacroControlPanel* macroControls = nullptr;
    ShortcutsSettingsPanel* shortcutsSettings = nullptr;
    AIHumanizerPanel* aiHumanizer = nullptr;

    // Sidebars & Mixer (managed separately for now)
    std::unique_ptr<MixerPanel> mixerPanel;
    std::unique_ptr<SpectrumAnalyzer> spectrumAnalyzer;
    std::unique_ptr<TrackSettingsPanel> trackSettingsPanel;
    std::unique_ptr<LibraryPanel> libraryPanel;

    // Layout state
    bool showMixer          = true;
    bool showMastering      = false;
    bool showPianoRoll      = false;
    bool showTrackSettings  = false;
    bool showLibraryPanel   = false;
    int  currentView        = 0; // 0=Timeline, 1=PianoRoll, 2=Mastering, 3=StemSep, 4=Cleanup, 5=AutoTune
    int  mixerHeight        = 200;
    int  sidebarWidth       = 280;

    juce::StretchableLayoutManager verticalLayout;
    juce::StretchableLayoutManager horizontalLayout;
    
    std::unique_ptr<juce::StretchableLayoutResizerBar> verticalResizerBar;
    std::unique_ptr<juce::StretchableLayoutResizerBar> horizontalResizerBar;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
