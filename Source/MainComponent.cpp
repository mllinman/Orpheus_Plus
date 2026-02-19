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

    // Layout Timeline
    if (timeline)
        timeline->setBounds(area);
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
