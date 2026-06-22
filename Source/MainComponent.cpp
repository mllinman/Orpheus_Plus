#include <JuceHeader.h>
#include "MainComponent.h"
#include "StemSeparation/StemSeparator.h"
#include "AudioToMidi/AudioToMidiConverter.h"
#include "AudioCleanup/AudioCleanupProcessor.h"
#include "Util/OrpheusLogger.h"
#include "UI/AudioMidiSettingsPanel.h"
#include "AI/MixingAssistant.h"
#include "Timeline/AudioClip.h"

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

    //═══════════════════════════════════════════════════════════════════════
    //  TOP-LEVEL LAYOUT: Toolbar → Transport → Workspace → StatusBar
    //═══════════════════════════════════════════════════════════════════════

    // 1. Toolbar
    toolbar = std::make_unique<ToolbarComponent>();
    addAndMakeVisible(toolbar.get());

    // 2. Transport Bar
    transportBar = std::make_unique<TransportBar>(*audioEngine, commandManager);
    addAndMakeVisible(transportBar.get());

    // 3. Workspace Manager (5-zone layout)
    workspace = std::make_unique<WorkspaceManager>();
    addAndMakeVisible(workspace.get());

    // 4. Status Bar
    statusBar = std::make_unique<StatusBar>(*audioEngine);
    addAndMakeVisible(statusBar.get());

    //═══════════════════════════════════════════════════════════════════════
    //  TIMELINE ZONE (top half, permanent)
    //═══════════════════════════════════════════════════════════════════════
    auto timelineComp = std::make_unique<TimelineComponent>(*audioEngine, appState, commandManager);
    timeline = timelineComp.get();
    workspace->addToTimeline(timelineComp.release()); // Workspace takes ownership via addAndMakeVisible

    //═══════════════════════════════════════════════════════════════════════
    //  BOTTOM TABS ZONE — All editor/tool panels
    //═══════════════════════════════════════════════════════════════════════

    // Mixer (default first tab)
    mixerPanel = std::make_unique<MixerPanel>(*audioEngine, appState);
    workspace->addToBottomTab("Mixer", mixerPanel.get());

    // Piano Roll
    auto pianoRollComp = std::make_unique<PianoRollComponent>(appState, *audioEngine);
    pianoRoll = pianoRollComp.get();
    workspace->addToBottomTab("Piano Roll", pianoRollComp.release());

    // Session View
    auto sessionComp = std::make_unique<SessionViewPanel>(*audioEngine);
    sessionView = sessionComp.get();
    workspace->addToBottomTab("Session", sessionComp.release());

    // Stem Separation
    auto stemComp = std::make_unique<StemSeparatorPanel>(*audioEngine, appState);
    stemSeparator = stemComp.get();
    workspace->addToBottomTab("Stem Sep", stemComp.release());

    // Audio Cleanup
    auto cleanupComp = std::make_unique<AudioCleanupPanel>(*audioEngine);
    audioCleanup = cleanupComp.get();
    workspace->addToBottomTab("Cleanup", cleanupComp.release());

    // AI Humanizer
    auto humanizerComp = std::make_unique<AIHumanizerPanel>(*audioEngine, appState, this);
    aiHumanizer = humanizerComp.get();
    workspace->addToBottomTab("AI Humanizer", humanizerComp.release());

    // Distribution Prep
    auto distPrepComp = std::make_unique<DistributionPrepPanel>(*audioEngine, appState, this);
    distPrep = distPrepComp.get();
    workspace->addToBottomTab("Dist Prep", distPrepComp.release());

    // VST Plugins
    auto pluginComp = std::make_unique<PluginWorkspacePanel>(*audioEngine, appState);
    pluginWorkspace = pluginComp.get();
    workspace->addToBottomTab("VST Plugins", pluginComp.release());

    // Tablature
    auto tabComp = std::make_unique<TablaturePanel>(*audioEngine);
    tablature = tabComp.get();
    workspace->addToBottomTab("Tablature", tabComp.release());

    // Vocal Automation
    auto vocalComp = std::make_unique<VocalAutomationPanel>(*audioEngine, this);
    vocalAutomation = vocalComp.get();
    workspace->addToBottomTab("Vocal Auto", vocalComp.release());

    // Voice Cloning
    auto voiceComp = std::make_unique<VoiceCloningPanel>(*audioEngine, this);
    voiceCloning = voiceComp.get();
    workspace->addToBottomTab("Voice Clone", voiceComp.release());

    // Text-to-Sample
    auto ttsComp = std::make_unique<TextToSamplePanel>();
    workspace->addToBottomTab("Text-to-Sample", ttsComp.release());

    // Auto-Mix (needs masteringModule pointer, but it's not created yet — use nullptr, we'll set it after)
    auto autoMixComp = std::make_unique<AutoMixPanel>(audioEngine.get(), nullptr);
    workspace->addToBottomTab("Auto-Mix", autoMixComp.release());

    // Modulation Matrix
    auto modComp = std::make_unique<ModulationMatrixPanel>(*audioEngine);
    modulationMatrix = modComp.get();
    workspace->addToBottomTab("Modulation", modComp.release());

    // Macro Controls
    auto macroComp = std::make_unique<MacroControlPanel>();
    macroControls = macroComp.get();
    workspace->addToBottomTab("Macros", macroComp.release());

    // Shortcuts Settings
    auto shortcutsComp = std::make_unique<ShortcutsSettingsPanel>(commandManager);
    shortcutsSettings = shortcutsComp.get();
    workspace->addToBottomTab("Shortcuts", shortcutsComp.release());

    // Pitch Game
    auto pitchComp = std::make_unique<PitchGamePanel>(*audioEngine);
    pitchGame = pitchComp.get();
    workspace->addToBottomTab("Pitch Game", pitchComp.release());

    // User Manual
    auto manualComp = std::make_unique<UserManualPanel>();
    userManual = manualComp.get();
    workspace->addToBottomTab("User Manual", manualComp.release());

    //═══════════════════════════════════════════════════════════════════════
    //  LEFT SIDEBAR — Library + Track Settings
    //═══════════════════════════════════════════════════════════════════════
    libraryPanel = std::make_unique<LibraryPanel>(*audioEngine, appState);
    workspace->addToLeftSidebar("Library", std::make_unique<LibraryPanel>(*audioEngine, appState));

    trackSettingsPanel = std::make_unique<TrackSettingsPanel>(*audioEngine, appState);
    workspace->addToLeftSidebar("Track Settings", std::make_unique<TrackSettingsPanel>(*audioEngine, appState));

    //═══════════════════════════════════════════════════════════════════════
    //  RIGHT SIDEBAR — Mastering, AutoTune, Spectrum, AI CoPilot, ADR
    //═══════════════════════════════════════════════════════════════════════
    auto masterComp = std::make_unique<MasteringModule>(*audioEngine);
    masteringModule = masterComp.get();
    audioEngine->setMasteringModule(masteringModule);
    masteringModule->onPhaseAlign = [this]() { audioEngine->alignAllTracksPhase(); };
    workspace->addToRightSidebar("Mastering", std::move(masterComp));

    auto autoTuneComp = std::make_unique<AutoTunePanel>(*audioEngine);
    autoTune = autoTuneComp.get();
    workspace->addToRightSidebar("AutoTune", std::move(autoTuneComp));

    spectrumAnalyzer = std::make_unique<SpectrumAnalyzer>(*audioEngine);
    audioEngine->registerAnalyzer(spectrumAnalyzer.get());
    workspace->addToRightSidebar("Spectrum", std::make_unique<SpectrumAnalyzer>(*audioEngine));

    auto copilotComp = std::make_unique<AICoPilotPanel>(*audioEngine);
    aiCoPilot = copilotComp.get();
    workspace->addToRightSidebar("AI Co-Pilot", std::move(copilotComp));

    auto adrComp = std::make_unique<ADRPanel>(*audioEngine);
    adrPanel = adrComp.get();
    workspace->addToRightSidebar("ADR / Foley", std::move(adrComp));

    //═══════════════════════════════════════════════════════════════════════
    //  WIRING
    //═══════════════════════════════════════════════════════════════════════
    if (pitchGame != nullptr && autoTune != nullptr)
        pitchGame->setProcessor(&autoTune->getProcessor());

    wireToolbarCallbacks();
    setupCallbacks();

    setWantsKeyboardFocus(true);
    setSize(1440, 900);
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

    // Toolbar (top)
    if (toolbar)
        toolbar->setBounds(area.removeFromTop(46));

    // Transport (below toolbar)
    if (transportBar)
        transportBar->setBounds(area.removeFromTop(48));

    // Status bar (bottom)
    if (statusBar)
        statusBar->setBounds(area.removeFromBottom(22));

    // Workspace (fills remainder)
    if (workspace)
        workspace->setBounds(area);
}

