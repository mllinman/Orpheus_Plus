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

    // 1. Menu Bar
    menuBar = std::make_unique<juce::MenuBarComponent>(this);
    addAndMakeVisible(menuBar.get());

    // 2. Toolbar
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

    // 5. Tooltip window — enables hover descriptions for all SettableTooltipClient items
    tooltipWindow = std::make_unique<juce::TooltipWindow>(this, 400);

    //═══════════════════════════════════════════════════════════════════════
    //  TIMELINE ZONE (top half, permanent)
    //═══════════════════════════════════════════════════════════════════════
    auto timelineComp = std::make_unique<TimelineComponent>(*audioEngine, appState, commandManager);
    timeline = timelineComp.get();
    workspace->addToTimeline(timelineComp.release()); // Workspace takes ownership via addAndMakeVisible

    //═══════════════════════════════════════════════════════════════════════
    //  BOTTOM TABS ZONE — All editor/tool panels (colour-coded by category)
    //═══════════════════════════════════════════════════════════════════════

    // Category colours
    auto colCore   = juce::Colour(0xff06b6d4);  // Cyan — core views
    auto colAudio  = juce::Colour(0xff10b981);  // Emerald — audio tools
    auto colVST    = juce::Colour(0xff8b5cf6);  // Violet — plugins
    auto colComp   = juce::Colour(0xfff59e0b);  // Amber — composition
    auto colAI     = juce::Colour(0xffec4899);  // Pink — AI
    auto colUtil   = juce::Colour(0xffa1a1aa);  // Zinc — utility

    // ── Core Views ────────────────────────────────────────────────────────
    mixerPanel = std::make_unique<MixerPanel>(*audioEngine, appState);
    workspace->addToBottomTab("Mixer", colCore, mixerPanel.get());

    auto pianoRollComp = std::make_unique<PianoRollComponent>(appState, *audioEngine);
    pianoRoll = pianoRollComp.get();
    workspace->addToBottomTab("Piano Roll", colCore, pianoRollComp.release());

    auto sessionComp = std::make_unique<SessionViewPanel>(*audioEngine);
    sessionView = sessionComp.get();
    workspace->addToBottomTab("Session", colCore, sessionComp.release());

    // ── Audio Tools ───────────────────────────────────────────────────────
    auto stemComp = std::make_unique<StemSeparatorPanel>(*audioEngine, appState);
    stemSeparator = stemComp.get();
    workspace->addToBottomTab("Stem Sep", colAudio, stemComp.release());

    auto cleanupComp = std::make_unique<AudioCleanupPanel>(*audioEngine);
    audioCleanup = cleanupComp.get();
    workspace->addToBottomTab("Cleanup", colAudio, cleanupComp.release());

    auto humanizerComp = std::make_unique<AIHumanizerPanel>(*audioEngine, appState, this);
    aiHumanizer = humanizerComp.get();
    workspace->addToBottomTab("AI Humanizer", colAudio, humanizerComp.release());

    auto distPrepComp = std::make_unique<DistributionPrepPanel>(*audioEngine, appState, this);
    distPrep = distPrepComp.get();
    workspace->addToBottomTab("Dist Prep", colAudio, distPrepComp.release());

    // ── VST Plugins ───────────────────────────────────────────────────────
    auto pluginComp = std::make_unique<PluginWorkspacePanel>(*audioEngine, appState);
    pluginWorkspace = pluginComp.get();
    workspace->addToBottomTab("VST Plugins", colVST, pluginComp.release());

    // ── Composition ───────────────────────────────────────────────────────
    auto tabComp = std::make_unique<TablaturePanel>(*audioEngine);
    tablature = tabComp.get();
    workspace->addToBottomTab("Tablature", colComp, tabComp.release());

    auto vocalComp = std::make_unique<VocalAutomationPanel>(*audioEngine, this);
    vocalAutomation = vocalComp.get();
    workspace->addToBottomTab("Vocal Auto", colComp, vocalComp.release());

    auto modComp = std::make_unique<ModulationMatrixPanel>(*audioEngine);
    modulationMatrix = modComp.get();
    workspace->addToBottomTab("Modulation", colComp, modComp.release());

    auto macroComp = std::make_unique<MacroControlPanel>();
    macroControls = macroComp.get();
    workspace->addToBottomTab("Macros", colComp, macroComp.release());

    // ── AI Tools ──────────────────────────────────────────────────────────
    auto voiceComp = std::make_unique<VoiceCloningPanel>(*audioEngine, this);
    voiceCloning = voiceComp.get();
    workspace->addToBottomTab("Voice Clone", colAI, voiceComp.release());

    auto ttsComp = std::make_unique<TextToSamplePanel>();
    workspace->addToBottomTab("Text-to-Sample", colAI, ttsComp.release());

    auto autoMixComp = std::make_unique<AutoMixPanel>(audioEngine.get(), nullptr);
    workspace->addToBottomTab("Auto-Mix", colAI, autoMixComp.release());

    auto smartFixComp = std::make_unique<SmartTrackFixerPanel>(*audioEngine);
    smartTrackFixer = smartFixComp.get();
    workspace->addToBottomTab("Track Fixer", colAI, smartFixComp.release());

    // ── Utility ───────────────────────────────────────────────────────────
    auto shortcutsComp = std::make_unique<ShortcutsSettingsPanel>(commandManager);
    shortcutsSettings = shortcutsComp.get();
    workspace->addToBottomTab("Shortcuts", colUtil, shortcutsComp.release());

    auto settingsComp = std::make_unique<SettingsHubPanel>(*audioEngine, appState, *projectManager, commandManager);
    settingsHub = settingsComp.get();
    workspace->addToBottomTab("Settings", colUtil, settingsComp.release());

    auto pitchComp = std::make_unique<PitchGamePanel>(*audioEngine);
    pitchGame = pitchComp.get();
    workspace->addToBottomTab("Pitch Game", colUtil, pitchComp.release());

    auto manualComp = std::make_unique<UserManualPanel>();
    userManual = manualComp.get();
    workspace->addToBottomTab("User Manual", colUtil, manualComp.release());

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
    menuBar->setModel(nullptr);
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(OrpheusLookAndFeel::bgDarkest());
}

