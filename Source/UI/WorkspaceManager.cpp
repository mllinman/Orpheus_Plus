#include "WorkspaceManager.h"
#include "OrpheusLookAndFeel.h"

//==============================================================================
// SidebarContainer — lays out CollapsiblePanels vertically
//==============================================================================
void WorkspaceManager::SidebarContainer::resized()
{
    int y = 0;
    for (auto* panel : panels)
    {
        int h = panel->getDesiredHeight();
        panel->setBounds(0, y, getWidth(), h);
        y += h;
    }
    setSize(getWidth(), juce::jmax(y, getParentHeight()));
}

void WorkspaceManager::SidebarContainer::addPanel(const juce::String& name,
                                                   std::unique_ptr<juce::Component> content)
{
    auto* cp = panels.add(new CollapsiblePanel(name, std::move(content)));
    addAndMakeVisible(cp);
    resized();
}

//==============================================================================
// WorkspaceManager
//==============================================================================
WorkspaceManager::WorkspaceManager()
{
    // Bottom tabs
    bottomTabs = std::make_unique<juce::TabbedComponent>(juce::TabbedButtonBar::TabsAtTop);
    bottomTabs->setTabBarDepth(26);
    bottomTabs->setOutline(0);
    bottomTabs->setColour(juce::TabbedComponent::backgroundColourId, OrpheusLookAndFeel::bgDarkest());
    addAndMakeVisible(bottomTabs.get());

    // Sidebars
    leftViewport.setViewedComponent(&leftContainer, false);
    leftViewport.setScrollBarsShown(true, false);
    addAndMakeVisible(leftViewport);

    rightViewport.setViewedComponent(&rightContainer, false);
    rightViewport.setScrollBarsShown(true, false);
    addAndMakeVisible(rightViewport);

    // Vertical resizer between timeline and bottom tabs
    // Layout: [0]=timeline, [1]=resizer, [2]=bottomTabs
    verticalLayout.setItemLayout(0, 100, -1.0, -0.55);  // Timeline: 55% default
    verticalLayout.setItemLayout(1, resizerBarSize, resizerBarSize, resizerBarSize);
    verticalLayout.setItemLayout(2, 80, -1.0, -0.45);   // BottomTabs: 45% default

    timelineBottomResizer = std::make_unique<juce::StretchableLayoutResizerBar>(
        &verticalLayout, 1, false);
    addAndMakeVisible(timelineBottomResizer.get());
}

WorkspaceManager::~WorkspaceManager()
{
    if (bottomTabs)
        bottomTabs->clearTabs();
}

void WorkspaceManager::paint(juce::Graphics& g)
{
    g.fillAll(OrpheusLookAndFeel::bgDarkest());

    // Draw sidebar edges
    if (leftSidebarVisible)
    {
        g.setColour(OrpheusLookAndFeel::borderSubtle());
        g.drawVerticalLine(leftSidebarWidth, 0, (float)getHeight());
    }
    if (rightSidebarVisible)
    {
        g.setColour(OrpheusLookAndFeel::borderSubtle());
        g.drawVerticalLine(getWidth() - rightSidebarWidth, 0, (float)getHeight());
    }
}

void WorkspaceManager::resized()
{
    auto bounds = getLocalBounds();

    // Left sidebar
    if (leftSidebarVisible)
    {
        auto leftArea = bounds.removeFromLeft(leftSidebarWidth);
        leftViewport.setBounds(leftArea);
        leftViewport.setVisible(true);
        leftContainer.setSize(leftArea.getWidth() - leftViewport.getScrollBarThickness(), 
                              leftContainer.getHeight());
        leftContainer.resized();
    }
    else
    {
        leftViewport.setVisible(false);
    }

    // Right sidebar
    if (rightSidebarVisible)
    {
        auto rightArea = bounds.removeFromRight(rightSidebarWidth);
        rightViewport.setBounds(rightArea);
        rightViewport.setVisible(true);
        rightContainer.setSize(rightArea.getWidth() - rightViewport.getScrollBarThickness(),
                               rightContainer.getHeight());
        rightContainer.resized();
    }
    else
    {
        rightViewport.setVisible(false);
    }

    // Center area: timeline (top) + resizer + bottom tabs
    // Use StretchableLayoutManager for the vertical split
    juce::Component* comps[] = { timelineComponent ? (juce::Component*)timelineComponent : (juce::Component*)bottomTabs.get(),
                                 timelineBottomResizer.get(),
                                 bottomTabs.get() };

    if (timelineComponent)
    {
        verticalLayout.layOutComponents(comps, 3,
                                        bounds.getX(), bounds.getY(),
                                        bounds.getWidth(), bounds.getHeight(),
                                        true, true);
    }
    else
    {
        // No timeline — give all space to bottom tabs
        bottomTabs->setBounds(bounds);
        timelineBottomResizer->setVisible(false);
    }
}

void WorkspaceManager::addToTimeline(juce::Component* timeline)
{
    timelineComponent = timeline;
    addAndMakeVisible(timeline);
    resized();
}

void WorkspaceManager::addToBottomTab(const juce::String& name, juce::Component* content)
{
    if (bottomTabs)
    {
        bottomTabs->addTab(name, OrpheusLookAndFeel::bgDarker(), content, false);
    }
}

void WorkspaceManager::addToLeftSidebar(const juce::String& name, std::unique_ptr<juce::Component> content)
{
    leftContainer.addPanel(name, std::move(content));
}

void WorkspaceManager::addToRightSidebar(const juce::String& name, std::unique_ptr<juce::Component> content)
{
    rightContainer.addPanel(name, std::move(content));
}

void WorkspaceManager::showBottomTab(const juce::String& name)
{
    if (bottomTabs)
    {
        for (int i = 0; i < bottomTabs->getNumTabs(); ++i)
        {
            if (bottomTabs->getTabNames()[i] == name)
            {
                bottomTabs->setCurrentTabIndex(i);
                return;
            }
        }
    }
}

void WorkspaceManager::setLeftSidebarVisible(bool visible)
{
    leftSidebarVisible = visible;
    resized();
}

void WorkspaceManager::setRightSidebarVisible(bool visible)
{
    rightSidebarVisible = visible;
    resized();
}

juce::ValueTree WorkspaceManager::saveLayout()
{
    juce::ValueTree vt("WorkspaceLayout");
    vt.setProperty("leftVisible", leftSidebarVisible, nullptr);
    vt.setProperty("rightVisible", rightSidebarVisible, nullptr);
    vt.setProperty("leftWidth", leftSidebarWidth, nullptr);
    vt.setProperty("rightWidth", rightSidebarWidth, nullptr);
    if (bottomTabs)
        vt.setProperty("activeTab", bottomTabs->getCurrentTabIndex(), nullptr);
    return vt;
}

void WorkspaceManager::loadLayout(const juce::ValueTree& state)
{
    if (state.hasType("WorkspaceLayout"))
    {
        leftSidebarVisible = state.getProperty("leftVisible", true);
        rightSidebarVisible = state.getProperty("rightVisible", true);
        leftSidebarWidth = state.getProperty("leftWidth", 240);
        rightSidebarWidth = state.getProperty("rightWidth", 280);
        if (bottomTabs && state.hasProperty("activeTab"))
            bottomTabs->setCurrentTabIndex(state.getProperty("activeTab"));
        resized();
    }
}
