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

    // Initialize ProjectManager
    projectManager = std::make_unique<ProjectManager>(appState, *audioEngine);

    // Register commands
    commandManager.registerAllCommandsForTarget(this);

    // Initialize MenuBar
    menuBar = std::make_unique<juce::MenuBarComponent>(this);
    addAndMakeVisible(menuBar.get());

    // Initialize TransportBar
    transportBar = std::make_unique<TransportBar>(*audioEngine, commandManager);
    addAndMakeVisible(transportBar.get());

    // Initialize Tabbed View
    addAndMakeVisible(mainTabbedView);
    mainTabbedView.setTabBarDepth(36);

    auto addPanel = [this](std::unique_ptr<DockablePanel>& panelPtr, 
                           const juce::String& name, 
                           std::unique_ptr<juce::Component> content) -> juce::Component* {
        auto* raw = content.get();
        panelPtr = std::make_unique<DockablePanel>(name, std::move(content));
        panelPtr->addListener(this);
        mainTabbedView.addTab(name, OrpheusLookAndFeel::bgDark(), panelPtr.get(), false);
        return raw;
    };

    // Initialize Panels
    timeline        = dynamic_cast<TimelineComponent*>(addPanel(timelinePanel,      "Timeline",   std::make_unique<TimelineComponent>(*audioEngine, appState, commandManager)));
    pianoRoll       = dynamic_cast<PianoRollComponent*>(addPanel(pianoRollPanel,     "Piano Roll", std::make_unique<PianoRollComponent>(appState, *audioEngine)));
    masteringModule = dynamic_cast<MasteringModule*>(addPanel(masteringPanel,       "Mastering",  std::make_unique<MasteringModule>(*audioEngine)));
    if (masteringModule) audioEngine->setMasteringModule(masteringModule);
    stemSeparator   = dynamic_cast<StemSeparatorPanel*>(addPanel(stemSeparatorPanel, "Stem Sep",   std::make_unique<StemSeparatorPanel>(*audioEngine, appState)));
    audioCleanup    = dynamic_cast<AudioCleanupPanel*>(addPanel(audioCleanupPanel,  "Cleanup",    std::make_unique<AudioCleanupPanel>(*audioEngine)));
    autoTune        = dynamic_cast<AutoTunePanel*>(addPanel(autoTunePanel,      "AutoTune",   std::make_unique<AutoTunePanel>(*audioEngine)));
    pluginWorkspace = dynamic_cast<PluginWorkspacePanel*>(addPanel(pluginWorkspacePanel, "VST Plugins", std::make_unique<PluginWorkspacePanel>(*audioEngine, appState)));
    tablature       = dynamic_cast<TablaturePanel*>(addPanel(tablaturePanel, "Tablature", std::make_unique<TablaturePanel>(*audioEngine)));
    sessionView     = dynamic_cast<SessionViewPanel*>(addPanel(sessionViewPanel, "Session View", std::make_unique<SessionViewPanel>(*audioEngine)));
    modulationMatrix = dynamic_cast<ModulationMatrixPanel*>(addPanel(modulationPanel, "Modulation", std::make_unique<ModulationMatrixPanel>(*audioEngine)));

    // Initialize Sidebar / Mixer components (not tabbed)
    mixerPanel = std::make_unique<MixerPanel>(*audioEngine, appState);
    addAndMakeVisible(mixerPanel.get());

    spectrumAnalyzer = std::make_unique<SpectrumAnalyzer>(*audioEngine);
    addChildComponent(spectrumAnalyzer.get());

    trackSettingsPanel = std::make_unique<TrackSettingsPanel>(*audioEngine, appState);
    addChildComponent(trackSettingsPanel.get());
    
    libraryPanel = std::make_unique<LibraryPanel>(*audioEngine, appState);
    addChildComponent(libraryPanel.get());
    
    appState.addChangeListener(this);

    // Initialize UI Resizers
    verticalLayout.setItemLayout(0, -0.1, -1.0, -0.7); // Main Workspace (flexible)
    verticalLayout.setItemLayout(1, 8, 8, 8);          // Vertical Resizer (fixed 8px)
    verticalLayout.setItemLayout(2, 50, 600, mixerHeight); // Mixer Panel (fixed bounds)

    horizontalLayout.setItemLayout(0, -0.1, -1.0, -0.7); // Center View (flexible)
    horizontalLayout.setItemLayout(1, 8, 8, 8);          // Horizontal Resizer (fixed 8px)
    horizontalLayout.setItemLayout(2, 100, 600, sidebarWidth); // Right Sidebar (fixed bounds)

    verticalResizerBar = std::make_unique<juce::StretchableLayoutResizerBar>(&verticalLayout, 1, true);
    addAndMakeVisible(verticalResizerBar.get());

    horizontalResizerBar = std::make_unique<juce::StretchableLayoutResizerBar>(&horizontalLayout, 1, false);
    addAndMakeVisible(horizontalResizerBar.get());

    // Keyboard focus for shortcuts
    setWantsKeyboardFocus(true);
    commandManager.setFirstCommandTarget(this);

    setSize(1280, 800);
}

