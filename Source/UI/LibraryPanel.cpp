#include "LibraryPanel.h"
#include "OrpheusLookAndFeel.h"

LibraryPanel::LibraryPanel(AudioEngine& engine, AppState& state)
    : audioEngine(engine), appState(state)
{
    // Start the directory scanner thread
    thread.startThread();

    addAndMakeVisible(titleLabel);
    titleLabel.setFont(juce::Font(18.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);

    // Set up the file tree component
    addAndMakeVisible(fileTree);
    fileTree.addListener(this);

    // Set the root directory
    juce::File libraryRoot = juce::File::getCurrentWorkingDirectory().getChildFile("Library");
    if (!libraryRoot.exists())
    {
        // Try to find it relative to executable if running from build folder
        libraryRoot = juce::File::getSpecialLocation(juce::File::currentExecutableFile)
                        .getParentDirectory().getParentDirectory().getChildFile("Library");
    }

    if (libraryRoot.exists() && libraryRoot.isDirectory())
    {
        directoryList.setDirectory(libraryRoot, true, true);
    }
}

LibraryPanel::~LibraryPanel()
{
    fileTree.removeListener(this);
    thread.stopThread(2000);
}

void LibraryPanel::paint(juce::Graphics& g)
{
    g.fillAll(OrpheusLookAndFeel::bgDark());
    g.setColour(OrpheusLookAndFeel::bgDarkest());
    g.drawRect(getLocalBounds(), 1);
}

void LibraryPanel::resized()
{
    auto area = getLocalBounds();
    titleLabel.setBounds(area.removeFromTop(36));
    
    // Add a small margin
    area.reduce(4, 4);
    fileTree.setBounds(area);
}

void LibraryPanel::selectionChanged()
{
    // Handle changes if needed
}

void LibraryPanel::fileClicked(const juce::File& file, const juce::MouseEvent& e)
{
    // Handle right click contexts etc.
}

void LibraryPanel::fileDoubleClicked(const juce::File& file)
{
    if (file.existsAsFile())
    {
        juce::String path = file.getFullPathName();
        // Here we could load presets, samples, etc based on extension
        // For now logging it or giving an alert
        juce::Logger::writeToLog("Loaded from library: " + path);
    }
}

void LibraryPanel::browserRootChanged(const juce::File& newRoot)
{
    
}
