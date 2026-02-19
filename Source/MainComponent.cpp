#include "MainComponent.h"

MainComponent::MainComponent()
{
    setLookAndFeel(&lookAndFeel);

    audioEngine   = std::make_unique<AudioEngine>();
    projectManager = std::make_unique<ProjectManager>(appState, *audioEngine);

    commandManager.registerAllCommandsForTarget(this);
    addKeyListener(commandManager.getKeyMappings());

    menuBar = std::make_unique<juce::MenuBarComponent>(this);
    addAndMakeVisible(*menuBar);

    transportBar = std::make_unique<TransportBar>(*audioEngine, commandManager);
    addAndMakeVisible(*transportBar);

    timeline = std::make_unique<TimelineComponent>(*audioEngine, appState, commandManager);
    addAndMakeVisible(*timeline);

    mixerPanel = std::make_unique<MixerPanel>(*audioEngine, appState);
    addAndMakeVisible(*mixerPanel);

    spectrumAnalyzer = std::make_unique<SpectrumAnalyzer>(*audioEngine);
    addAndMakeVisible(*spectrumAnalyzer);

    pianoRoll = std::make_unique<PianoRollComponent>(appState, *audioEngine);
    addChildComponent(*pianoRoll); // hidden until needed

    // masteringModule = std::make_unique<MasteringModule>(*audioEngine);
    // addChildComponent(*masteringModule);

    pluginBrowser = std::make_unique<PluginBrowser>(*audioEngine, appState);
    addChildComponent(*pluginBrowser);

    setSize(1600, 960);
    startTimerHz(30);
}

MainComponent::~MainComponent()
{
    setLookAndFeel(nullptr);
    stopTimer();
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(lookAndFeel.findColour(juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds();

    menuBar->setBounds(bounds.removeFromTop(24));
    transportBar->setBounds(bounds.removeFromTop(56));
    spectrumAnalyzer->setBounds(bounds.removeFromBottom(80));

    if (showMixer)
    {
        mixerPanel->setBounds(bounds.removeFromBottom(220));
        mixerPanel->setVisible(true);
    }
    else
    {
        mixerPanel->setVisible(false);
    }

    /*
    if (showMastering)
    {
        masteringModule->setBounds(bounds.removeFromRight(360));
        masteringModule->setVisible(true);
    }
    else
    {
        masteringModule->setVisible(false);
    }
    */

    if (showPluginBrowser)
    {
        pluginBrowser->setBounds(bounds.removeFromLeft(260));
        pluginBrowser->setVisible(true);
    }
    else
    {
        pluginBrowser->setVisible(false);
    }

    if (showPianoRoll)
    {
        auto pianoHeight = juce::roundToInt(bounds.getHeight() * 0.45f);
        pianoRoll->setBounds(bounds.removeFromBottom(pianoHeight));
        pianoRoll->setVisible(true);
    }
    else
    {
        pianoRoll->setVisible(false);
    }

    timeline->setBounds(bounds);
}

bool MainComponent::hasUnsavedChanges() const
{
    return appState.isDirty();
}

void MainComponent::timerCallback()
{
    spectrumAnalyzer->repaint();
    transportBar->updatePositionDisplay();
}

//==============================================================================
juce::StringArray MainComponent::getMenuBarNames()
{
    return { "File", "Edit", "View", "Track", "Tools", "Help" };
}

juce::PopupMenu MainComponent::getMenuForIndex(int menuIndex, const juce::String&)
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
            break;
        case 1: // Edit
            menu.addCommandItem(&commandManager, cmdUndo);
            menu.addCommandItem(&commandManager, cmdRedo);
            break;
        case 2: // View
            menu.addItem(100, "Show Mixer",    true, showMixer);
            menu.addItem(101, "Show Mastering",true, showMastering);
            menu.addItem(102, "Show Plugin Browser", true, showPluginBrowser);
            break;
        case 3: // Track
            menu.addCommandItem(&commandManager, cmdAddAudioTrack);
            menu.addCommandItem(&commandManager, cmdAddMidiTrack);
            break;
        case 4: // Tools
            menu.addCommandItem(&commandManager, cmdOpenStemSeparation);
            menu.addCommandItem(&commandManager, cmdAudioToMidi);
            menu.addCommandItem(&commandManager, cmdOpenPluginBrowser);
            break;
        case 5: // Help
            menu.addItem(200, "About Orpheus Plus");
            break;
    }
    return menu;
}

void MainComponent::menuItemSelected(int menuItemID, int)
{
    switch (menuItemID)
    {
        case 100: showMixer         = !showMixer;         resized(); break;
        case 101: showMastering     = !showMastering;     resized(); break;
        case 102: showPluginBrowser = !showPluginBrowser; resized(); break;
        case 200:
            juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,
                "Orpheus Plus", "Version 1.0.0\nAdvanced JUCE DAW");
            break;
    }
}

//==============================================================================
void MainComponent::getAllCommands(juce::Array<juce::CommandID>& commands)
{
    commands.addArray({
        cmdNewProject, cmdOpenProject, cmdSaveProject, cmdSaveProjectAs,
        cmdUndo, cmdRedo, cmdPlay, cmdStop, cmdRecord,
        cmdAddAudioTrack, cmdAddMidiTrack,
        cmdOpenMastering, cmdOpenStemSeparation, cmdAudioToMidi,
        cmdOpenPianoRoll, cmdExportMix, cmdExportStems, cmdOpenPluginBrowser
    });
}

