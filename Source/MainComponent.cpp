#include "MainComponent.h"

// #if 0
// MainComponent::MainComponent(OrpheusPlusApplication& app) ...
// #endif

// Stub implementation to satisfy linker (if needed, but usually not for build test)
MainComponent::MainComponent()
{
    // Minimum valid constructor to satisfy member initialization
    setSize(800, 600);
}

MainComponent::~MainComponent() {}
void MainComponent::paint(juce::Graphics& g) { g.fillAll(juce::Colours::black); }
void MainComponent::resized() {}
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
