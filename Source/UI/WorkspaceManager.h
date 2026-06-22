#pragma once
#include <JuceHeader.h>
#include "CollapsiblePanel.h"

//==============================================================================
// WorkspaceManager — 5-zone DAW layout (Reaper/Cubase-inspired).
//
// Layout:  [LeftSidebar | Timeline + BottomTabs | RightSidebar]
//          All zones separated by draggable resizer bars.
//==============================================================================
class WorkspaceManager : public juce::Component
{
public:
    WorkspaceManager();
    ~WorkspaceManager() override;

    void resized() override;
    void paint(juce::Graphics& g) override;

    // Zone assignment for panels
    enum class Zone { Timeline, BottomTabs, LeftSidebar, RightSidebar };

    // Add a component to a zone
    void addToTimeline(juce::Component* timeline);
    void addToBottomTab(const juce::String& name, juce::Component* content);
    void addToBottomTab(const juce::String& name, juce::Colour tabColour, juce::Component* content);
    void addToLeftSidebar(const juce::String& name, std::unique_ptr<juce::Component> content);
    void addToRightSidebar(const juce::String& name, std::unique_ptr<juce::Component> content);

    // Show a specific bottom tab by name
    void showBottomTab(const juce::String& name);

    // Toggle sidebar visibility
    void setLeftSidebarVisible(bool visible);
    void setRightSidebarVisible(bool visible);
    bool isLeftSidebarVisible() const { return leftSidebarVisible; }
    bool isRightSidebarVisible() const { return rightSidebarVisible; }

    // Layout persistence
    juce::ValueTree saveLayout();
    void loadLayout(const juce::ValueTree& state);

private:
    // Timeline (top, permanent)
    juce::Component* timelineComponent = nullptr;

    // Bottom tabbed zone
    std::unique_ptr<juce::TabbedComponent> bottomTabs;

    // Sidebars — stacked CollapsiblePanels inside a viewport
    struct SidebarContainer : public juce::Component
    {
        juce::OwnedArray<CollapsiblePanel> panels;
        void resized() override;
        void addPanel(const juce::String& name, std::unique_ptr<juce::Component> content);
    };

    juce::Viewport leftViewport, rightViewport;
    SidebarContainer leftContainer, rightContainer;

    bool leftSidebarVisible = true;
    bool rightSidebarVisible = true;

    // Horizontal layout: [leftSidebar | resizer | center | resizer | rightSidebar]
    juce::StretchableLayoutManager horizontalLayout;
    std::unique_ptr<juce::StretchableLayoutResizerBar> leftSidebarResizer;
    std::unique_ptr<juce::StretchableLayoutResizerBar> rightSidebarResizer;

    // Vertical layout for center area: [timeline | resizer | bottomTabs]
    juce::StretchableLayoutManager verticalLayout;
    std::unique_ptr<juce::StretchableLayoutResizerBar> timelineBottomResizer;

    // Static sizes
    static constexpr int resizerBarSize = 5;

    // Internal helper to lay out the center area (timeline + bottom tabs)
    void layoutCenter(juce::Rectangle<int> centerBounds);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WorkspaceManager)
};