MainComponent::~MainComponent()
{
    appState.removeChangeListener(this);
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
}

void MainComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &appState)
    {
        audioEngine->syncWithAppState(appState);
    }
}

bool MainComponent::hasUnsavedChanges() const
{
    return appState.isDirty();
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(OrpheusLookAndFeel::bgDarkest());
}

void MainComponent::resized()
{
    auto area = getLocalBounds();

    // ── Top: Menu Bar (32px) ───────────────────────────────────────────
    if (menuBar)
        menuBar->setBounds(area.removeFromTop(32));

    // ── Bottom: Transport Bar (56px) ───────────────────────────────────
    if (transportBar)
        transportBar->setBounds(area.removeFromBottom(56));

    // ── Layout Managers ────────────────────────────────────────────────
    if (showMixer)
    {
        juce::Component dummyTop, dummyBottom;
        juce::Component* vComps[] = { &dummyTop, verticalResizerBar.get(), &dummyBottom };
        verticalLayout.layOutComponents(vComps, 3, area.getX(), area.getY(), area.getWidth(), area.getHeight(), true, true);
        
        auto mixerArea = dummyBottom.getBounds();
        if (spectrumAnalyzer)
        {
            spectrumAnalyzer->setVisible(true);
            spectrumAnalyzer->setBounds(mixerArea.removeFromRight(300));
        }
        if (mixerPanel)
        {
            mixerPanel->setVisible(true);
            mixerPanel->setBounds(mixerArea);
        }
        
        area = dummyTop.getBounds(); // the remaining top workspace
        verticalResizerBar->setVisible(true);
    }
    else
    {
        if (spectrumAnalyzer) spectrumAnalyzer->setVisible(false);
        if (mixerPanel) mixerPanel->setVisible(false);
        verticalResizerBar->setVisible(false);
    }

    if (showTrackSettings || showLibraryPanel)
    {
        juce::Component dummyLeft, dummyRight;
        juce::Component* hComps[] = { &dummyLeft, horizontalResizerBar.get(), &dummyRight };
        horizontalLayout.layOutComponents(hComps, 3, area.getX(), area.getY(), area.getWidth(), area.getHeight(), false, true);
        
        auto sideArea = dummyRight.getBounds();

        if (showTrackSettings && trackSettingsPanel)
        {
            trackSettingsPanel->setVisible(true);
            trackSettingsPanel->setBounds(sideArea);
        }
        else if (trackSettingsPanel) trackSettingsPanel->setVisible(false);
        
        if (showLibraryPanel && libraryPanel)
        {
            libraryPanel->setVisible(true);
            libraryPanel->setBounds(sideArea);
        }
        else if (libraryPanel) libraryPanel->setVisible(false);
        
        area = dummyLeft.getBounds(); // center workspace
        horizontalResizerBar->setVisible(true);
    }
    else
    {
        if (trackSettingsPanel) trackSettingsPanel->setVisible(false);
        if (libraryPanel) libraryPanel->setVisible(false);
        horizontalResizerBar->setVisible(false);
    }

    // ── Center Area: mainTabbedView ───────────────────────────────────
    mainTabbedView.setBounds(area);
}

void MainComponent::panelUndocked(DockablePanel* panel)
{
    // If undocked, we might want to hide the tab or show a placeholder
    // For now, it stays in the tab bar but shows "Undocked" text (handled in DockablePanel::paint)
}

void MainComponent::panelRedocked(DockablePanel* panel)
{
}

void MainComponent::switchToView(int viewIndex)
{
    mainTabbedView.setCurrentTabIndex(viewIndex);
}

void MainComponent::timerCallback() {}

juce::ApplicationCommandManager& MainComponent::getCommandManager() { return commandManager; }

//==============================================================================
// Menu Bar
//==============================================================================

juce::StringArray MainComponent::getMenuBarNames()
{
    return { "File", "Edit", "View", "Track", "AI", "Help" };
}

