#pragma once
#include <JuceHeader.h>
#include "../Audio/AudioEngine.h"
#include "../Audio/PluginManager.h"

class ActivePluginsView : public juce::Component,
                          public juce::ListBoxModel,
                          public PluginManager::Listener
{
public:
    ActivePluginsView(AudioEngine& e);
    ~ActivePluginsView() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    int getNumRows() override;
    void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool selected) override;
    void listBoxItemClicked(int row, const juce::MouseEvent& e) override;
    
    // PluginManager::Listener
    void pluginListChanged() override;

    void refreshList();

private:
    struct ActivePluginInfo
    {
        int trackIndex;
        int slotIndex;
        int nodeID;
        juce::String pluginName;
        juce::String trackName;
    };

    AudioEngine& audioEngine;
    juce::ListBox pluginList;
    std::vector<ActivePluginInfo> activePlugins;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ActivePluginsView)
};
