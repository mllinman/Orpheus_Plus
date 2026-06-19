#pragma once
#include <JuceHeader.h>
#include "PluginBrowser.h"
#include "ActivePluginsView.h"
#include "../Audio/AudioEngine.h"
#include "../Project/AppState.h"

class PluginWorkspacePanel : public juce::Component
{
public:
    PluginWorkspacePanel(AudioEngine& e, AppState& s);
    ~PluginWorkspacePanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::TabbedComponent tabs { juce::TabbedButtonBar::TabsAtTop };
    std::unique_ptr<PluginBrowser> pluginBrowser;
    std::unique_ptr<ActivePluginsView> activePluginsView;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginWorkspacePanel)
};