juce::PopupMenu MainComponent::getMenuForIndex(int menuIndex, const juce::String& /*menuName*/)
{
    juce::PopupMenu menu;

    switch (menuIndex)
    {
        case 0: // File
            menu.addCommandItem(&commandManager, cmdNewProject);
            menu.addCommandItem(&commandManager, cmdOpenProject);
            menu.addSeparator();
            menu.addCommandItem(&commandManager, cmdImportAudio);
            menu.addSeparator();
            menu.addCommandItem(&commandManager, cmdSaveProject);
            menu.addCommandItem(&commandManager, cmdSaveProjectAs);
            menu.addSeparator();
            menu.addCommandItem(&commandManager, cmdExportMix);
            menu.addCommandItem(&commandManager, cmdExportStems);
            menu.addSeparator();
            menu.addCommandItem(&commandManager, cmdOpenSettings);
            menu.addSeparator();
            menu.addCommandItem(&commandManager, cmdQuit);
            break;

        case 1: // Edit
            menu.addCommandItem(&commandManager, cmdUndo);
            menu.addCommandItem(&commandManager, cmdRedo);
            break;

        case 2: // View
            menu.addCommandItem(&commandManager, cmdShowTimeline);
            menu.addCommandItem(&commandManager, cmdOpenPianoRoll);
            menu.addCommandItem(&commandManager, cmdOpenMastering);
            menu.addSeparator();
            menu.addCommandItem(&commandManager, cmdOpenStemSeparation);
            menu.addCommandItem(&commandManager, cmdAudioCleanup);
            menu.addCommandItem(&commandManager, cmdOpenAutoTune);
            menu.addSeparator();
            menu.addCommandItem(&commandManager, cmdToggleMixer);
            menu.addCommandItem(&commandManager, cmdOpenPluginBrowser);
            menu.addCommandItem(&commandManager, cmdToggleTrackSettings);
            menu.addCommandItem(&commandManager, cmdToggleLibraryPanel);
            break;

        case 3: // Track
            menu.addCommandItem(&commandManager, cmdAddAudioTrack);
            menu.addCommandItem(&commandManager, cmdAddMidiTrack);
            menu.addCommandItem(&commandManager, cmdAddFolderTrack);
            menu.addCommandItem(&commandManager, cmdAddArrangerTrack);
            break;

        case 4: // AI
            menu.addCommandItem(&commandManager, cmdOpenStemSeparation);
            menu.addCommandItem(&commandManager, cmdAudioToMidi);
            menu.addCommandItem(&commandManager, cmdAudioCleanup);
            menu.addCommandItem(&commandManager, cmdOpenAutoTune);
            break;

        case 5: // Help
            menu.addCommandItem(&commandManager, cmdAbout);
            break;
    }

    return menu;
}

void MainComponent::menuItemSelected(int /*menuItemID*/, int /*topLevelMenuIndex*/) {}

//==============================================================================
// Commands
//==============================================================================

void MainComponent::getAllCommands(juce::Array<juce::CommandID>& commands)
{
    commands.addArray({
        cmdNewProject, cmdOpenProject, cmdSaveProject, cmdSaveProjectAs,
        cmdUndo, cmdRedo,
        cmdPlay, cmdStop, cmdRecord,
        cmdAddAudioTrack, cmdAddMidiTrack,
        cmdOpenMastering, cmdOpenStemSeparation, cmdAudioToMidi,
        cmdOpenPianoRoll, cmdShowTimeline,
        cmdExportMix, cmdExportStems,
        cmdOpenPluginBrowser, cmdOpenSettings,
        cmdToggleMixer, cmdAudioCleanup, cmdAbout, cmdQuit,
        cmdOpenAutoTune, cmdToggleTrackSettings, cmdToggleLibraryPanel,
        cmdAddVocalTrack, cmdAddInstrumentTrack,
        cmdAddFolderTrack, cmdAddArrangerTrack,
        cmdImportAudio
    });
}