void MainComponent::resized()
{
    auto area = getLocalBounds();

    // Menu bar (top)
    if (menuBar)
        menuBar->setBounds(area.removeFromTop(24));

    // Toolbar (below menu)
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

    toolbar->onNewProject  = [this]() { projectManager->newProject(); };
    toolbar->onOpenProject = [this]() { projectManager->openProject(); };
    toolbar->onSaveProject = [this]() { projectManager->saveProject(); };
    toolbar->onExport = [this]() { showExportDialog(); };
    toolbar->onShowSettings = [this]() { workspace->showBottomTab("Settings"); };

    // Edit — undo, redo, clipboard
    toolbar->onUndo  = [this]() { commandManager.invokeDirectly(juce::StandardApplicationCommandIDs::undo, true); };
    toolbar->onRedo  = [this]() { commandManager.invokeDirectly(juce::StandardApplicationCommandIDs::redo, true); };
    toolbar->onCut   = [this]() { commandManager.invokeDirectly(juce::StandardApplicationCommandIDs::cut,  true); };
    toolbar->onCopy  = [this]() { commandManager.invokeDirectly(juce::StandardApplicationCommandIDs::copy, true); };
    toolbar->onPaste = [this]() { commandManager.invokeDirectly(juce::StandardApplicationCommandIDs::paste, true); };

    // Tools — editing modes (maps toolbar buttons to TimelineComponent::EditTool)
    toolbar->onSelectTool = [this]() { if (timeline) timeline->setTool(TimelineComponent::EditTool::Select); };
    toolbar->onDrawTool   = [this]() { if (timeline) timeline->setTool(TimelineComponent::EditTool::Draw); };
    toolbar->onSliceTool  = [this]() { if (timeline) timeline->setTool(TimelineComponent::EditTool::Split); };
    toolbar->onEraserTool = [this]() { if (timeline) timeline->setTool(TimelineComponent::EditTool::Erase); };
    toolbar->onMuteTool   = [this]() { if (timeline) timeline->setTool(TimelineComponent::EditTool::Select); };

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

//==============================================================================
// Menu Bar Implementation
//==============================================================================
juce::StringArray MainComponent::getMenuBarNames()
{
    return { "File", "Edit", "View", "AI", "Help" };
}

juce::PopupMenu MainComponent::getMenuForIndex(int menuIndex, const juce::String& /*menuName*/)
{
    juce::PopupMenu menu;

    if (menuIndex == 0) // File
    {
        menu.addItem(100, "New Project",       true, false);
        menu.addItem(101, "Open Project...",   true, false);
        menu.addSeparator();
        menu.addItem(102, "Save",              true, false);
        menu.addItem(103, "Save As...",        true, false);
        menu.addSeparator();
        menu.addItem(110, "Import Audio...",   true, false);
        menu.addItem(111, "Import MIDI...",    true, false);
        menu.addSeparator();
        menu.addItem(120, "Export Audio...",   true, false);
        menu.addItem(121, "Export MIDI...",    true, false);
        menu.addItem(122, "Export Stems...",   true, false);
        menu.addSeparator();

        // Recent files submenu
        juce::PopupMenu recentMenu;
        auto recentFiles = projectManager->getRecentFiles();
        for (int i = 0; i < recentFiles.size(); ++i)
            recentMenu.addItem(200 + i, juce::File(recentFiles[i]).getFileName());
        if (recentFiles.isEmpty())
            recentMenu.addItem(299, "(No recent files)", false);
        menu.addSubMenu("Recent Projects", recentMenu);

        menu.addSeparator();
        menu.addItem(199, "Settings", true, false);
    }
    else if (menuIndex == 1) // Edit
    {
        menu.addItem(300, "Undo",     true, false);
        menu.addItem(301, "Redo",     true, false);
        menu.addSeparator();
        menu.addItem(310, "Cut",      true, false);
        menu.addItem(311, "Copy",     true, false);
        menu.addItem(312, "Paste",    true, false);
        menu.addItem(313, "Delete",   true, false);
        menu.addSeparator();
        menu.addItem(320, "Select All", true, false);
        menu.addItem(321, "Deselect All", true, false);
    }
    else if (menuIndex == 2) // View
    {
        menu.addItem(400, "Mixer",           true, false);
        menu.addItem(401, "Piano Roll",      true, false);
        menu.addItem(402, "Session View",    true, false);
        menu.addSeparator();
        menu.addItem(410, "Left Sidebar",    true, workspace ? workspace->isLeftSidebarVisible() : false);
        menu.addItem(411, "Right Sidebar",   true, workspace ? workspace->isRightSidebarVisible() : false);
        menu.addSeparator();

        // Reopen closed tabs
        if (workspace)
        {
            auto closedNames = workspace->getClosedTabNames();
            if (!closedNames.isEmpty())
            {
                juce::PopupMenu reopenMenu;
                for (int i = 0; i < closedNames.size(); ++i)
                    reopenMenu.addItem(500 + i, closedNames[i]);
                menu.addSubMenu("Reopen Closed Tab", reopenMenu);
            }
        }

        menu.addSeparator();
        menu.addItem(420, "Zoom In",  true, false);
        menu.addItem(421, "Zoom Out", true, false);
    }
    else if (menuIndex == 3) // AI
    {
        menu.addItem(600, "Stem Separation",   true, false);
        menu.addItem(601, "AutoTune",          true, false);
        menu.addItem(602, "AI Humanizer",      true, false);
        menu.addItem(603, "Auto-Mix",          true, false);
        menu.addItem(604, "Text-to-Sample",    true, false);
        menu.addItem(605, "Voice Clone",       true, false);
        menu.addSeparator();
        menu.addItem(610, "AI Co-Pilot",       true, false);
        menu.addItem(611, "Distribution Prep", true, false);
    }
    else if (menuIndex == 4) // Help
    {
        menu.addItem(700, "User Manual",       true, false);
        menu.addItem(701, "Keyboard Shortcuts", true, false);
        menu.addSeparator();
        menu.addItem(710, "About Orpheus Plus", true, false);
    }

    return menu;
}

void MainComponent::menuItemSelected(int menuItemID, int /*topLevelMenuIndex*/)
{
    // Handle range-based IDs first (recent files, closed tabs)
    if (menuItemID >= 200 && menuItemID < 299)
    {
        auto recentFiles = projectManager->getRecentFiles();
        int idx = menuItemID - 200;
        if (idx < recentFiles.size())
            projectManager->loadProjectFile(juce::File(recentFiles[idx]));
        return;
    }
    if (menuItemID >= 500 && menuItemID < 600)
    {
        if (workspace)
        {
            auto closedNames = workspace->getClosedTabNames();
            int idx = menuItemID - 500;
            if (idx < closedNames.size())
                workspace->reopenBottomTab(closedNames[idx]);
        }
        return;
    }

    switch (menuItemID)
    {
        // ── File ─────────────────────────────────────────────────────────
        case 100: projectManager->newProject();    break;
        case 101: projectManager->openProject();   break;
        case 102: projectManager->saveProject();   break;
        case 103: projectManager->saveProjectAs(); break;
        case 110: importAudioFile();                break;
        case 111: importMidiFile();                 break;
        case 120: showExportDialog();               break;
        case 121: /* Export MIDI — TODO */          break;
        case 122: /* Export Stems — TODO */         break;
        case 199: workspace->showBottomTab("Settings"); break;

        // ── Edit ─────────────────────────────────────────────────────────
        case 300: commandManager.invokeDirectly(juce::StandardApplicationCommandIDs::undo, true);  break;
        case 301: commandManager.invokeDirectly(juce::StandardApplicationCommandIDs::redo, true);  break;
        case 310: commandManager.invokeDirectly(juce::StandardApplicationCommandIDs::cut,  true);  break;
        case 311: commandManager.invokeDirectly(juce::StandardApplicationCommandIDs::copy, true);  break;
        case 312: commandManager.invokeDirectly(juce::StandardApplicationCommandIDs::paste, true); break;
        case 313: commandManager.invokeDirectly(juce::StandardApplicationCommandIDs::del, true);   break;
        case 320: commandManager.invokeDirectly(juce::StandardApplicationCommandIDs::selectAll, true); break;
        case 321: if (timeline) timeline->clearAllSelections(); break;

        // ── View ─────────────────────────────────────────────────────────
        case 400: workspace->showBottomTab("Mixer");      break;
        case 401: workspace->showBottomTab("Piano Roll");  break;
        case 402: workspace->showBottomTab("Session");     break;
        case 410: workspace->setLeftSidebarVisible(!workspace->isLeftSidebarVisible());   break;
        case 411: workspace->setRightSidebarVisible(!workspace->isRightSidebarVisible()); break;
        case 420: if (timeline) timeline->zoomIn();  break;
        case 421: if (timeline) timeline->zoomOut(); break;

        // ── AI ───────────────────────────────────────────────────────────
        case 600: workspace->showBottomTab("Stem Sep");        break;
        case 601: workspace->setRightSidebarVisible(true);     break;
        case 602: workspace->showBottomTab("AI Humanizer");    break;
        case 603: workspace->showBottomTab("Auto-Mix");        break;
        case 604: workspace->showBottomTab("Text-to-Sample");  break;
        case 605: workspace->showBottomTab("Voice Clone");     break;
        case 610: workspace->setRightSidebarVisible(true);     break;
        case 611: workspace->showBottomTab("Dist Prep");       break;

        // ── Help ─────────────────────────────────────────────────────────
        case 700: workspace->showBottomTab("User Manual");  break;
        case 701: workspace->showBottomTab("Settings");     break;
        case 710: workspace->showBottomTab("Settings");     break;
        default: break;
    }
}

void MainComponent::importAudioFile()
{
    if (importChooser) return;
    importChooser = std::make_unique<juce::FileChooser>(
        "Import Audio File",
        juce::File::getSpecialLocation(juce::File::userMusicDirectory),
        "*.wav;*.aiff;*.flac;*.mp3;*.ogg");

    importChooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles
        | juce::FileBrowserComponent::canSelectMultipleItems,
        [this](const juce::FileChooser& fc)
        {
            auto results = fc.getResults();
            for (auto& file : results)
            {
                if (file.existsAsFile())
                {
                    int trackIdx = audioEngine->addAudioTrack(file.getFileNameWithoutExtension());
                    auto& trackInfo = audioEngine->getTrackInfo(trackIdx);

                    auto* clip = new AudioClip(file, 0.0);
                    clip->loadAudioData(audioEngine->getFormatManager());
                    trackInfo.clips.add(clip);

                    // Auto-detect BPM and update session if first track
                    if (audioEngine->getTrackCount() == 1 && clip->sourceBpm > 0 && clip->sourceBpm != 120.0)
                        audioEngine->setBpm(clip->sourceBpm);
                }
            }
            if (timeline) timeline->rebuildTracks();
            juce::MessageManager::callAsync([this]() { importChooser.reset(); });
        });
}

