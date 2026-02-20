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
    };
    addAndMakeVisible(scanButton);

    searchBox.setTextToShowWhenEmpty("Search plugins...", juce::Colours::grey);
    searchBox.onTextChange = [this] { rebuildFilteredList(); };
    addAndMakeVisible(searchBox);

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

    rebuildFilteredList();
}

PluginBrowser::~PluginBrowser()
{
    audioEngine.getPluginManager().removeListener(this);
}

void PluginBrowser::resized()
{
    auto bounds = getLocalBounds().reduced(4);
    scanButton.setBounds(bounds.removeFromTop(28).reduced(0, 2));
    progressBar.setBounds(bounds.removeFromTop(16));
    statusLabel.setBounds(bounds.removeFromTop(16));
    searchBox.setBounds(bounds.removeFromTop(24).reduced(0, 2));
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

    // Load plugin onto currently selected track
    // TODO: determine selected track from AppState
    audioEngine.getPluginManager().addPluginToTrack(0, *filteredPlugins[row]);
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

    const juce::KnownPluginList& list = audioEngine.getPluginManager().getKnownPluginList();
    const juce::Array<juce::PluginDescription>& types = list.getTypes();

    for (const auto& desc : types)
    {
        if (query.isEmpty() || desc.name.toLowerCase().contains(query) ||
            desc.manufacturerName.toLowerCase().contains(query))
        {
            filteredPlugins.add(&desc);
        }
    }

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
    rebuildFilteredList();
    statusLabel.setText(juce::String(filteredPlugins.size()) + " plugins found.",
                        juce::dontSendNotification);
}

void PluginBrowser::pluginListChanged()
{
    rebuildFilteredList();
}