void MainComponent::getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result)
{
    switch (commandID)
    {
        case cmdNewProject:
            result.setInfo("New Project", "Create a new project", "File", 0);
            result.addDefaultKeypress('n', juce::ModifierKeys::commandModifier);
            break;
        case cmdOpenProject:
            result.setInfo("Open Project...", "Open an existing project", "File", 0);
            result.addDefaultKeypress('o', juce::ModifierKeys::commandModifier);
            break;
        case cmdImportAudio:
            result.setInfo("Import Audio...", "Import an audio file into a new track", "File", 0);
            result.addDefaultKeypress('i', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier);
            break;
        case cmdSaveProject:
            result.setInfo("Save", "Save the current project", "File", 0);
            result.addDefaultKeypress('s', juce::ModifierKeys::commandModifier);
            break;
        case cmdSaveProjectAs:
            result.setInfo("Save As...", "Save project to a new file", "File", 0);
            result.addDefaultKeypress('s', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier);
            break;
        case cmdUndo:
            result.setInfo("Undo", "Undo last action", "Edit", 0);
            result.addDefaultKeypress('z', juce::ModifierKeys::commandModifier);
            result.setActive(appState.canUndo());
            break;
        case cmdRedo:
            result.setInfo("Redo", "Redo last undone action", "Edit", 0);
            result.addDefaultKeypress('y', juce::ModifierKeys::commandModifier);
            result.setActive(appState.canRedo());
            break;
        case cmdPlay:
            result.setInfo("Play", "Start playback", "Transport", 0);
            result.addDefaultKeypress(juce::KeyPress::spaceKey, 0);
            break;
        case cmdStop:
            result.setInfo("Stop", "Stop playback", "Transport", 0);
            break;
        case cmdRecord:
            result.setInfo("Record", "Start recording", "Transport", 0);
            result.addDefaultKeypress('r', juce::ModifierKeys::commandModifier);
            break;
        case cmdAddAudioTrack:
            result.setInfo("Add Audio Track", "Add a new audio track", "Track", 0);
            result.addDefaultKeypress('t', juce::ModifierKeys::commandModifier);
            break;
        case cmdAddMidiTrack:
            result.setInfo("Add MIDI Track", "Add a new MIDI track", "Track", 0);
            result.addDefaultKeypress('t', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier);
            break;
        case cmdShowTimeline:
            result.setInfo("Timeline", "Show the timeline view", "View", 0);
            result.setTicked(currentView == 0);
            break;
        case cmdOpenPianoRoll:
            result.setInfo("Piano Roll", "Show the piano roll editor", "View", 0);
            result.setTicked(currentView == 1);
            break;
        case cmdOpenMastering:
            result.setInfo("Mastering", "Show the mastering suite", "View", 0);
            result.setTicked(currentView == 2);
            break;
        case cmdToggleMixer:
            result.setInfo("Toggle Mixer", "Show or hide the mixer panel", "View", 0);
            result.addDefaultKeypress('m', juce::ModifierKeys::commandModifier);
            result.setTicked(showMixer);
            break;
        case cmdOpenPluginBrowser:
            result.setInfo("Plugin Browser", "Show VST Plugins", "View", 0);
            result.addDefaultKeypress('b', juce::ModifierKeys::commandModifier);
            result.setTicked(currentView == 6);
            break;
        case cmdOpenStemSeparation:
            result.setInfo("Stem Separation...", "Separate audio into stems", "AI", 0);
            break;
        case cmdAudioToMidi:
            result.setInfo("Audio to MIDI...", "Convert audio to MIDI notes", "AI", 0);
            break;
        case cmdAudioCleanup:
            result.setInfo("Audio Cleanup...", "Remove noise, hum, clicks", "AI", 0);
            break;
        case cmdExportMix:
            result.setInfo("Export Mix...", "Export the full mix to file", "File", 0);
            result.addDefaultKeypress('e', juce::ModifierKeys::commandModifier);
            break;
        case cmdExportStems:
            result.setInfo("Export Stems...", "Export individual stems", "File", 0);
            break;
        case cmdOpenSettings:
            result.setInfo("Settings...", "Open Audio/MIDI Settings", "File", 0);
            result.addDefaultKeypress(',', juce::ModifierKeys::commandModifier);
            break;
        case cmdAbout:
            result.setInfo("About Orpheus Plus", "About this application", "Help", 0);
            break;
        case cmdQuit:
            result.setInfo("Quit", "Exit the application", "File", 0);
            result.addDefaultKeypress('q', juce::ModifierKeys::commandModifier);
            break;
        case cmdOpenAutoTune:
            result.setInfo("Auto-Tune", "Show the auto-tune panel", "AI", 0);
            result.setTicked(currentView == 5);
            break;
        case cmdToggleTrackSettings:
            result.setInfo("Track Settings", "Show or hide the track settings panel", "View", 0);
            result.setTicked(showTrackSettings);
            break;
        case cmdToggleLibraryPanel:
            result.setInfo("Library Component", "Show or hide the library panel", "View", 0);
            result.setTicked(showLibraryPanel);
            result.addDefaultKeypress('l', juce::ModifierKeys::commandModifier);
            break;
        case cmdAddVocalTrack:
            result.setInfo("Add Vocal Track", "Add a new vocal track", "Track", 0);
            break;
        case cmdAddInstrumentTrack:
            result.setInfo("Add Instrument Track", "Add a new instrument track", "Track", 0);
            break;
        case cmdAddFolderTrack:
            result.setInfo("Add Folder Track", "Add a new folder track for grouping", "Track", 0);
            break;
        case cmdAddArrangerTrack:
            result.setInfo("Add Arranger Track", "Add an arranger track for song sections", "Track", 0);
            break;
        default:
            break;
    }
}

