#include <JuceHeader.h>
#include "MainComponent.h"
#include "StemSeparation/StemSeparator.h"
#include "AudioToMidi/AudioToMidiConverter.h"
#include "AudioCleanup/AudioCleanupProcessor.h"

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

    // View Tab Buttons
    addAndMakeVisible(tabTimeline);
    addAndMakeVisible(tabPianoRoll);
    addAndMakeVisible(tabMastering);
    addAndMakeVisible(tabStemSep);
    addAndMakeVisible(tabCleanup);
    addAndMakeVisible(tabAutoTune);

    auto tabStyle = [](juce::TextButton& btn, bool active) {
        btn.setColour(juce::TextButton::buttonColourId,
                      active ? OrpheusLookAndFeel::bgActive() : OrpheusLookAndFeel::bgDark());
        btn.setColour(juce::TextButton::textColourOnId,   OrpheusLookAndFeel::textPrimary());
        btn.setColour(juce::TextButton::textColourOffId,  OrpheusLookAndFeel::textSecondary());
    };

    tabTimeline.onClick  = [this] { switchToView(0); };
    tabPianoRoll.onClick = [this] { switchToView(1); };
    tabMastering.onClick = [this] { switchToView(2); };
    tabStemSep.onClick   = [this] { switchToView(3); };
    tabCleanup.onClick   = [this] { switchToView(4); };
    tabAutoTune.onClick  = [this] { switchToView(5); };

    tabStyle(tabTimeline, true);
    tabStyle(tabPianoRoll, false);
    tabStyle(tabMastering, false);
    tabStyle(tabStemSep, false);
    tabStyle(tabCleanup, false);
    tabStyle(tabAutoTune, false);

    // Initialize Timeline
    timeline = std::make_unique<TimelineComponent>(*audioEngine, appState, commandManager);
    addAndMakeVisible(timeline.get());

    // Initialize MixerPanel
    mixerPanel = std::make_unique<MixerPanel>(*audioEngine, appState);
    addAndMakeVisible(mixerPanel.get());

    // Initialize SpectrumAnalyzer
    spectrumAnalyzer = std::make_unique<SpectrumAnalyzer>(*audioEngine);
    addChildComponent(spectrumAnalyzer.get());

    // Initialize PluginBrowser
    pluginBrowser = std::make_unique<PluginBrowser>(*audioEngine, appState);
    addChildComponent(pluginBrowser.get());
    
    // Initialize PianoRoll
    pianoRoll = std::make_unique<PianoRollComponent>(appState, *audioEngine);
    addChildComponent(pianoRoll.get());

    // Initialize MasteringModule
    masteringModule = std::make_unique<MasteringModule>(*audioEngine);
    addChildComponent(masteringModule.get());

    // Initialize new panels
    trackSettingsPanel = std::make_unique<TrackSettingsPanel>(*audioEngine, appState);
    addChildComponent(trackSettingsPanel.get());

    stemSeparatorPanel = std::make_unique<StemSeparatorPanel>(*audioEngine, appState);
    addChildComponent(stemSeparatorPanel.get());

    audioCleanupPanel = std::make_unique<AudioCleanupPanel>(*audioEngine);
    addChildComponent(audioCleanupPanel.get());

    autoTunePanel = std::make_unique<AutoTunePanel>(*audioEngine);
    addChildComponent(autoTunePanel.get());

    appState.addChangeListener(this);

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

    // ── Top: Menu Bar (32px, matches --menu-bar-h) ─────────────────────
    if (menuBar)
        menuBar->setBounds(area.removeFromTop(32));

    // ── View Tab Bar (36px, matches --toolbar-h) ───────────────────────
    {
        auto tabArea = area.removeFromTop(36);
        int tabW = 90;
        tabTimeline.setBounds(tabArea.removeFromLeft(tabW));
        tabPianoRoll.setBounds(tabArea.removeFromLeft(tabW));
        tabMastering.setBounds(tabArea.removeFromLeft(tabW));
        tabStemSep.setBounds(tabArea.removeFromLeft(tabW));
        tabCleanup.setBounds(tabArea.removeFromLeft(tabW));
        tabAutoTune.setBounds(tabArea.removeFromLeft(tabW));
    }

    // ── Bottom: Transport Bar (56px, matches --transport-h) ────────────
    if (transportBar)
        transportBar->setBounds(area.removeFromBottom(56));

    // ── Bottom Panel (Mixer & Spectrum, above transport) ───────────────
    if (showMixer)
    {
        auto bottomArea = area.removeFromBottom(200);
        
        if (spectrumAnalyzer)
        {
            spectrumAnalyzer->setVisible(true);
            spectrumAnalyzer->setBounds(bottomArea.removeFromRight(300));
        }

        if (mixerPanel)
        {
            mixerPanel->setVisible(true);
            mixerPanel->setBounds(bottomArea);
        }
    }
    else
    {
        if (spectrumAnalyzer) spectrumAnalyzer->setVisible(false);
        if (mixerPanel) mixerPanel->setVisible(false);
    }

    // ── Right Sidebar ──────────────────────────────────────────────────
    if (showPluginBrowser && pluginBrowser)
    {
        pluginBrowser->setVisible(true);
        pluginBrowser->setBounds(area.removeFromRight(250));
    }
    else if (pluginBrowser)
    {
        pluginBrowser->setVisible(false);
    }

    if (showTrackSettings && trackSettingsPanel)
    {
        trackSettingsPanel->setVisible(true);
        trackSettingsPanel->setBounds(area.removeFromRight(280));
    }
    else if (trackSettingsPanel)
    {
        trackSettingsPanel->setVisible(false);
    }

    // ── Center Area — switch based on currentView ──────────────────────
    auto centerArea = area;

    // Hide all center panels first
    if (timeline) timeline->setVisible(false);
    if (pianoRoll) pianoRoll->setVisible(false);
    if (masteringModule) masteringModule->setVisible(false);
    if (stemSeparatorPanel) stemSeparatorPanel->setVisible(false);
    if (audioCleanupPanel) audioCleanupPanel->setVisible(false);
    if (autoTunePanel) autoTunePanel->setVisible(false);

    switch (currentView) {
        case 0: // Timeline
            if (timeline) { timeline->setVisible(true); timeline->setBounds(centerArea); }
            break;
        case 1: // Piano Roll
            if (pianoRoll) { pianoRoll->setVisible(true); pianoRoll->setBounds(centerArea); }
            break;
        case 2: // Mastering
            if (masteringModule) { masteringModule->setVisible(true); masteringModule->setBounds(centerArea); }
            break;
        case 3: // Stem Separator
            if (stemSeparatorPanel) { stemSeparatorPanel->setVisible(true); stemSeparatorPanel->setBounds(centerArea); }
            break;
        case 4: // Audio Cleanup
            if (audioCleanupPanel) { audioCleanupPanel->setVisible(true); audioCleanupPanel->setBounds(centerArea); }
            break;
        case 5: // AutoTune
            if (autoTunePanel) { autoTunePanel->setVisible(true); autoTunePanel->setBounds(centerArea); }
            break;
    }
}

