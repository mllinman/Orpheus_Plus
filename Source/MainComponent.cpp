#include <JuceHeader.h>
#include "MainComponent.h"
#include "StemSeparation/StemSeparator.h"
#include "AudioToMidi/AudioToMidiConverter.h"
#include "AudioCleanup/AudioCleanupProcessor.h"
#include "Util/OrpheusLogger.h"
#include "UI/AudioMidiSettingsPanel.h"
#include "AI/MixingAssistant.h"
#include "Timeline/AudioClip.h"
#include "UI/VoiceCloningPanel.h"
#include "UI/ProjectSettingsPanel.h"

//==============================================================================
MainComponent::MainComponent()
{
    // Initialize LookAndFeel
    juce::LookAndFeel::setDefaultLookAndFeel(&orpheusLookAndFeel);

    // Initialize AudioEngine
    audioEngine = std::make_unique<AudioEngine>();

    // Initialize ExportManager
    exportManager = std::make_unique<AudioExportManager>(*audioEngine);

    // Initialize ProjectManager
    projectManager = std::make_unique<ProjectManager>(appState, *audioEngine);

    // Initialize TransportBar
    transportBar = std::make_unique<TransportBar>(*audioEngine, commandManager);
    addAndMakeVisible(transportBar.get());

    // Initialize Workspace
    workspace = std::make_unique<WorkspaceManager>();
    addAndMakeVisible(workspace.get());

    auto addPanel = [this](const juce::String& name, 
                           std::unique_ptr<juce::Component> content,
                           WorkspaceManager::Zone zone = WorkspaceManager::Zone::Center) -> juce::Component* {
        auto* raw = content.get();
        auto panel = std::make_unique<DockablePanel>(name, std::move(content));
        workspace->addPanel(panel.get(), zone);
        dockablePanels.add(panel.release());
        return raw;
    };

    // Initialize Panels
    timeline        = dynamic_cast<TimelineComponent*>(addPanel("Timeline",   std::make_unique<TimelineComponent>(*audioEngine, appState, commandManager), WorkspaceManager::Zone::Center));
    pianoRoll       = dynamic_cast<PianoRollComponent*>(addPanel("Piano Roll", std::make_unique<PianoRollComponent>(appState, *audioEngine), WorkspaceManager::Zone::Center));
    masteringModule = dynamic_cast<MasteringModule*>(addPanel("Mastering",  std::make_unique<MasteringModule>(*audioEngine), WorkspaceManager::Zone::Right));
    if (masteringModule) {
        audioEngine->setMasteringModule(masteringModule);
        masteringModule->onPhaseAlign = [this]() {
            audioEngine->alignAllTracksPhase();
        };
    }
    stemSeparator   = dynamic_cast<StemSeparatorPanel*>(addPanel("Stem Sep",   std::make_unique<StemSeparatorPanel>(*audioEngine, appState), WorkspaceManager::Zone::Center));
    audioCleanup    = dynamic_cast<AudioCleanupPanel*>(addPanel("Cleanup",    std::make_unique<AudioCleanupPanel>(*audioEngine), WorkspaceManager::Zone::Center));
    aiHumanizer     = dynamic_cast<AIHumanizerPanel*>(addPanel("AI Humanizer", std::make_unique<AIHumanizerPanel>(*audioEngine, appState, this), WorkspaceManager::Zone::Center));
    
    // AutoTune requires special processor
    auto autoTuneComp = std::make_unique<AutoTunePanel>(*audioEngine);
    auto* rawAutoTune = autoTuneComp.get();
    autoTune = rawAutoTune;
    addPanel("AutoTune", std::move(autoTuneComp), WorkspaceManager::Zone::Right);

    pluginWorkspace = dynamic_cast<PluginWorkspacePanel*>(addPanel("VST Plugins", std::make_unique<PluginWorkspacePanel>(*audioEngine, appState), WorkspaceManager::Zone::Center));
    tablature       = dynamic_cast<TablaturePanel*>(addPanel("Tablature", std::make_unique<TablaturePanel>(*audioEngine), WorkspaceManager::Zone::Center));
    sessionView     = dynamic_cast<SessionViewPanel*>(addPanel("Session View", std::make_unique<SessionViewPanel>(*audioEngine), WorkspaceManager::Zone::Center));
    modulationMatrix = dynamic_cast<ModulationMatrixPanel*>(addPanel("Modulation", std::make_unique<ModulationMatrixPanel>(*audioEngine), WorkspaceManager::Zone::Bottom));
    vocalAutomation  = dynamic_cast<VocalAutomationPanel*>(addPanel("Vocal Auto", std::make_unique<VocalAutomationPanel>(*audioEngine, this), WorkspaceManager::Zone::Center));
    voiceCloning     = dynamic_cast<VoiceCloningPanel*>(addPanel("Voice Clone", std::make_unique<VoiceCloningPanel>(*audioEngine, this), WorkspaceManager::Zone::Center));
    userManual       = dynamic_cast<UserManualPanel*>(addPanel("User Manual", std::make_unique<UserManualPanel>(), WorkspaceManager::Zone::Center));
    pitchGame        = dynamic_cast<PitchGamePanel*>(addPanel("Pitch Game", std::make_unique<PitchGamePanel>(*audioEngine), WorkspaceManager::Zone::Center));
    macroControls    = dynamic_cast<MacroControlPanel*>(addPanel("Macro Controls", std::make_unique<MacroControlPanel>(), WorkspaceManager::Zone::Bottom));
    shortcutsSettings = dynamic_cast<ShortcutsSettingsPanel*>(addPanel("Shortcuts", std::make_unique<ShortcutsSettingsPanel>(commandManager), WorkspaceManager::Zone::Center));
    aiCoPilot        = dynamic_cast<AICoPilotPanel*>(addPanel("AI Co-Pilot", std::make_unique<AICoPilotPanel>(*audioEngine), WorkspaceManager::Zone::Right));
    adrPanel         = dynamic_cast<ADRPanel*>(addPanel("ADR / Foley", std::make_unique<ADRPanel>(*audioEngine), WorkspaceManager::Zone::Right));

    // Wire the Pitch Game to the AutoTune processor for live pitch data
    if (pitchGame != nullptr && autoTune != nullptr)
        pitchGame->setProcessor(&autoTune->getProcessor());

    // Initialize Sidebar / Mixer components
    mixerPanel = std::make_unique<MixerPanel>(*audioEngine, appState);
    
    addAndMakeVisible(projectSettingsBtn);
    projectSettingsBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    projectSettingsBtn.onClick = [this]() { toggleProjectSettings(); };

    addAndMakeVisible(exportBtn);
    exportBtn.setColour(juce::TextButton::buttonColourId, OrpheusLookAndFeel::accentPrimary());
    exportBtn.onClick = [this]() { showExportDialog(); };

    setupCallbacks();
    trackSettingsPanel = std::make_unique<TrackSettingsPanel>(*audioEngine, appState);
    libraryPanel = std::make_unique<LibraryPanel>(*audioEngine, appState);

    setWantsKeyboardFocus(true);
    setSize(1280, 800);
}