void MainComponent::importMidiFile()
{
    if (importChooser) return;
    importChooser = std::make_unique<juce::FileChooser>(
        "Import MIDI File",
        juce::File::getSpecialLocation(juce::File::userMusicDirectory),
        "*.mid;*.midi");

    importChooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc)
        {
            auto result = fc.getResult();
            if (result.existsAsFile())
            {
                int trackIdx = audioEngine->addMidiTrack(result.getFileNameWithoutExtension());
                auto& trackInfo = audioEngine->getTrackInfo(trackIdx);

                // Load the MIDI file and add clips
                juce::FileInputStream stream(result);
                if (stream.openedOk())
                {
                    juce::MidiFile midiFile;
                    midiFile.readFrom(stream);
                    midiFile.convertTimestampTicksToSeconds();

                    for (int t = 0; t < midiFile.getNumTracks(); ++t)
                    {
                        auto* midiTrack = midiFile.getTrack(t);
                        if (midiTrack && midiTrack->getNumEvents() > 0)
                        {
                            double clipStart = midiTrack->getStartTime();
                            double clipDur = midiTrack->getEndTime() - clipStart;
                            if (clipDur <= 0.0) clipDur = 4.0;

                            auto* clip = new MidiClip(clipStart, clipDur);
                            clip->midiData = *midiTrack;
                            trackInfo.clips.add(clip);
                        }
                    }
                }
                if (timeline) timeline->rebuildTracks();
            }
            juce::MessageManager::callAsync([this]() { importChooser.reset(); });
        });
}
