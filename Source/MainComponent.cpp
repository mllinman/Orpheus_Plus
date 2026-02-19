#include <JuceHeader.h>
#include "MainComponent.h"

// #if 0
// MainComponent::MainComponent(OrpheusPlusApplication& app) ...
// #endif

// Stub implementation removed
MainComponent::MainComponent()
{
    // Initialize LookAndFeel
    juce::LookAndFeel::setDefaultLookAndFeel(&orpheusLookAndFeel);

    // Initialize AudioEngine
    audioEngine = std::make_unique<AudioEngine>();

    // Initialize TransportBar
    transportBar = std::make_unique<TransportBar>(*audioEngine, commandManager);
    addAndMakeVisible(transportBar.get());

    // Initialize Timeline
    timeline = std::make_unique<TimelineComponent>(*audioEngine, appState, commandManager);
    addAndMakeVisible(timeline.get());

    // Initialize MixerPanel
    mixerPanel = std::make_unique<MixerPanel>(*audioEngine, appState);
    addAndMakeVisible(mixerPanel.get());

    // Initialize PluginBrowser
    pluginBrowser = std::make_unique<PluginBrowser>(*audioEngine, appState);
    addChildComponent(pluginBrowser.get());
    
    // Initialize PianoRoll
    pianoRoll = std::make_unique<PianoRollComponent>(appState, *audioEngine);
    addChildComponent(pianoRoll.get());

    // Initialize MasteringModule
    masteringModule = std::make_unique<MasteringModule>(*audioEngine);
    addChildComponent(masteringModule.get());

    setSize(1024, 768);
}

MainComponent::~MainComponent()
{
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a2e)); // Background from LookAndFeel
}

void MainComponent::resized()
{
    auto area = getLocalBounds();

    // Layout TransportBar at the top
    if (transportBar)
        transportBar->setBounds(area.removeFromTop(40));

    // Layout Bottom Panel (Mixer)
    if (showMixer && mixerPanel)
    {
        mixerPanel->setBounds(area.removeFromBottom(200));
    }

    // Layout Right Sidebar (Plugin Browser)
    if (showPluginBrowser && pluginBrowser)
    {
        pluginBrowser->setVisible(true);
        pluginBrowser->setBounds(area.removeFromRight(250));
    }
    else if (pluginBrowser)
    {
        pluginBrowser->setVisible(false);
    }

    // Layout Center Area (Timeline vs PianoRoll vs Mastering)
    auto centerArea = area;

    if (showMastering && masteringModule)
    {
        masteringModule->setVisible(true);
        masteringModule->setBounds(centerArea);
        if (timeline) timeline->setVisible(false);
        if (pianoRoll) pianoRoll->setVisible(false);
    }
    else if (showPianoRoll && pianoRoll)
    {
        pianoRoll->setVisible(true);
        pianoRoll->setBounds(centerArea);
        if (timeline) timeline->setVisible(false);
        if (masteringModule) masteringModule->setVisible(false);
    }
    else
    {
        // Default: Timeline
        if (timeline)
        {
            timeline->setVisible(true);
            timeline->setBounds(centerArea);
        }
        if (pianoRoll) pianoRoll->setVisible(false);
        if (masteringModule) masteringModule->setVisible(false);
    }
}
void MainComponent::timerCallback() {}
juce::ApplicationCommandManager& MainComponent::getCommandManager() { return commandManager; }
void MainComponent::getAllCommands(juce::Array<juce::CommandID>&) {}
void MainComponent::getCommandInfo(juce::CommandID, juce::ApplicationCommandInfo&) {}
bool MainComponent::perform(const InvocationInfo&) { return false; }

juce::StringArray MainComponent::getMenuBarNames() { return {}; }
juce::PopupMenu MainComponent::getMenuForIndex(int, const juce::String&) { return {}; }
void MainComponent::menuItemSelected(int, int) {}

// Dummy to avoid empty translation unit warning
void dummy() {}