MainComponent::~MainComponent()
{
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(OrpheusLookAndFeel::bgDarkest());
}

void MainComponent::resized()
{
    auto area = getLocalBounds();

    if (transportBar)
        transportBar->setBounds(area.removeFromBottom(56));

    auto topBar = area.removeFromTop(40);
    auto settingsArea = topBar.removeFromRight(150);
    projectSettingsBtn.setBounds(settingsArea.removeFromLeft(100).reduced(0, 10));
    exportBtn.setBounds(settingsArea.removeFromRight(80).reduced(4, 10));

    if (workspace)
        workspace->setBounds(area);
}

void MainComponent::toggleProjectSettings()
{
    if (projectSettingsPanel == nullptr) {
        projectSettingsPanel = std::make_unique<ProjectSettingsPanel>(*projectManager);
        addChildComponent(projectSettingsPanel.get());
    }
    
    if (projectSettingsPanel->isVisible()) {
        projectSettingsPanel->setVisible(false);
    } else {
        projectSettingsPanel->setBounds(getLocalBounds().withSizeKeepingCentre(400, 500));
        projectSettingsPanel->setVisible(true);
        projectSettingsPanel->toFront(true);
    }
}

void MainComponent::showExportDialog()
{
    if (exportDialog == nullptr) {
        exportDialog = std::make_unique<ExportDialog>(*exportManager);
        addChildComponent(exportDialog.get());
        exportDialog->onCancel = [this]() { exportDialog->setVisible(false); };
        exportDialog->onExportStarted = [this]() { exportDialog->setVisible(false); };
    }
    
    exportDialog->setBounds(getLocalBounds().withSizeKeepingCentre(500, 560));
    exportDialog->setVisible(true);
    exportDialog->toFront(true);
}

void MainComponent::setupCallbacks()
{
}
