#include <JuceHeader.h>
#include "PluginBrowser.h"
#include "../Audio/AudioEngine.h"

PluginBrowser::PluginBrowser(AudioEngine& e, AppState& s)
    : audioEngine(e), appState(s), progressBar(scanProgress_)
{
    audioEngine.getPluginManager().addListener(this);

    scanButton.onClick = [this] {
        audioEngine.getPluginManager().scanForPlugins();
        statusLabel.setText("Scanning...", juce::dontSendNotification);
        progressBar.setVisible(true);
        cancelScanButton.setVisible(true);
        scanButton.setEnabled(false);
    };
    addAndMakeVisible(scanButton);

    cancelScanButton.onClick = [this] {
        audioEngine.getPluginManager().cancelScan();
        cancelScanButton.setVisible(false);
        scanButton.setEnabled(true);
        statusLabel.setText("Scan cancelled.", juce::dontSendNotification);
    };
    cancelScanButton.setVisible(false);
    addAndMakeVisible(cancelScanButton);

    searchBox.setTextToShowWhenEmpty("Search plugins...", juce::Colours::grey);
    searchBox.onTextChange = [this] { rebuildFilteredList(); };
    addAndMakeVisible(searchBox);

    // Category filter
    categoryCombo.addItem("All", 1);
    categoryCombo.addItem("Instruments", 2);
    categoryCombo.addItem("Effects", 3);
    categoryCombo.setSelectedId(1, juce::dontSendNotification);
    categoryCombo.onChange = [this] { rebuildFilteredList(); };
    addAndMakeVisible(categoryCombo);

    // Sort order
    sortCombo.addItem("Name", 1);
    sortCombo.addItem("Manufacturer", 2);
    sortCombo.addItem("Format", 3);
    sortCombo.setSelectedId(1, juce::dontSendNotification);
    sortCombo.onChange = [this] { rebuildFilteredList(); };
    addAndMakeVisible(sortCombo);

    pluginList.setModel(this);
    pluginList.setRowHeight(22);
    pluginList.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff0d0d1a));
    addAndMakeVisible(pluginList);

    statusLabel.setText("No plugins scanned.", juce::dontSendNotification);
    statusLabel.setFont(juce::Font(10.0f));
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible(statusLabel);

    progressBar.setVisible(false);
    addChildComponent(progressBar);

    //── Custom path management ───────────────────────────────────────────────
    showPathsToggle.onClick = [this] {
        showingPaths = showPathsToggle.getToggleState();
        pathListBox.setVisible(showingPaths);
        addPathButton.setVisible(showingPaths);
        removePathButton.setVisible(showingPaths);
        resized();
    };
    addAndMakeVisible(showPathsToggle);

    addPathButton.onClick = [this] {
        if (fileChooser != nullptr) return; // Prevent double-open
        fileChooser = std::make_unique<juce::FileChooser>(
            "Select VST Plugin Directory",
            juce::File::getSpecialLocation(juce::File::userHomeDirectory));

        fileChooser->launchAsync(
            juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
            [this](const juce::FileChooser& fc)
            {
                auto result = fc.getResult();
                if (result.isDirectory())
                {
                    audioEngine.getPluginManager().addSearchPath(result);
                    refreshPathList();
                }
                // Release the dialog so it doesn't persist
                juce::MessageManager::callAsync([this]() { fileChooser.reset(); });
            });
    };
    addPathButton.setVisible(false);
    addAndMakeVisible(addPathButton);

    removePathButton.onClick = [this] {
        int selected = pathListBox.getSelectedRow();
        if (selected >= 0 && selected < pathModel.paths.size())
        {
            juce::File dir(pathModel.paths[selected]);
            audioEngine.getPluginManager().removeSearchPath(dir);
            refreshPathList();
        }
    };
    removePathButton.setVisible(false);
    addAndMakeVisible(removePathButton);

    pathListBox.setModel(&pathModel);
    pathListBox.setRowHeight(18);
    pathListBox.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff0a0a16));
    pathListBox.setVisible(false);
    addAndMakeVisible(pathListBox);

    refreshPathList();
    rebuildFilteredList();
}

PluginBrowser::~PluginBrowser()
{
    audioEngine.getPluginManager().removeListener(this);
}

void PluginBrowser::resized()
{
    auto bounds = getLocalBounds().reduced(4);
    
    // Scan row
    auto scanRow = bounds.removeFromTop(28);
    cancelScanButton.setBounds(scanRow.removeFromRight(60).reduced(0, 2));
    scanButton.setBounds(scanRow.reduced(0, 2));
    
    progressBar.setBounds(bounds.removeFromTop(16));
    statusLabel.setBounds(bounds.removeFromTop(16));
    searchBox.setBounds(bounds.removeFromTop(24).reduced(0, 2));
    
    // Filter row
    auto filterRow = bounds.removeFromTop(24);
    categoryCombo.setBounds(filterRow.removeFromLeft(filterRow.getWidth() / 2).reduced(0, 2));
    sortCombo.setBounds(filterRow.reduced(0, 2));

    // Custom paths section (collapsible, at bottom)
    showPathsToggle.setBounds(bounds.removeFromBottom(22));

    if (showingPaths)
    {
        auto pathBtnRow = bounds.removeFromBottom(26);
        addPathButton.setBounds(pathBtnRow.removeFromLeft(pathBtnRow.getWidth() / 2).reduced(1, 2));
        removePathButton.setBounds(pathBtnRow.reduced(1, 2));

        int pathListHeight = juce::jmin(90, bounds.getHeight() / 3);
        pathListBox.setBounds(bounds.removeFromBottom(pathListHeight));
    }

    pluginList.setBounds(bounds);
}

