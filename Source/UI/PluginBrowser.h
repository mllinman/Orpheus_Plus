#pragma once
#include <JuceHeader.h>
#include "../Audio/PluginManager.h"
#include "../Project/AppState.h"

class AudioEngine;

//==============================================================================
class PluginBrowser : public juce::Component,
                      private juce::ListBoxModel,
                      private PluginManager::Listener,
                      private juce::ChangeListener
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

    // ChangeListener (for folder chooser)
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

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

    //── Custom VST path management ───────────────────────────────────────────
    juce::TextButton addPathButton  { "Add VST Folder..." };
    juce::TextButton removePathButton { "Remove" };
    juce::ListBox    pathListBox;
    juce::ToggleButton showPathsToggle { "Show Custom Paths" };
    bool showingPaths { false };

    // Path list model (inline)
    struct PathListModel : public juce::ListBoxModel
    {
        juce::StringArray paths;
        int getNumRows() override { return paths.size(); }
        void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool selected) override
        {
            if (!juce::isPositiveAndBelow(row, paths.size())) return;
            if (selected)
            {
                g.setColour(juce::Colour(0xff533483).withAlpha(0.5f));
                g.fillRect(0, 0, w, h);
            }
            g.setColour(juce::Colours::lightgrey);
            g.setFont(juce::Font(10.0f));
            g.drawText(paths[row], 6, 0, w - 12, h, juce::Justification::centredLeft);
        }
    };
    PathListModel pathModel;

    void refreshPathList();

    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginBrowser)
};
