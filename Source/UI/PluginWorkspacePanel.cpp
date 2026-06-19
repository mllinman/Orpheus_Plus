#include "PluginWorkspacePanel.h"

PluginWorkspacePanel::PluginWorkspacePanel(AudioEngine& e, AppState& s)
{
    pluginBrowser = std::make_unique<PluginBrowser>(e, s);
    activePluginsView = std::make_unique<ActivePluginsView>(e);

    tabs.setTabBarDepth(36);
    tabs.addTab("Installed VSTs", juce::Colour(0xff12121e), pluginBrowser.get(), false);
    tabs.addTab("Active VSTs", juce::Colour(0xff12121e), activePluginsView.get(), false);

    addAndMakeVisible(tabs);
}

PluginWorkspacePanel::~PluginWorkspacePanel()
{
}

void PluginWorkspacePanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0d0d1a));
}

void PluginWorkspacePanel::resized()
{
    tabs.setBounds(getLocalBounds());
}
