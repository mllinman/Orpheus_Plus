#include "WorkspaceManager.h"
#include "OrpheusLookAndFeel.h"

//==============================================================================
// SidebarContainer — proportionally distributes height among expanded panels
//==============================================================================
void WorkspaceManager::SidebarContainer::resized()
{
    int viewportH = getParentHeight();
    if (viewportH <= 0) viewportH = 600;

    // Count collapsed vs expanded
    int collapsedTotal = 0;
    int expandedCount = 0;
    for (auto* panel : panels)
    {
        if (panel->isCollapsed())
            collapsedTotal += CollapsiblePanel::headerHeight;
        else
            expandedCount++;
    }

    // Distribute remaining height proportionally among expanded panels
    int availableForExpanded = juce::jmax(0, viewportH - collapsedTotal);
    int perExpandedHeight = expandedCount > 0
        ? juce::jmax(CollapsiblePanel::expandedMinHeight, availableForExpanded / expandedCount)
        : 0;

    int y = 0;
    for (auto* panel : panels)
    {
        int h = panel->isCollapsed() ? CollapsiblePanel::headerHeight : perExpandedHeight;
        panel->setBounds(0, y, getWidth(), h);
        y += h;
    }
    setSize(getWidth(), juce::jmax(y, viewportH));
}

void WorkspaceManager::SidebarContainer::addPanel(const juce::String& name,
                                                   std::unique_ptr<juce::Component> content)
{
    auto* cp = panels.add(new CollapsiblePanel(name, std::move(content)));
    addAndMakeVisible(cp);
    resized();
}

void WorkspaceManager::SidebarContainer::paint(juce::Graphics& g)
{
    // Draw drag-drop insertion indicator
    if (dropInsertIndex >= 0)
    {
        int y = 0;
        for (int i = 0; i < dropInsertIndex && i < panels.size(); ++i)
            y += panels[i]->getHeight();

        g.setColour(juce::Colour(0xff4ecdc4));
        g.fillRect(0, y - 1, getWidth(), 3);
    }
}

// Sidebar drag-and-drop reordering
bool WorkspaceManager::SidebarContainer::isInterestedInDragSource(const SourceDetails& details)
{
    return details.description.toString().startsWith("SidebarPanel:");
}

void WorkspaceManager::SidebarContainer::itemDragEnter(const SourceDetails&)
{
    repaint();
}

void WorkspaceManager::SidebarContainer::itemDragMove(const SourceDetails& details)
{
    int mouseY = details.localPosition.getY();
    int y = 0;
    dropInsertIndex = panels.size();
    for (int i = 0; i < panels.size(); ++i)
    {
        int midY = y + panels[i]->getHeight() / 2;
        if (mouseY < midY) { dropInsertIndex = i; break; }
        y += panels[i]->getHeight();
    }
    repaint();
}

void WorkspaceManager::SidebarContainer::itemDragExit(const SourceDetails&)
{
    dropInsertIndex = -1;
    repaint();
}

void WorkspaceManager::SidebarContainer::itemDropped(const SourceDetails& details)
{
    auto desc = details.description.toString();
    if (!desc.startsWith("SidebarPanel:")) { dropInsertIndex = -1; return; }

    auto panelName = desc.fromFirstOccurrenceOf("SidebarPanel:", false, false);
    int sourceIdx = -1;
    for (int i = 0; i < panels.size(); ++i)
        if (panels[i]->getPanelName() == panelName) { sourceIdx = i; break; }

    if (sourceIdx >= 0 && dropInsertIndex >= 0 && sourceIdx != dropInsertIndex)
    {
        auto* panel = panels[sourceIdx];
        panels.move(sourceIdx, dropInsertIndex > sourceIdx ? dropInsertIndex - 1 : dropInsertIndex);
        resized();
    }

    dropInsertIndex = -1;
    repaint();
}

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

    // Horizontal layout: [0]=leftSidebar, [1]=resizer, [2]=center, [3]=resizer, [4]=rightSidebar
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
        juce::Component* hComps[] = {
            &leftViewport, leftSidebarResizer.get(),
            nullptr, // placeholder — we'll set center bounds manually
            rightSidebarResizer.get(), &rightViewport
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

        // Sync sidebar container sizes
        leftContainer.setSize(leftViewport.getWidth() - leftViewport.getScrollBarThickness(),
                              leftContainer.getHeight());
        leftContainer.resized();

        rightContainer.setSize(rightViewport.getWidth() - rightViewport.getScrollBarThickness(),
                               rightContainer.getHeight());
        rightContainer.resized();

        // Vertical layout inside center
        layoutCenter(centerBounds);
    }
    else if (leftSidebarVisible)
    {
        // Left sidebar + center only
        auto leftArea = bounds.removeFromLeft(240);
        leftViewport.setBounds(leftArea);
        leftViewport.setVisible(true);
        leftSidebarResizer->setVisible(false);
        rightSidebarResizer->setVisible(false);
        rightViewport.setVisible(false);

        leftContainer.setSize(leftArea.getWidth() - leftViewport.getScrollBarThickness(),
                              leftContainer.getHeight());
        leftContainer.resized();

        layoutCenter(bounds);
    }
    else if (rightSidebarVisible)
    {
        // Center + right sidebar only
        auto rightArea = bounds.removeFromRight(280);
        rightViewport.setBounds(rightArea);
        rightViewport.setVisible(true);
        leftSidebarResizer->setVisible(false);
        rightSidebarResizer->setVisible(false);
        leftViewport.setVisible(false);

        rightContainer.setSize(rightArea.getWidth() - rightViewport.getScrollBarThickness(),
                               rightContainer.getHeight());
        rightContainer.resized();

        layoutCenter(bounds);
    }
    else
    {
        // No sidebars
        leftViewport.setVisible(false);
        rightViewport.setVisible(false);
        leftSidebarResizer->setVisible(false);
        rightSidebarResizer->setVisible(false);
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
    leftViewport.setVisible(visible);
    leftSidebarResizer->setVisible(visible && rightSidebarVisible);
    resized();
}

void WorkspaceManager::setRightSidebarVisible(bool visible)
{
    rightSidebarVisible = visible;
    rightViewport.setVisible(visible);
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
