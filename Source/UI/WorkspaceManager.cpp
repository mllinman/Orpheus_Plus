#include "WorkspaceManager.h"
#include "OrpheusLookAndFeel.h"



//==============================================================================
// WorkspaceManager
//==============================================================================
WorkspaceManager::WorkspaceManager()
{
    // Bottom tabs — increased depth for readability with many tabs
    bottomTabs = std::make_unique<juce::TabbedComponent>(juce::TabbedButtonBar::TabsAtTop);
    bottomTabs->setTabBarDepth(30);
    bottomTabs->setOutline(0);
    bottomTabs->setColour(juce::TabbedComponent::backgroundColourId, OrpheusLookAndFeel::bgDarkest());
    addAndMakeVisible(bottomTabs.get());

    // Sidebars
    addAndMakeVisible(leftContainer);
    addAndMakeVisible(rightContainer);

    // Vertical resizer between timeline and bottom tabs
    // Layout: [0]=timeline, [1]=resizer, [2]=bottomTabs
    verticalLayout.setItemLayout(0, 100, -1.0, -0.55);  // Timeline: 55% default
    verticalLayout.setItemLayout(1, resizerBarSize, resizerBarSize, resizerBarSize);
    verticalLayout.setItemLayout(2, 80, -1.0, -0.45);   // BottomTabs: 45% default

    timelineBottomResizer = std::make_unique<juce::StretchableLayoutResizerBar>(
        &verticalLayout, 1, false);
    addAndMakeVisible(timelineBottomResizer.get());

    // Horizontal layout (both sidebars): [0]=left, [1]=resizer, [2]=center, [3]=resizer, [4]=right
    horizontalLayout.setItemLayout(0, 120, 500, 240);     // Left sidebar
    horizontalLayout.setItemLayout(1, resizerBarSize, resizerBarSize, resizerBarSize);
    horizontalLayout.setItemLayout(2, 200, -1.0, -1.0);   // Center (flex)
    horizontalLayout.setItemLayout(3, resizerBarSize, resizerBarSize, resizerBarSize);
    horizontalLayout.setItemLayout(4, 250, 500, 350);     // Right sidebar

    leftSidebarResizer = std::make_unique<juce::StretchableLayoutResizerBar>(
        &horizontalLayout, 1, true);
    addAndMakeVisible(leftSidebarResizer.get());

    rightSidebarResizer = std::make_unique<juce::StretchableLayoutResizerBar>(
        &horizontalLayout, 3, true);
    addAndMakeVisible(rightSidebarResizer.get());

    // Single-sidebar layouts with draggable resizer bars
    // Left-only: [0]=leftSidebar, [1]=resizer, [2]=center
    leftOnlyLayout.setItemLayout(0, 120, 500, 240);
    leftOnlyLayout.setItemLayout(1, resizerBarSize, resizerBarSize, resizerBarSize);
    leftOnlyLayout.setItemLayout(2, 200, -1.0, -1.0);

    leftOnlyResizer = std::make_unique<juce::StretchableLayoutResizerBar>(
        &leftOnlyLayout, 1, true);
    addAndMakeVisible(leftOnlyResizer.get());

    // Right-only: [0]=center, [1]=resizer, [2]=rightSidebar
    rightOnlyLayout.setItemLayout(0, 200, -1.0, -1.0);
    rightOnlyLayout.setItemLayout(1, resizerBarSize, resizerBarSize, resizerBarSize);
    rightOnlyLayout.setItemLayout(2, 120, 500, 280);

    rightOnlyResizer = std::make_unique<juce::StretchableLayoutResizerBar>(
        &rightOnlyLayout, 1, true);
    addAndMakeVisible(rightOnlyResizer.get());
}

WorkspaceManager::~WorkspaceManager()
{
    if (bottomTabs)
        bottomTabs->clearTabs();
}

void WorkspaceManager::paint(juce::Graphics& g)
{
    g.fillAll(OrpheusLookAndFeel::bgDarkest());
}