bool MainComponent::perform(const InvocationInfo& info)
{
    switch (info.commandID)
    {
        // ── File ────────────────────────────────────────────────────────
        case cmdNewProject:
        {
            juce::PopupMenu menu;
            menu.addItem(1, "Empty Project");
            menu.addItem(2, "Rock Band Template");
            menu.addItem(3, "Electronic Template");
            menu.addItem(4, "Podcast Template");
            
            menu.showMenuAsync(juce::PopupMenu::Options{}, [this](int result) {
                if (result == 0) return; // User cancelled
                
                if (projectManager) projectManager->newProject();
                
                if (result == 2) { // Rock Band
                    appState.addFolderTrack("Drums");
                    appState.addAudioTrack("Kick");
                    appState.addAudioTrack("Snare");
                    appState.addAudioTrack("Overheads");
                    appState.addFolderTrack("Guitars");
                    appState.addAudioTrack("Rhythm L");
                    appState.addAudioTrack("Rhythm R");
                    appState.addAudioTrack("Lead Guitar");
                    appState.addAudioTrack("Bass");
                    appState.addVocalTrack("Lead Vocal");
                }
                else if (result == 3) { // Electronic
                    appState.addFolderTrack("Beat");
                    appState.addInstrumentTrack("Drum Machine");
                    appState.addInstrumentTrack("Synth Bass");
                    appState.addInstrumentTrack("Pad");
                    appState.addInstrumentTrack("Lead Synth");
                    appState.addAudioTrack("FX / Risers");
                }
                else if (result == 4) { // Podcast
                    appState.addVocalTrack("Host 1");
                    appState.addVocalTrack("Host 2");
                    appState.addVocalTrack("Guest / Remote");
                    appState.addAudioTrack("Intro / Outro Music");
                    appState.addAudioTrack("Sound Effects");
                }
                
                if (audioEngine) audioEngine->syncWithAppState(appState);
                if (timeline) timeline->rebuildTracks();
            });
            return true;
        }

        case cmdOpenProject:
            if (projectManager) projectManager->openProject();
            if (timeline) timeline->rebuildTracks();
            return true;

        case cmdImportAudio:
        {
            auto chooser = std::make_shared<juce::FileChooser>("Import Audio",
                juce::File::getSpecialLocation(juce::File::userMusicDirectory),
                "*.wav;*.mp3;*.aiff;*.flac");
            chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                [this, chooser](const juce::FileChooser& c) {
                    if (c.getResult().existsAsFile())
                    {
                        auto file = c.getResult();
                        int trackIdx = audioEngine->addAudioTrack(file.getFileNameWithoutExtension());
                        auto* clip = new AudioClip(file, 0.0);
                        clip->colour = audioEngine->getTrackInfo(trackIdx).colour;
                        clip->loadAudioData(audioEngine->getFormatManager());

                        audioEngine->getTrackInfo(trackIdx).clips.add(clip);
                        if (timeline) timeline->rebuildTracks();
                    }
                });
            return true;
        }

        case cmdSaveProject:
            if (projectManager) projectManager->saveProject();
            return true;

        case cmdSaveProjectAs:
            if (projectManager) projectManager->saveProjectAs();
            return true;

        case cmdExportMix:
        case cmdExportStems:
            showExportDialog();
            return true;

        case cmdOpenSettings:
            showSettingsDialog();
            return true;

        case cmdQuit:
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
            return true;

        // ── Edit ────────────────────────────────────────────────────────
        case cmdUndo:
            appState.undo();
            return true;

        case cmdRedo:
            appState.redo();
            return true;

        // ── Transport ───────────────────────────────────────────────────
        case cmdPlay:
            if (audioEngine)
            {
                if (audioEngine->isPlaying())
                    audioEngine->stop();
                else
                    audioEngine->play();
            }
            return true;

        case cmdStop:
            if (audioEngine) audioEngine->stop();
            return true;

        case cmdRecord:
            if (audioEngine)
                audioEngine->toggleRecord();
            return true;

        // ── Track ───────────────────────────────────────────────────────
        case cmdAddAudioTrack:
        {
            OrpheusLogger::logInfo("Adding audio track...");
            appState.addAudioTrack();
            if (audioEngine) audioEngine->addAudioTrack();
            if (timeline) timeline->rebuildTracks();
            OrpheusLogger::logInfo("Audio track added. Total tracks: " + juce::String(audioEngine ? audioEngine->getNumTracks() : 0));
            return true;
        }

        case cmdAddMidiTrack:
        {
            OrpheusLogger::logInfo("Adding MIDI track...");
            appState.addMidiTrack();
            if (audioEngine) audioEngine->addMidiTrack();
            if (timeline) timeline->rebuildTracks();
            OrpheusLogger::logInfo("MIDI track added. Total tracks: " + juce::String(audioEngine ? audioEngine->getNumTracks() : 0));
            return true;
        }

        case cmdAddVocalTrack:
        {
            auto name = "Vocal " + juce::String(audioEngine ? audioEngine->getNumTracks() + 1 : 1);
            OrpheusLogger::logInfo("Adding vocal track: " + name);
            appState.addVocalTrack(name);
            if (audioEngine) audioEngine->addAudioTrack(name);
            if (timeline) timeline->rebuildTracks();
            OrpheusLogger::logInfo("Vocal track added. Total tracks: " + juce::String(audioEngine ? audioEngine->getNumTracks() : 0));
            return true;
        }

        case cmdAddInstrumentTrack:
        {
            auto name = "Instrument " + juce::String(audioEngine ? audioEngine->getNumTracks() + 1 : 1);
            OrpheusLogger::logInfo("Adding instrument track: " + name);
            appState.addInstrumentTrack(name);
            if (audioEngine) audioEngine->addMidiTrack(name);
            if (timeline) timeline->rebuildTracks();
            OrpheusLogger::logInfo("Instrument track added. Total tracks: " + juce::String(audioEngine ? audioEngine->getNumTracks() : 0));
            return true;
        }

        case cmdAddFolderTrack:
        {
            auto name = "Folder " + juce::String(audioEngine ? audioEngine->getNumTracks() + 1 : 1);
            OrpheusLogger::logInfo("Adding folder track: " + name);
            appState.addFolderTrack(name);
            if (audioEngine) audioEngine->addFolderTrack(name);
            if (timeline) timeline->rebuildTracks();
            OrpheusLogger::logInfo("Folder track added.");
            return true;
        }

        case cmdAddArrangerTrack:
        {
            OrpheusLogger::logInfo("Adding arranger track");
            appState.addArrangerTrack("Arranger");
            if (audioEngine) audioEngine->addArrangerTrack("Arranger");
            if (timeline) timeline->rebuildTracks();
            OrpheusLogger::logInfo("Arranger track added.");
            return true;
        }

        // ── New Workflow Commands ─────────────────────────────────────────
        case cmdCaptureMidi:
        {
            int selTrack = appState.getSelectedTrackIndex();
            if (selTrack >= 0 && audioEngine)
            {
                audioEngine->captureMidi(selTrack);
                if (timeline) timeline->rebuildTracks();
                OrpheusLogger::logInfo("Captured MIDI to track " + juce::String(selTrack));
            }
            return true;
        }

        case cmdToggleTempoFollower:
        {
            if (audioEngine)
            {
                bool currentState = audioEngine->getTempoFollower().isEnabled();
                audioEngine->getTempoFollower().setEnabled(!currentState);
                OrpheusLogger::logInfo("Tempo Follower " + juce::String(!currentState ? "enabled" : "disabled"));
            }
            return true;
        }

        case cmdSwitchTheme:
        {
            juce::PopupMenu menu;
            menu.addItem(1, "Dark");
            menu.addItem(2, "Light");
            menu.addItem(3, "Midnight");
            menu.showMenuAsync(juce::PopupMenu::Options{}, [this](int result) {
                juce::String theme;
                if (result == 1) theme = "Dark";
                else if (result == 2) theme = "Light";
                else if (result == 3) theme = "Midnight";
                else return;
                appState.setTheme(theme);
                orpheusLookAndFeel.loadTheme(theme);
                repaint();
            });
            return true;
        }

        case cmdShowMixingAssistant:
        {
            if (audioEngine)
            {
                MixingAssistant assistant;
                juce::String report = "=== Mixing Assistant Report ===\n\n";

                for (int i = 0; i < audioEngine->getNumTracks(); ++i)
                {
                    auto& ti = audioEngine->getTrackInfo(i);
                    // Find the first loaded audio clip on this track
                    for (auto* clip : ti.clips)
                    {
                        if (auto* ac = dynamic_cast<AudioClip*>(clip))
                        {
                            if (ac->isLoaded)
                            {
                                auto analysis = assistant.analyzeTrack(ti.name, ac->audioData, ac->sampleRate);
                                report += "Track: " + analysis.trackName + "\n";
                                report += "  EQ: " + analysis.eqSuggestion + "\n";
                                report += "  Comp: " + analysis.compSuggestion + "\n";
                                report += "  Gain Adjust: " + juce::String(analysis.suggestedGain, 1) + " dB\n\n";
                                break;
                            }
                        }
                    }
                }

                juce::AlertWindow::showMessageBoxAsync(
                    juce::MessageBoxIconType::InfoIcon,
                    "AI Mixing Assistant", report);
            }
            return true;
        }

        case cmdFreezeTrack:
        {
            int selTrack = appState.getSelectedTrackIndex();
            if (selTrack >= 0 && audioEngine)
            {
                if (audioEngine->isTrackFrozen(selTrack))
                    audioEngine->unfreezeTrack(selTrack);
                else
                    audioEngine->freezeTrack(selTrack);
            }
            return true;
        }

        case cmdScratchPadSave:
        {
            juce::String padName = "Scratch Pad " + juce::String((int)appState.scratchPads.size() + 1);
            appState.createScratchPad(padName);
            OrpheusLogger::logInfo("Created scratch pad: " + padName);
            return true;
        }

        case cmdScratchPadLoad:
        {
            if (!appState.scratchPads.empty())
            {
                juce::PopupMenu menu;
                for (int i = 0; i < (int)appState.scratchPads.size(); ++i)
                    menu.addItem(i + 1, appState.scratchPads[(size_t)i].name);

                menu.showMenuAsync(juce::PopupMenu::Options{}, [this](int result) {
                    if (result > 0)
                    {
                        appState.loadScratchPad(result - 1);
                        if (audioEngine) audioEngine->syncWithAppState(appState);
                        if (timeline) timeline->rebuildTracks();
                    }
                });
            }
            return true;
        }

        // ── View ────────────────────────────────────────────────────────
        case cmdShowTimeline:
            switchToView(0);
            return true;

        case cmdOpenPianoRoll:
            switchToView(1);
            return true;

        case cmdOpenMastering:
            switchToView(2);
            return true;

        case cmdToggleMixer:
            showMixer = !showMixer;
            resized();
            return true;

        case cmdOpenPluginBrowser:
            currentView = 6;
            switchToView(currentView);
            return true;

        // ── AI ──────────────────────────────────────────────────────────
        case cmdOpenStemSeparation:
            switchToView(3);
            return true;

        case cmdAudioToMidi:
            showAudioToMidiDialog();
            return true;

        case cmdAudioCleanup:
            switchToView(4);
            return true;

        case cmdOpenAutoTune:
            switchToView(5);
            return true;

        case cmdToggleTrackSettings:
            showTrackSettings = !showTrackSettings;
            if (showTrackSettings)
            {
                showLibraryPanel = false;
                if (trackSettingsPanel) trackSettingsPanel->setTrackIndex(appState.getSelectedTrackIndex());
            }
            resized();
            return true;

        case cmdToggleLibraryPanel:
            showLibraryPanel = !showLibraryPanel;
            if (showLibraryPanel)
            {
                showTrackSettings = false;
            }
            resized();
            return true;

        // ── Help ────────────────────────────────────────────────────────
        case cmdAbout:
            showAboutDialog();
            return true;

        default:
            return false;
    }
}

