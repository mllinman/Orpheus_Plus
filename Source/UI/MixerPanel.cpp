#include "MixerPanel.h"

MixerPanel::MixerPanel(AudioEngine& e, AppState& s)
    : audioEngine(e), appState(s)
{
    audioEngine.addListener(this);
    channelViewport.setViewedComponent(&channelContainer, false);
    channelViewport.setScrollBarsShown(false, true);
    addAndMakeVisible(channelViewport);
    startTimerHz(30);
    rebuildStrips();
}

MixerPanel::~MixerPanel()
{
    audioEngine.removeListener(this);
    stopTimer();
}

void MixerPanel::resized()
{
    auto bounds = getLocalBounds();
    channelViewport.setBounds(bounds);

    const int stripW = 72;
    channelContainer.setSize(juce::jmax(bounds.getWidth(),
        strips.size() * stripW), bounds.getHeight());

    for (int i = 0; i < strips.size(); ++i)
        strips[i]->setBounds(i * stripW, 0, stripW, bounds.getHeight());
}

void MixerPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff12121e));
    g.setColour(juce::Colour(0xff533483).withAlpha(0.5f));
    g.drawHorizontalLine(0, 0, (float)getWidth());
}

void MixerPanel::trackListChanged()
{
    rebuildStrips();
}

void MixerPanel::timerCallback()
{
    repaint();
}

void MixerPanel::rebuildStrips()
{
    strips.clear();
    channelContainer.removeAllChildren();

    for (int i = 0; i < audioEngine.getNumTracks(); ++i)
    {
        auto* strip = strips.add(new ChannelStrip(i, audioEngine));
        channelContainer.addAndMakeVisible(strip);
    }

    resized();
}
