#include "WorkspaceManager.h"

WorkspaceManager::WorkspaceManager()
{
}

WorkspaceManager::~WorkspaceManager()
{
}

void WorkspaceManager::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey.darker());
}

void WorkspaceManager::resized()
{
    auto bounds = getLocalBounds();
    
    // Very simplified layout for now
    juce::Rectangle<int> leftArea;
    juce::Rectangle<int> rightArea;
    juce::Rectangle<int> topArea;
    juce::Rectangle<int> bottomArea;
    juce::Rectangle<int> centerArea = bounds;

    for (auto& info : panels)
    {
        if (info.panel->isUndocked()) continue;

        switch (info.zone)
        {
        case Zone::Left:
            info.panel->setBounds(centerArea.removeFromLeft(300));
            break;
        case Zone::Right:
            info.panel->setBounds(centerArea.removeFromRight(300));
            break;
        case Zone::Top:
            info.panel->setBounds(centerArea.removeFromTop(200));
            break;
        case Zone::Bottom:
            info.panel->setBounds(centerArea.removeFromBottom(250));
            break;
        case Zone::Center:
            // Center handled at the end
            break;
        }
    }

    for (auto& info : panels)
    {
        if (!info.panel->isUndocked() && info.zone == Zone::Center)
        {
            info.panel->setBounds(centerArea);
        }
    }
}

void WorkspaceManager::addPanel(DockablePanel* panel, Zone zone)
{
    panels.push_back({panel, zone});
    addAndMakeVisible(panel);
    resized();
}

void WorkspaceManager::removePanel(DockablePanel* panel)
{
    panels.erase(std::remove_if(panels.begin(), panels.end(),
        [panel](const PanelInfo& info) { return info.panel == panel; }), panels.end());
    removeChildComponent(panel);
    resized();
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
        resized();
    }
}