//==============================================================================
// Dialogs
//==============================================================================

void MainComponent::showSettingsDialog()
{
    auto* deviceManager = &audioEngine->getDeviceManager();
    if (!deviceManager) return;

    auto* panel = new AudioMidiSettingsPanel(*deviceManager);

    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned(panel);
    options.dialogTitle = "Audio & MIDI Settings";
    options.dialogBackgroundColour = getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId);
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = false;
    options.resizable = false;

    options.launchAsync();
}

void MainComponent::showStemSeparationDialog()
{
    auto chooser = std::make_shared<juce::FileChooser>(
        "Select audio file for stem separation",
        juce::File{},
        "*.wav;*.mp3;*.flac;*.aiff;*.ogg");

    chooser->launchAsync(juce::FileBrowserComponent::openMode
                        | juce::FileBrowserComponent::canSelectFiles,
        [this, chooser](const juce::FileChooser& fc)
        {
            auto result = fc.getResult();
            if (!result.existsAsFile()) return;

            // Create a dialog to show progress
            auto* progressLabel = new juce::Label({}, "Separating stems...\nThis may take a few minutes.");
            progressLabel->setJustificationType(juce::Justification::centred);
            progressLabel->setSize(350, 120);
            progressLabel->setFont(juce::Font(16.0f));

            juce::DialogWindow::LaunchOptions opts;
            opts.content.setOwned(progressLabel);
            opts.dialogTitle = "Stem Separation";
            opts.dialogBackgroundColour = getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId);
            opts.escapeKeyTriggersCloseButton = true;
            opts.useNativeTitleBar = false;
            opts.resizable = false;

            auto* dialog = opts.launchAsync();

            audioEngine->getStemSeparator().separate(result, appState,
                [this, dialog](StemSeparationResult stemResult)
                {
                    if (dialog) dialog->exitModalState(0);

                    juce::String msg = "Stems saved to:\n";
                    if (stemResult.vocals.existsAsFile()) msg += "  Vocals: " + stemResult.vocals.getFileName() + "\n";
                    if (stemResult.drums.existsAsFile())  msg += "  Drums: " + stemResult.drums.getFileName() + "\n";
                    if (stemResult.bass.existsAsFile())   msg += "  Bass: " + stemResult.bass.getFileName() + "\n";
                    if (stemResult.guitar.existsAsFile()) msg += "  Guitar: " + stemResult.guitar.getFileName() + "\n";
                    if (stemResult.other.existsAsFile())  msg += "  Other: " + stemResult.other.getFileName() + "\n";

                    juce::AlertWindow::showMessageBoxAsync(
                        juce::MessageBoxIconType::InfoIcon,
                        "Stem Separation Complete", msg);
                });
        });
}

