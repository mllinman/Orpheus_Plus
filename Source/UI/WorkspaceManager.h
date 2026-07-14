#pragma once
#include <JuceHeader.h>
#include "CollapsiblePanel.h"

//==============================================================================
// WorkspaceManager — 5-zone DAW layout (Reaper/Cubase-inspired).
//
// Layout:  [LeftSidebar | Timeline + BottomTabs | RightSidebar]
//          All zones separated by draggable resizer bars.
//          Tabs are closeable and reorderable via drag-and-drop.
//==============================================================================
class WorkspaceManager : public juce::Component,
                         public juce::DragAndDropContainer
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

    // Show a specific bottom tab by name (reopens if closed)
    void showBottomTab(const juce::String& name);

    // Close/reopen bottom tabs
    void closeBottomTab(int tabIndex);
    void reopenBottomTab(const juce::String& name);
    juce::StringArray getClosedTabNames() const;

    // Toggle sidebar visibility
    void setLeftSidebarVisible(bool visible);
    void setRightSidebarVisible(bool visible);
    bool isLeftSidebarVisible() const { return leftSidebarVisible; }
    bool isRightSidebarVisible() const { return rightSidebarVisible; }

    // Expand and scroll to a specific right sidebar panel by name
    void showRightSidebarPanel(const juce::String& panelName);

    // Callback fired when a tab is closed (so toolbar can update state)
    std::function<void(const juce::String&)> onTabClosed;
    std::function<void(const juce::String&)> onTabReopened;

    // Layout persistence
    juce::ValueTree saveLayout();
    void loadLayout(const juce::ValueTree& state);

private:
    // Timeline (top, permanent)
    juce::Component* timelineComponent = nullptr;

    // Bottom tabbed zone
    std::unique_ptr<juce::TabbedComponent> bottomTabs;

    // Closed tabs storage (name -> { component, colour })
    struct ClosedTabInfo {
        juce::Component* component = nullptr;
        juce::Colour colour;
    };
    std::map<juce::String, ClosedTabInfo> closedTabs;

    juce::ConcertinaPanel leftContainer, rightContainer;

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
    static constexpr int resizerBarSize = 3;

    // Internal helper to lay out the center area (timeline + bottom tabs)
    void layoutCenter(juce::Rectangle<int> centerBounds);

    // Apply tooltip text to each tab button matching its tab name
    void updateTabTooltips();

    // Layout managers for single-sidebar modes
    juce::StretchableLayoutManager leftOnlyLayout;   // [leftSidebar | resizer | center]
    juce::StretchableLayoutManager rightOnlyLayout;  // [center | resizer | rightSidebar]
    std::unique_ptr<juce::StretchableLayoutResizerBar> leftOnlyResizer;
    std::unique_ptr<juce::StretchableLayoutResizerBar> rightOnlyResizer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WorkspaceManager)
};
