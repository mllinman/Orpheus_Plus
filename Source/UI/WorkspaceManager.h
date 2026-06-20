#pragma once
#include <JuceHeader.h>
#include "DockablePanel.h"

class WorkspaceManager : public juce::Component
{
public:
    WorkspaceManager();
    ~WorkspaceManager() override;

    void resized() override;
    void paint(juce::Graphics& g) override;

    // Registers a panel in the workspace (top, bottom, left, right, or center)
    enum class Zone { Left, Right, Top, Bottom, Center };
    void addPanel(DockablePanel* panel, Zone zone);
    void removePanel(DockablePanel* panel);

    // Save and load workspace layouts
    juce::ValueTree saveLayout();
    void loadLayout(const juce::ValueTree& state);

    // Expose tab switching for center panels
    void showCenterPanel(const juce::String& panelName);

private:
    struct PanelInfo {
        DockablePanel* panel;
        Zone zone;
    };
    std::vector<PanelInfo> panels;

    // Tabbed container for center zone panels
    std::unique_ptr<juce::TabbedComponent> centerTabs;

    juce::StretchableLayoutManager horizontalLayout;
    juce::StretchableLayoutManager verticalLayout;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WorkspaceManager)
};