void MainComponent::getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result)
{
    using juce::ModifierKeys;
    switch (commandID)
    {
        case cmdNewProject:
            result.setInfo("New Project", "Create a new project", "File", 0);
            result.addDefaultKeypress('n', ModifierKeys::commandModifier);
            break;
        case cmdOpenProject:
            result.setInfo("Open Project...", "Open an existing project", "File", 0);
            result.addDefaultKeypress('o', ModifierKeys::commandModifier);
            break;
        case cmdSaveProject:
            result.setInfo("Save Project", "Save current project", "File", 0);
            result.addDefaultKeypress('s', ModifierKeys::commandModifier);
            break;
        case cmdSaveProjectAs:
            result.setInfo("Save Project As...", "Save project with new name", "File", 0);
            result.addDefaultKeypress('s', ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
            break;
        case cmdUndo:
            result.setInfo("Undo", "Undo last action", "Edit", 0);
            result.addDefaultKeypress('z', ModifierKeys::commandModifier);
            break;
        case cmdRedo:
            result.setInfo("Redo", "Redo last undone action", "Edit", 0);
            result.addDefaultKeypress('z', ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
            break;
        case cmdPlay:
            result.setInfo("Play / Pause", "Toggle playback", "Transport", 0);
            result.addDefaultKeypress(juce::KeyPress::spaceKey, 0);
            break;
        case cmdStop:
            result.setInfo("Stop", "Stop playback and return to start", "Transport", 0);
            result.addDefaultKeypress(juce::KeyPress::returnKey, 0);
            break;
        case cmdRecord:
            result.setInfo("Record", "Begin recording", "Transport", 0);
            result.addDefaultKeypress('r', ModifierKeys::commandModifier);
            break;
        case cmdAddAudioTrack:
            result.setInfo("Add Audio Track", "Insert a new audio track", "Track", 0);
            result.addDefaultKeypress('t', ModifierKeys::commandModifier);
            break;
        case cmdAddMidiTrack:
            result.setInfo("Add MIDI Track", "Insert a new MIDI track", "Track", 0);
            result.addDefaultKeypress('t', ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
            break;
        case cmdOpenStemSeparation:
            result.setInfo("Stem Separation...", "AI-powered stem separation", "Tools", 0);
            break;
        case cmdAudioToMidi:
            result.setInfo("Audio to MIDI...", "Convert audio to MIDI notes", "Tools", 0);
            break;
        case cmdExportMix:
            result.setInfo("Export Mix...", "Export final mix to audio file", "File", 0);
            result.addDefaultKeypress('e', ModifierKeys::commandModifier);
            break;
        case cmdExportStems:
            result.setInfo("Export Stems...", "Export individual track stems", "File", 0);
            result.addDefaultKeypress('e', ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
            break;
        case cmdOpenPluginBrowser:
            result.setInfo("Plugin Browser", "Browse and load VST3/AU plugins", "Tools", 0);
            result.addDefaultKeypress('p', ModifierKeys::commandModifier);
            break;
        default:
            break;
    }
}

bool MainComponent::perform(const juce::InvocationInfo& info)
{
    switch (info.commandID)
    {
        case cmdNewProject:      projectManager->newProject(); return true;
        case cmdOpenProject:     projectManager->openProject(); return true;
        case cmdSaveProject:     projectManager->saveProject(); return true;
        case cmdSaveProjectAs:   projectManager->saveProjectAs(); return true;
        case cmdUndo:            appState.undo(); return true;
        case cmdRedo:            appState.redo(); return true;
        case cmdPlay:            audioEngine->togglePlayback(); return true;
        case cmdStop:            audioEngine->stop(); return true;
        case cmdRecord:          audioEngine->toggleRecord(); return true;
        case cmdAddAudioTrack:   appState.addAudioTrack(); timeline->rebuildTracks(); return true;
        case cmdAddMidiTrack:    appState.addMidiTrack();  timeline->rebuildTracks(); return true;
        case cmdOpenStemSeparation: showStemSeparationDialog(); return true;
        case cmdAudioToMidi:        showAudioToMidiDialog(); return true;
        case cmdExportMix:          showExportDialog(); return true;
        case cmdOpenPluginBrowser:
            showPluginBrowser = !showPluginBrowser;
            resized();
            return true;
        default:
            return false;
    }
}

void MainComponent::showStemSeparationDialog()
{
    // Opens a file chooser then hands off to StemSeparator
    auto chooser = std::make_shared<juce::FileChooser>("Select audio file for stem separation",
        juce::File::getSpecialLocation(juce::File::userMusicDirectory),
        "*.wav;*.mp3;*.aiff;*.flac");

    chooser->launchAsync(juce::FileBrowserComponent::openMode |
                         juce::FileBrowserComponent::canSelectFiles,
        [this, chooser](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file.existsAsFile())
                audioEngine->getStemSeparator().separate(file, appState);
        });
}

void MainComponent::showAudioToMidiDialog()
{
    auto chooser = std::make_shared<juce::FileChooser>("Select audio file for MIDI conversion",
        juce::File::getSpecialLocation(juce::File::userMusicDirectory),
        "*.wav;*.mp3;*.aiff;*.flac");

    chooser->launchAsync(juce::FileBrowserComponent::openMode |
                         juce::FileBrowserComponent::canSelectFiles,
        [this, chooser](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file.existsAsFile())
                audioEngine->getAudioToMidiConverter().convert(file, appState);
        });
}

void MainComponent::showExportDialog()
{
    auto chooser = std::make_shared<juce::FileChooser>("Export Mix",
        juce::File::getSpecialLocation(juce::File::userMusicDirectory),
        "*.wav;*.aiff;*.flac");

    chooser->launchAsync(juce::FileBrowserComponent::saveMode |
                         juce::FileBrowserComponent::warnAboutOverwriting,
        [this, chooser](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file != juce::File{})
                audioEngine->exportMix(file);
        });
}
