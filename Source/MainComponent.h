#pragma once
#include <JuceHeader.h>
#include "Project/AppState.h"
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
#include "UI/VoiceCloningPanel.h"
#include "UI/UserManualPanel.h"
#include "UI/PitchGamePanel.h"
#include "UI/MacroControlPanel.h"
#include "UI/ShortcutsSettingsPanel.h"
#include "UI/SettingsHubPanel.h"
#include "UI/ProjectSettingsPanel.h"
#include "UI/ADRPanel.h"
#include "UI/DistributionPrepPanel.h"
#include "UI/ExportDialog.h"
#include "UI/WorkspaceManager.h"
#include "UI/AICoPilotPanel.h"
#include "UI/ToolbarComponent.h"
#include "UI/StatusBar.h"
#include "UI/TextToSamplePanel.h"
#include "UI/AutoMixPanel.h"

class MainComponent : public juce::Component
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    bool hasUnsavedChanges() const { return appState.isDirty(); }
    juce::ApplicationCommandManager& getCommandManager() { return commandManager; }
    AutoTunePanel* getAutoTunePanel() { return autoTune; }
    AppState* getAppState() { return &appState; }

    enum CommandIDs
    {
        cmdSaveProject = 1000,
        cmdExportAudio = 1001
    };

    void toggleProjectSettings();
    void showExportDialog();
    void setupCallbacks();

private:
    void wireToolbarCallbacks();

    OrpheusLookAndFeel orpheusLookAndFeel;
    AppState appState;
    std::unique_ptr<AudioEngine> audioEngine;
    std::unique_ptr<AudioExportManager> exportManager;
    std::unique_ptr<ProjectManager> projectManager;
    juce::ApplicationCommandManager commandManager;

    // Layout components (top to bottom)
    std::unique_ptr<ToolbarComponent> toolbar;
    std::unique_ptr<TransportBar> transportBar;
    std::unique_ptr<WorkspaceManager> workspace;
    std::unique_ptr<StatusBar> statusBar;

    // Panel raw pointers (owned by workspace zones)
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
    VoiceCloningPanel* voiceCloning = nullptr;
    UserManualPanel* userManual = nullptr;
    PitchGamePanel* pitchGame = nullptr;
    MacroControlPanel* macroControls = nullptr;
    ShortcutsSettingsPanel* shortcutsSettings = nullptr;
    SettingsHubPanel* settingsHub = nullptr;
    AIHumanizerPanel* aiHumanizer = nullptr;
    AICoPilotPanel* aiCoPilot = nullptr;
    ADRPanel* adrPanel = nullptr;
    DistributionPrepPanel* distPrep = nullptr;

    // Owned panels (for bottom tabs / sidebars)
    std::unique_ptr<MixerPanel> mixerPanel;
    std::unique_ptr<SpectrumAnalyzer> spectrumAnalyzer;
    std::unique_ptr<TrackSettingsPanel> trackSettingsPanel;
    std::unique_ptr<LibraryPanel> libraryPanel;

    std::unique_ptr<ProjectSettingsPanel> projectSettingsPanel;
    std::unique_ptr<ExportDialog> exportDialog;

    // Tooltip support for toolbar hover descriptions
    std::unique_ptr<juce::TooltipWindow> tooltipWindow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
