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
#include "UI/ProjectSettingsPanel.h"
#include "UI/ADRPanel.h"
#include "UI/ExportDialog.h"
#include "UI/WorkspaceManager.h"
#include "UI/AICoPilotPanel.h"

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
    OrpheusLookAndFeel orpheusLookAndFeel;
    AppState appState;
    std::unique_ptr<AudioEngine> audioEngine;
    std::unique_ptr<AudioExportManager> exportManager;
    std::unique_ptr<ProjectManager> projectManager;
    juce::ApplicationCommandManager commandManager;

    std::unique_ptr<WorkspaceManager> workspace;
    
    std::unique_ptr<TransportBar> transportBar;

    TimelineComponent* timeline;
    PianoRollComponent* pianoRoll;
    MasteringModule* masteringModule;
    StemSeparatorPanel* stemSeparator;
    AudioCleanupPanel* audioCleanup;
    AutoTunePanel* autoTune;
    PluginWorkspacePanel* pluginWorkspace;
    TablaturePanel* tablature;
    SessionViewPanel* sessionView;
    ModulationMatrixPanel* modulationMatrix;
    VocalAutomationPanel* vocalAutomation;
    VoiceCloningPanel* voiceCloning;
    UserManualPanel* userManual;
    PitchGamePanel* pitchGame;
    MacroControlPanel* macroControls;
    ShortcutsSettingsPanel* shortcutsSettings;
    AIHumanizerPanel* aiHumanizer;
    AICoPilotPanel* aiCoPilot;
    ADRPanel* adrPanel;

    std::unique_ptr<MixerPanel> mixerPanel;
    std::unique_ptr<SpectrumAnalyzer> spectrumAnalyzer;
    std::unique_ptr<TrackSettingsPanel> trackSettingsPanel;
    std::unique_ptr<LibraryPanel> libraryPanel;

    std::unique_ptr<ProjectSettingsPanel> projectSettingsPanel;
    std::unique_ptr<ExportDialog> exportDialog;

    juce::TextButton projectSettingsBtn { "Project Settings" };
    juce::TextButton exportBtn { "Export" };

    juce::OwnedArray<DockablePanel> dockablePanels;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