void PluginBrowser::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff12121e));
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText("PLUGINS", getLocalBounds().removeFromTop(16),
               juce::Justification::centred);
}

int PluginBrowser::getNumRows() { return filteredPlugins.size(); }

void PluginBrowser::paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool selected)
{
    if (!juce::isPositiveAndBelow(row, filteredPlugins.size())) return;

    auto* desc = filteredPlugins[row];

    if (selected)
    {
        g.setColour(juce::Colour(0xff533483).withAlpha(0.6f));
        g.fillRect(0, 0, w, h);
    }

    g.setColour(selected ? juce::Colours::white : juce::Colour(0xffccccee));
    g.setFont(juce::Font(11.0f));
    g.drawText(desc->name, 6, 0, w - 60, h, juce::Justification::centredLeft);

    g.setColour(juce::Colour(0xff7b8bb0));
    g.setFont(juce::Font(9.0f));
    g.drawText(desc->pluginFormatName, w - 54, 0, 50, h, juce::Justification::centredRight);
}

void PluginBrowser::listBoxItemDoubleClicked(int row, const juce::MouseEvent&)
{
    if (!juce::isPositiveAndBelow(row, filteredPlugins.size())) return;

    // Load plugin onto selected track (or track 0 if none selected)
    int trackIdx = appState.getSelectedTrackIndex();
    if (trackIdx < 0 || trackIdx >= audioEngine.getNumTracks())
        trackIdx = audioEngine.getNumTracks() > 0 ? 0 : -1;
    
    if (trackIdx >= 0)
        audioEngine.getPluginManager().addPluginToTrack(trackIdx, *filteredPlugins[row]);
}

juce::var PluginBrowser::getDragSourceDescription(const juce::SparseSet<int>& rows)
{
    if (rows.isEmpty()) return {};
    int row = rows[0];
    if (!juce::isPositiveAndBelow(row, filteredPlugins.size())) return {};

    return "PluginDesc:" + filteredPlugins[row]->createXml()->toString();
}

void PluginBrowser::rebuildFilteredList()
{
    filteredPlugins.clear();
    juce::String query = searchBox.getText().toLowerCase();
    int category = categoryCombo.getSelectedId();
    int sortMode = sortCombo.getSelectedId();

    const juce::KnownPluginList& list = audioEngine.getPluginManager().getKnownPluginList();
    allPlugins = list.getTypes();

    for (const auto& desc : allPlugins)
    {
        // Category filter
        if (category == 2 && !desc.isInstrument) continue;     // Instruments only
        if (category == 3 && desc.isInstrument) continue;      // Effects only

        // Text search
        if (query.isEmpty() || desc.name.toLowerCase().contains(query) ||
            desc.manufacturerName.toLowerCase().contains(query))
        {
            filteredPlugins.add(&desc);
        }
    }

    // Sort
    struct NameSorter {
        static int compareElements(const juce::PluginDescription* a, const juce::PluginDescription* b)
        { return a->name.compareIgnoreCase(b->name); }
    };
    struct MfgSorter {
        static int compareElements(const juce::PluginDescription* a, const juce::PluginDescription* b)
        { int r = a->manufacturerName.compareIgnoreCase(b->manufacturerName); return r != 0 ? r : a->name.compareIgnoreCase(b->name); }
    };
    struct FmtSorter {
        static int compareElements(const juce::PluginDescription* a, const juce::PluginDescription* b)
        { int r = a->pluginFormatName.compareIgnoreCase(b->pluginFormatName); return r != 0 ? r : a->name.compareIgnoreCase(b->name); }
    };

    if (sortMode == 1) { NameSorter s; filteredPlugins.sort(s); }
    else if (sortMode == 2) { MfgSorter s; filteredPlugins.sort(s); }
    else if (sortMode == 3) { FmtSorter s; filteredPlugins.sort(s); }

    pluginList.updateContent();
}

void PluginBrowser::scanProgress(float progress, const juce::String& name)
{
    scanProgress_ = progress;
    statusLabel.setText("Scanning: " + name, juce::dontSendNotification);
    progressBar.repaint();
}

void PluginBrowser::scanComplete()
{
    progressBar.setVisible(false);
    cancelScanButton.setVisible(false);
    scanButton.setEnabled(true);
    rebuildFilteredList();
    statusLabel.setText(juce::String(filteredPlugins.size()) + " plugins found.",
                        juce::dontSendNotification);
}

void PluginBrowser::pluginListChanged()
{
    // Need to do this on message thread
    juce::MessageManager::callAsync([this]() {
        rebuildFilteredList();
    });
}

void PluginBrowser::changeListenerCallback(juce::ChangeBroadcaster*)
{
    // Reserved for future use (e.g. file watcher notifications)
}

void PluginBrowser::refreshPathList()
{
    pathModel.paths.clear();
    auto& customPaths = audioEngine.getPluginManager().getCustomSearchPaths();
    for (int i = 0; i < customPaths.getNumPaths(); ++i)
        pathModel.paths.add(customPaths[i].getFullPathName());
    pathListBox.updateContent();
    pathListBox.repaint();
}
