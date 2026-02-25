#pragma once
#include <JuceHeader.h>
#include "../Audio/PluginManager.h"
#include "../Project/AppState.h"

class AudioEngine;

//==============================================================================
class PluginBrowser : public juce::Component,
                      private juce::ListBoxModel,
                      private PluginManager::Listener
{
public:
    PluginBrowser(AudioEngine& engine, AppState& state);
    ~PluginBrowser() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    // ListBoxModel
    int  getNumRows() override;
    void paintListBoxItem(int row, juce::Graphics&, int w, int h, bool selected) override;
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override;
    juce::var getDragSourceDescription(const juce::SparseSet<int>& rows) override;

    // PluginManager::Listener
    void scanProgress(float progress, const juce::String& name) override;
    void scanComplete() override;
    void pluginListChanged() override;

    void rebuildFilteredList();

    AudioEngine& audioEngine;
    AppState&    appState;

    juce::TextButton scanButton  { "Scan Plugins" };
    juce::TextButton cancelScanButton { "Cancel" };
    juce::TextEditor searchBox;
    juce::ComboBox   categoryCombo;
    juce::ComboBox   sortCombo;
    juce::ListBox    pluginList;
    juce::Label      statusLabel;

    juce::ProgressBar progressBar;
    double scanProgress_ = 0.0;

    juce::Array<juce::PluginDescription> allPlugins;
    juce::Array<const juce::PluginDescription*> filteredPlugins;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginBrowser)
};
