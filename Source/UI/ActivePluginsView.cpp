#include "ActivePluginsView.h"

ActivePluginsView::ActivePluginsView(AudioEngine& e)
    : audioEngine(e)
{
    pluginList.setModel(this);
    pluginList.setRowHeight(40);
    pluginList.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff0d0d1a));
    addAndMakeVisible(pluginList);

    audioEngine.getPluginManager().addListener(this);
    refreshList();
}

ActivePluginsView::~ActivePluginsView()
{
    audioEngine.getPluginManager().removeListener(this);
}

void ActivePluginsView::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff12121e));
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.setFont(juce::Font(14.0f, juce::Font::bold));
    g.drawText("ACTIVE VSTS", getLocalBounds().removeFromTop(30).reduced(10, 0),
               juce::Justification::centredLeft);
}

void ActivePluginsView::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(30); // Header
    pluginList.setBounds(bounds);
}

int ActivePluginsView::getNumRows()
{
    return (int)activePlugins.size();
}

void ActivePluginsView::paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool selected)
{
    if (row < 0 || row >= activePlugins.size()) return;
    const auto& info = activePlugins[row];

    if (selected)
    {
        g.setColour(juce::Colour(0xff533483).withAlpha(0.6f));
        g.fillRect(0, 0, w, h);
    }

    g.setColour(selected ? juce::Colours::white : juce::Colour(0xffccccee));
    g.setFont(juce::Font(14.0f, juce::Font::bold));
    g.drawText(info.pluginName, 10, 4, w - 160, h/2, juce::Justification::bottomLeft);

    g.setColour(juce::Colours::grey);
    g.setFont(juce::Font(11.0f));
    g.drawText(info.trackName + " (Slot " + juce::String(info.slotIndex + 1) + ")", 
               10, h/2, w - 160, h/2, juce::Justification::topLeft);

    // Buttons
    juce::Rectangle<int> openBtn(w - 150, 6, 64, h - 12);
    juce::Rectangle<int> removeBtn(w - 80, 6, 64, h - 12);

    g.setColour(juce::Colour(0xff2a2a3a));
    g.fillRoundedRectangle(openBtn.toFloat(), 4.0f);
    g.setColour(juce::Colour(0xff3a1a1a));
    g.fillRoundedRectangle(removeBtn.toFloat(), 4.0f);

    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(11.0f));
    g.drawText("Open UI", openBtn, juce::Justification::centred);
    g.setColour(juce::Colour(0xffff5555));
    g.drawText("Remove", removeBtn, juce::Justification::centred);
}

void ActivePluginsView::listBoxItemClicked(int row, const juce::MouseEvent& e)
{
    if (row < 0 || row >= activePlugins.size()) return;
    const auto& info = activePlugins[row];

    int w = pluginList.getWidth();
    juce::Rectangle<int> openBtn(w - 150, 6, 64, pluginList.getRowHeight() - 12);
    juce::Rectangle<int> removeBtn(w - 80, 6, 64, pluginList.getRowHeight() - 12);

    auto clickPos = e.getEventRelativeTo(&pluginList).getMouseDownPosition();
    // we need to offset Y by the row's Y position
    int rowY = pluginList.getRowPosition(row, true).getY();
    int localY = clickPos.y - rowY;

    if (openBtn.contains(clickPos.x, localY))
    {
        audioEngine.getPluginManager().openPluginEditor(info.trackIndex, info.slotIndex);
    }
    else if (removeBtn.contains(clickPos.x, localY))
    {
        audioEngine.getPluginManager().removePluginFromTrack(info.trackIndex, info.slotIndex);
    }
}

void ActivePluginsView::pluginListChanged()
{
    juce::MessageManager::callAsync([this] { refreshList(); });
}

void ActivePluginsView::refreshList()
{
    activePlugins.clear();
    for (int t = 0; t < audioEngine.getNumTracks(); ++t)
    {
        auto& trackInfo = audioEngine.getTrackInfo(t);
        for (int s = 0; s < trackInfo.MAX_PLUGINS; ++s)
        {
            int nodeID = trackInfo.pluginSlots[s];
            if (nodeID != -1)
            {
                juce::String name = audioEngine.getPluginManager().getPluginName(nodeID);
                if (name.isNotEmpty())
                {
                    activePlugins.push_back({ t, s, nodeID, name, trackInfo.name });
                }
            }
        }
    }
    pluginList.updateContent();
}