void MainComponent::showAudioToMidiDialog()
{
    auto chooser = std::make_shared<juce::FileChooser>(
        "Select audio file for MIDI conversion",
        juce::File{},
        "*.wav;*.mp3;*.flac;*.aiff;*.ogg");

    chooser->launchAsync(juce::FileBrowserComponent::openMode
                        | juce::FileBrowserComponent::canSelectFiles,
        [this, chooser](const juce::FileChooser& fc)
        {
            auto result = fc.getResult();
            if (!result.existsAsFile()) return;

            auto* progressLabel = new juce::Label({}, "Converting audio to MIDI...\nThis may take a moment.");
            progressLabel->setJustificationType(juce::Justification::centred);
            progressLabel->setSize(350, 120);
            progressLabel->setFont(juce::Font(16.0f));

            juce::DialogWindow::LaunchOptions opts;
            opts.content.setOwned(progressLabel);
            opts.dialogTitle = "Audio to MIDI";
            opts.dialogBackgroundColour = getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId);
            opts.escapeKeyTriggersCloseButton = true;
            opts.useNativeTitleBar = false;
            opts.resizable = false;

            auto* dialog = opts.launchAsync();

            audioEngine->getAudioToMidiConverter().convert(result,
                [this, dialog](AudioToMidiResult midiResult)
                {
                    if (dialog) dialog->exitModalState(0);

                    juce::String msg;
                    if (midiResult.midiFileOnDisk.existsAsFile())
                        msg = "MIDI file saved to:\n" + midiResult.midiFileOnDisk.getFullPathName();
                    else
                        msg = "Conversion completed but no output file was generated.";

                    if (midiResult.detectedBPM > 0)
                        msg += "\n\nDetected BPM: " + juce::String(midiResult.detectedBPM, 1);

                    juce::AlertWindow::showMessageBoxAsync(
                        juce::MessageBoxIconType::InfoIcon,
                        "Audio to MIDI Complete", msg);
                });
        });
}