void MainComponent::wireToolbarCallbacks()
{
    if (!toolbar) return;

    // File
    toolbar->onExport = [this]() { showExportDialog(); };
    toolbar->onShowSettings = [this]() { toggleProjectSettings(); };

    // View — switch bottom tabs
    toolbar->onShowMixer     = [this]() { workspace->showBottomTab("Mixer"); };
    toolbar->onShowPianoRoll = [this]() { workspace->showBottomTab("Piano Roll"); };
    toolbar->onShowSession   = [this]() { workspace->showBottomTab("Session"); };
    toolbar->onShowSpectrum  = [this]() { workspace->setRightSidebarVisible(true); };
    toolbar->onShowCoPilot   = [this]() { workspace->setRightSidebarVisible(true); };

    // AI panels
    toolbar->onShowStemSep      = [this]() { workspace->showBottomTab("Stem Sep"); };
    toolbar->onShowAutoTune     = [this]() { workspace->setRightSidebarVisible(true); };
    toolbar->onShowHumanizer    = [this]() { workspace->showBottomTab("AI Humanizer"); };
    toolbar->onShowTextToSample = [this]() { workspace->showBottomTab("Text-to-Sample"); };
    toolbar->onShowAutoMix      = [this]() { workspace->showBottomTab("Auto-Mix"); };
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