void MainComponent::switchToView(int viewIndex)
{
    currentView = viewIndex;

    auto updateTab = [](juce::TextButton& btn, bool active) {
        btn.setColour(juce::TextButton::buttonColourId,
                      active ? OrpheusLookAndFeel::bgActive() : OrpheusLookAndFeel::bgDark());
    };

    updateTab(tabTimeline,  viewIndex == 0);
    updateTab(tabPianoRoll, viewIndex == 1);
    updateTab(tabMastering, viewIndex == 2);
    updateTab(tabStemSep,   viewIndex == 3);
    updateTab(tabCleanup,   viewIndex == 4);
    updateTab(tabAutoTune,  viewIndex == 5);

    resized();
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
            break;

        case 3: // Track
            menu.addCommandItem(&commandManager, cmdAddAudioTrack);
            menu.addCommandItem(&commandManager, cmdAddMidiTrack);
            menu.addCommandItem(&commandManager, cmdAddVocalTrack);
            menu.addCommandItem(&commandManager, cmdAddInstrumentTrack);
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
        cmdOpenAutoTune, cmdToggleTrackSettings,
        cmdAddVocalTrack, cmdAddInstrumentTrack
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
            result.setInfo("Plugin Browser", "Show or hide the plugin browser", "View", 0);
            result.addDefaultKeypress('b', juce::ModifierKeys::commandModifier);
            result.setTicked(showPluginBrowser);
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
        case cmdAddVocalTrack:
            result.setInfo("Add Vocal Track", "Add a new vocal track", "Track", 0);
            break;
        case cmdAddInstrumentTrack:
            result.setInfo("Add Instrument Track", "Add a new instrument track", "Track", 0);
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
            if (projectManager) projectManager->newProject();
            if (timeline) timeline->rebuildTracks();
            return true;

        case cmdOpenProject:
            if (projectManager) projectManager->openProject();
            if (timeline) timeline->rebuildTracks();
            return true;

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
            appState.addAudioTrack();
            if (audioEngine) audioEngine->addAudioTrack();
            if (timeline) timeline->rebuildTracks();
            return true;
        }

        case cmdAddMidiTrack:
        {
            appState.addMidiTrack();
            if (audioEngine) audioEngine->addMidiTrack();
            if (timeline) timeline->rebuildTracks();
            return true;
        }

        case cmdAddVocalTrack:
        {
            appState.addVocalTrack();
            if (audioEngine) audioEngine->addAudioTrack(); // audio engine treats it as audio
            if (timeline) timeline->rebuildTracks();
            return true;
        }

        case cmdAddInstrumentTrack:
        {
            appState.addInstrumentTrack();
            if (audioEngine) audioEngine->addMidiTrack(); // instrument tracks use MIDI
            if (timeline) timeline->rebuildTracks();
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
            showPluginBrowser = !showPluginBrowser;
            resized();
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
            if (showTrackSettings && trackSettingsPanel)
                trackSettingsPanel->setTrackIndex(appState.getSelectedTrackIndex());
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

    auto* selector = new juce::AudioDeviceSelectorComponent(
        *deviceManager,
        0, 256,
        0, 256,
        true, true, true, false);

    selector->setSize(500, 450);

    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned(selector);
    options.dialogTitle = "Audio/MIDI Settings";
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

            audioEngine->getAudioToMidiConverter().convert(result, appState,
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
    auto chooser = std::make_shared<juce::FileChooser>(
        "Export Mix",
        juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
            .getChildFile("mix.wav"),
        "*.wav;*.flac;*.ogg");

    chooser->launchAsync(juce::FileBrowserComponent::saveMode
                        | juce::FileBrowserComponent::canSelectFiles,
        [this, chooser](const juce::FileChooser& fc)
        {
            auto result = fc.getResult();
            if (result == juce::File{}) return;

            // Start offline export
            if (audioEngine)
            {
                audioEngine->exportMix(result);
                juce::AlertWindow::showMessageBoxAsync(
                    juce::MessageBoxIconType::InfoIcon,
                    "Export Complete",
                    "Mix exported to:\n" + result.getFullPathName());
            }
        });
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