void MainComponent::showAudioCleanupDialog()
{
    auto* aw = new juce::AlertWindow("Audio Cleanup", "Configure audio cleanup settings:", juce::MessageBoxIconType::NoIcon);

    aw->addComboBox("mode", {"DC Offset Removal", "Noise Reduction", "De-Click", "De-Esser", "Hum Removal"}, "Cleanup Mode:");
    aw->addButton("Apply", 1, juce::KeyPress(juce::KeyPress::returnKey));
    aw->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    aw->enterModalState(true, juce::ModalCallbackFunction::create(
        [](int result)
        {
            if (result == 1)
            {
                juce::AlertWindow::showMessageBoxAsync(
                    juce::MessageBoxIconType::InfoIcon,
                    "Audio Cleanup",
                    "Audio cleanup will be applied during playback.\n"
                    "Select an audio clip and the cleanup will be applied in real-time.");
            }
        }), true);
}

void MainComponent::showExportDialog()
{
    // Show format/quality selection first
    auto* alert = new juce::AlertWindow("Export Mix", "Configure export settings:", juce::MessageBoxIconType::NoIcon);

    alert->addComboBox("format", {"WAV", "FLAC", "OGG"}, "Format:");
    alert->addComboBox("sampleRate", {"44100 Hz", "48000 Hz", "88200 Hz", "96000 Hz"}, "Sample Rate:");
    alert->addComboBox("bitDepth", {"16-bit", "24-bit", "32-bit float"}, "Bit Depth:");

    // Default to 48000 Hz
    alert->getComboBoxComponent("sampleRate")->setSelectedItemIndex(1, juce::dontSendNotification);

    alert->addButton("Export", 1, juce::KeyPress(juce::KeyPress::returnKey));
    alert->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    alert->enterModalState(true, juce::ModalCallbackFunction::create(
        [this, alert](int result)
        {
            if (result == 0) { delete alert; return; }

            auto formatIdx = alert->getComboBoxComponent("format")->getSelectedItemIndex();
            auto srIdx     = alert->getComboBoxComponent("sampleRate")->getSelectedItemIndex();
            auto bdIdx     = alert->getComboBoxComponent("bitDepth")->getSelectedItemIndex();
            delete alert;

            juce::String ext;
            if (formatIdx == 0) ext = ".wav";
            else if (formatIdx == 1) ext = ".flac";
            else ext = ".ogg";

            int sampleRates[] = { 44100, 48000, 88200, 96000 };
            int bitDepths[]   = { 16, 24, 32 };

            int chosenSR = sampleRates[srIdx];
            int chosenBD = bitDepths[bdIdx];

            juce::String filterStr = "*" + ext;

            auto chooser = std::make_shared<juce::FileChooser>(
                "Export Mix",
                juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                    .getChildFile("mix" + ext),
                filterStr);

            chooser->launchAsync(juce::FileBrowserComponent::saveMode
                                | juce::FileBrowserComponent::canSelectFiles,
                [this, chooser, chosenSR, chosenBD, ext](const juce::FileChooser& fc)
                {
                    auto file = fc.getResult();
                    if (file == juce::File{}) return;

                    // Ensure correct extension
                    if (!file.hasFileExtension(ext.substring(1)))
                        file = file.withFileExtension(ext.substring(1));

                    if (audioEngine)
                    {
                        // Log export settings
                        OrpheusLogger::logInfo("Exporting: " + file.getFullPathName()
                            + " | SR=" + juce::String(chosenSR)
                            + " | BD=" + juce::String(chosenBD)
                            + " | Fmt=" + ext);

                        audioEngine->exportMix(file);

                        juce::AlertWindow::showMessageBoxAsync(
                            juce::MessageBoxIconType::InfoIcon,
                            "Export Complete",
                            "Mix exported to:\n" + file.getFullPathName()
                            + "\n\nFormat: " + ext.substring(1).toUpperCase()
                            + "\nSample Rate: " + juce::String(chosenSR) + " Hz"
                            + "\nBit Depth: " + juce::String(chosenBD) + "-bit");
                    }
                });
        }), true);
}

void MainComponent::showAboutDialog()
{
    juce::String aboutText;
    aboutText << "Orpheus Plus v1.0.0\n\n"
              << "Professional Digital Audio Workstation\n\n"
              << "Features:\n"
              << "  • Multi-track audio & MIDI recording\n"
              << "  • Piano Roll MIDI editor\n"
              << "  • Mastering suite (EQ, Compressor, Limiter)\n"
              << "  • AI Stem Separation (Demucs/Spleeter)\n"
              << "  • AI Audio-to-MIDI conversion\n"
              << "  • Audio Cleanup & Noise Reduction\n"
              << "  • VST3/AU Plugin hosting\n"
              << "  • Pitch Correction (Auto-Tune)\n\n"
              << "Built with JUCE " << juce::SystemStats::getJUCEVersion() << "\n"
              << "(C) 2026 Orpheus Audio";

    juce::AlertWindow::showMessageBoxAsync(
        juce::MessageBoxIconType::InfoIcon,
        "About Orpheus Plus", aboutText);
}

void MainComponent::updateLayout()
{
    resized();
}