void WorkspaceManager::resized()
{
    auto bounds = getLocalBounds();

    if (leftSidebarVisible && rightSidebarVisible)
    {
        // Full 5-component horizontal layout
        // Hide single-sidebar resizers
        leftOnlyResizer->setVisible(false);
        rightOnlyResizer->setVisible(false);

        juce::Component* hComps[] = {
            &leftContainer, leftSidebarResizer.get(),
            nullptr, // placeholder — we'll set center bounds manually
            rightSidebarResizer.get(), &rightContainer
        };

        // We need a temporary component for the center to get its bounds
        juce::Component centerPlaceholder;
        hComps[2] = &centerPlaceholder;
        addAndMakeVisible(&centerPlaceholder);

        horizontalLayout.layOutComponents(hComps, 5,
            bounds.getX(), bounds.getY(),
            bounds.getWidth(), bounds.getHeight(),
            false, true);

        auto centerBounds = centerPlaceholder.getBounds();
        removeChildComponent(&centerPlaceholder);

        // Vertical layout inside center
        layoutCenter(centerBounds);
    }
    else if (leftSidebarVisible)
    {
        // Left sidebar + center only — use leftOnlyLayout for draggable resizer
        leftSidebarResizer->setVisible(false);
        rightSidebarResizer->setVisible(false);
        rightContainer.setVisible(false);
        rightOnlyResizer->setVisible(false);
        leftOnlyResizer->setVisible(true);
        leftContainer.setVisible(true);

        juce::Component centerPlaceholder;
        juce::Component* comps[] = { &leftContainer, leftOnlyResizer.get(), &centerPlaceholder };
        addAndMakeVisible(&centerPlaceholder);
        leftOnlyLayout.layOutComponents(comps, 3,
            bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(),
            false, true);

        auto centerBounds = centerPlaceholder.getBounds();
        removeChildComponent(&centerPlaceholder);

        layoutCenter(centerBounds);
    }
    else if (rightSidebarVisible)
    {
        // Center + right sidebar only — use rightOnlyLayout for draggable resizer
        leftSidebarResizer->setVisible(false);
        rightSidebarResizer->setVisible(false);
        leftContainer.setVisible(false);
        leftOnlyResizer->setVisible(false);
        rightOnlyResizer->setVisible(true);
        rightContainer.setVisible(true);

        juce::Component centerPlaceholder;
        juce::Component* comps[] = { &centerPlaceholder, rightOnlyResizer.get(), &rightContainer };
        addAndMakeVisible(&centerPlaceholder);
        rightOnlyLayout.layOutComponents(comps, 3,
            bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(),
            false, true);

        auto centerBounds = centerPlaceholder.getBounds();
        removeChildComponent(&centerPlaceholder);

        layoutCenter(centerBounds);
    }
    else
    {
        // No sidebars
        leftContainer.setVisible(false);
        rightContainer.setVisible(false);
        leftSidebarResizer->setVisible(false);
        rightSidebarResizer->setVisible(false);
        leftOnlyResizer->setVisible(false);
        rightOnlyResizer->setVisible(false);
        layoutCenter(bounds);
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
    addToBottomTab(name, OrpheusLookAndFeel::bgDarker(), content);
}

void WorkspaceManager::addToBottomTab(const juce::String& name, juce::Colour tabColour, juce::Component* content)
{
    if (bottomTabs)
    {
        bottomTabs->addTab(name, tabColour, content, false);
        updateTabTooltips();
    }
}

void WorkspaceManager::addToLeftSidebar(const juce::String& name, std::unique_ptr<juce::Component> content)
{
    auto* cp = new CollapsiblePanel(name, std::move(content));
    leftContainer.addPanel(cp, true, true);
    leftContainer.setMaximumPanelSize(cp, 0); // Allow it to expand as much as needed
}

void WorkspaceManager::addToRightSidebar(const juce::String& name, std::unique_ptr<juce::Component> content)
{
    auto* cp = new CollapsiblePanel(name, std::move(content));
    rightContainer.addPanel(cp, true, true);
    rightContainer.setMaximumPanelSize(cp, 0);
}

void WorkspaceManager::showBottomTab(const juce::String& name)
{
    if (!bottomTabs) return;

    // Check if this tab is closed — if so, reopen it first
    auto it = closedTabs.find(name);
    if (it != closedTabs.end())
    {
        reopenBottomTab(name);
    }

    // Now find and switch to it
    for (int i = 0; i < bottomTabs->getNumTabs(); ++i)
    {
        if (bottomTabs->getTabNames()[i] == name)
        {
            bottomTabs->setCurrentTabIndex(i);
            return;
        }
    }
}

void WorkspaceManager::closeBottomTab(int tabIndex)
{
    if (!bottomTabs || tabIndex < 0 || tabIndex >= bottomTabs->getNumTabs()) return;

    auto name = bottomTabs->getTabNames()[tabIndex];
    auto colour = bottomTabs->getTabBackgroundColour(tabIndex);
    auto* comp = bottomTabs->getTabContentComponent(tabIndex);

    // Stash the tab info
    closedTabs[name] = { comp, colour };

    // Remove from tab bar (false = don't delete component)
    bottomTabs->removeTab(tabIndex);

    if (onTabClosed)
        onTabClosed(name);
}

void WorkspaceManager::reopenBottomTab(const juce::String& name)
{
    auto it = closedTabs.find(name);
    if (it == closedTabs.end()) return;

    bottomTabs->addTab(name, it->second.colour, it->second.component, false);
    closedTabs.erase(it);

    if (onTabReopened)
        onTabReopened(name);
}

juce::StringArray WorkspaceManager::getClosedTabNames() const
{
    juce::StringArray names;
    for (auto& [name, info] : closedTabs)
        names.add(name);
    return names;
}

void WorkspaceManager::setLeftSidebarVisible(bool visible)
{
    leftSidebarVisible = visible;
    leftContainer.setVisible(visible);
    leftSidebarResizer->setVisible(visible && rightSidebarVisible);
    resized();
}

void WorkspaceManager::setRightSidebarVisible(bool visible)
{
    rightSidebarVisible = visible;
    rightContainer.setVisible(visible);
    rightSidebarResizer->setVisible(visible && leftSidebarVisible);
    resized();
}

juce::ValueTree WorkspaceManager::saveLayout()
{
    juce::ValueTree vt("WorkspaceLayout");
    vt.setProperty("leftVisible", leftSidebarVisible, nullptr);
    vt.setProperty("rightVisible", rightSidebarVisible, nullptr);
    if (bottomTabs)
        vt.setProperty("activeTab", bottomTabs->getCurrentTabIndex(), nullptr);

    // Save tab order
    juce::StringArray tabOrder;
    for (int i = 0; i < bottomTabs->getNumTabs(); ++i)
        tabOrder.add(bottomTabs->getTabNames()[i]);
    vt.setProperty("tabOrder", tabOrder.joinIntoString("|"), nullptr);

    // Save closed tabs
    auto closedNames = getClosedTabNames();
    vt.setProperty("closedTabs", closedNames.joinIntoString("|"), nullptr);

    return vt;
}

void WorkspaceManager::loadLayout(const juce::ValueTree& state)
{
    if (state.hasType("WorkspaceLayout"))
    {
        leftSidebarVisible = state.getProperty("leftVisible", true);
        rightSidebarVisible = state.getProperty("rightVisible", true);
        if (bottomTabs && state.hasProperty("activeTab"))
            bottomTabs->setCurrentTabIndex(state.getProperty("activeTab"));
        resized();
    }
}

void WorkspaceManager::layoutCenter(juce::Rectangle<int> centerBounds)
{
    if (timelineComponent)
    {
        juce::Component* comps[] = { timelineComponent,
                                     timelineBottomResizer.get(),
                                     bottomTabs.get() };

        verticalLayout.layOutComponents(comps, 3,
                                        centerBounds.getX(), centerBounds.getY(),
                                        centerBounds.getWidth(), centerBounds.getHeight(),
                                        true, true);
        timelineBottomResizer->setVisible(true);
    }
    else
    {
        // No timeline — give all space to bottom tabs
        bottomTabs->setBounds(centerBounds);
        timelineBottomResizer->setVisible(false);
    }
}

void WorkspaceManager::updateTabTooltips()
{
    if (!bottomTabs) return;

    auto& tabBar = bottomTabs->getTabbedButtonBar();
    for (int i = 0; i < tabBar.getNumTabs(); ++i)
    {
        if (auto* tabBtn = tabBar.getTabButton(i))
            tabBtn->setTooltip(tabBar.getTabNames()[i]);
    }
}

void WorkspaceManager::showRightSidebarPanel(const juce::String& panelName)
{
    // Find the named panel in the right sidebar, expand it
    for (int i = 0; i < rightContainer.getNumPanels(); ++i)
    {
        if (auto* panel = rightContainer.getPanel(i))
        {
            if (auto* cp = dynamic_cast<CollapsiblePanel*>(panel))
            {
                if (cp->getPanelName() == panelName)
                {
                    cp->setVisible(true);
                    rightContainer.expandPanelFully(cp, true);
                    return;
                }
            }
        }
    }
}
