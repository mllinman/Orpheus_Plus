#include "WorkspaceManager.h"
#include "OrpheusLookAndFeel.h"

WorkspaceManager::WorkspaceManager()
{
    centerTabs = std::make_unique<juce::TabbedComponent>(juce::TabbedButtonBar::TabsAtTop);
    centerTabs->setTabBarDepth(28);
    centerTabs->setOutline(0);
    centerTabs->setColour(juce::TabbedComponent::backgroundColourId, OrpheusLookAndFeel::bgDarkest());
    centerTabs->setColour(juce::TabbedComponent::outlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(centerTabs.get());
}

WorkspaceManager::~WorkspaceManager()
{
    // Detach center panels from tabs before destruction
    if (centerTabs)
    {
        centerTabs->clearTabs();
    }
}

void WorkspaceManager::paint(juce::Graphics& g)
{
    g.fillAll(OrpheusLookAndFeel::bgDarkest());
}

void WorkspaceManager::resized()
{
    auto bounds = getLocalBounds();
    
    // Layout side/top/bottom panels first, consuming space from bounds
    for (auto& info : panels)
    {
        if (info.panel->isUndocked()) continue;

        switch (info.zone)
        {
        case Zone::Left:
            info.panel->setBounds(bounds.removeFromLeft(300));
            break;
        case Zone::Right:
            info.panel->setBounds(bounds.removeFromRight(300));
            break;
        case Zone::Top:
            info.panel->setBounds(bounds.removeFromTop(200));
            break;
        case Zone::Bottom:
            info.panel->setBounds(bounds.removeFromBottom(250));
            break;
        case Zone::Center:
            // Handled by TabbedComponent below
            break;
        }
    }

    // Give remaining space to the tabbed center area
    if (centerTabs)
        centerTabs->setBounds(bounds);
}

void WorkspaceManager::addPanel(DockablePanel* panel, Zone zone)
{
    panels.push_back({panel, zone});

    if (zone == Zone::Center)
    {
        // Add to tabbed center view instead of stacking
        if (centerTabs)
        {
            centerTabs->addTab(panel->getPanelName(),
                               OrpheusLookAndFeel::bgDarker(),
                               panel, false);
        }
    }
    else
    {
        addAndMakeVisible(panel);
    }
    
    resized();
}

void WorkspaceManager::removePanel(DockablePanel* panel)
{
    // Remove from tabs if it was a center panel
    if (centerTabs)
    {
        for (int i = centerTabs->getNumTabs() - 1; i >= 0; --i)
        {
            if (centerTabs->getTabContentComponent(i) == panel)
            {
                centerTabs->removeTab(i);
                break;
            }
        }
    }

    panels.erase(std::remove_if(panels.begin(), panels.end(),
        [panel](const PanelInfo& info) { return info.panel == panel; }), panels.end());
    removeChildComponent(panel);
    resized();
}

void WorkspaceManager::showCenterPanel(const juce::String& panelName)
{
    if (centerTabs)
    {
        for (int i = 0; i < centerTabs->getNumTabs(); ++i)
        {
            if (centerTabs->getTabNames()[i] == panelName)
            {
                centerTabs->setCurrentTabIndex(i);
                return;
            }
        }
    }
}

juce::ValueTree WorkspaceManager::saveLayout()
{
    juce::ValueTree vt("WorkspaceLayout");
    for (const auto& info : panels)
    {
        juce::ValueTree p("Panel");
        p.setProperty("name", info.panel->getPanelName(), nullptr);
        p.setProperty("undocked", info.panel->isUndocked(), nullptr);
        p.setProperty("zone", static_cast<int>(info.zone), nullptr);
        vt.addChild(p, -1, nullptr);
    }
    
    // Save current center tab index
    if (centerTabs)
        vt.setProperty("centerTabIndex", centerTabs->getCurrentTabIndex(), nullptr);
    
    return vt;
}

void WorkspaceManager::loadLayout(const juce::ValueTree& state)
{
    if (state.hasType("WorkspaceLayout"))
    {
        for (auto child : state)
        {
            juce::String name = child.getProperty("name");
            bool undocked = child.getProperty("undocked");
            int zone = child.getProperty("zone");

            for (auto& info : panels)
            {
                if (info.panel->getPanelName() == name)
                {
                    info.zone = static_cast<Zone>(zone);
                    if (undocked) {
                        info.panel->setUndocked(true);
                    } else {
                        info.panel->setUndocked(false);
                    }
                }
            }
        }
        
        // Restore center tab
        if (centerTabs && state.hasProperty("centerTabIndex"))
            centerTabs->setCurrentTabIndex(state.getProperty("centerTabIndex"));
        
        resized();
    }
}
