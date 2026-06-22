#pragma once
#include <JuceHeader.h>
#include "OrpheusLookAndFeel.h"

//==============================================================================
// CollapsiblePanel — A panel with a clickable header that collapses/expands.
// Panels stack vertically in sidebars. Retains undock capability.
//==============================================================================
class CollapsiblePanel : public juce::Component
{
public:
    CollapsiblePanel(const juce::String& name, std::unique_ptr<juce::Component> content);
    ~CollapsiblePanel() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;

    bool isCollapsed() const { return collapsed; }
    void setCollapsed(bool shouldCollapse);
    int getDesiredHeight() const;

    juce::String getPanelName() const { return panelName; }
    juce::Component* getContent() { return content.get(); }

    // Undock support
    void setUndocked(bool shouldUndock);
    bool isUndocked() const { return floatingWindow != nullptr; }

    static constexpr int headerHeight = 26;
    static constexpr int expandedMinHeight = 120;

private:
    class FloatingWindow : public juce::DocumentWindow
    {
    public:
        FloatingWindow(CollapsiblePanel& owner, const juce::String& name);
        void closeButtonPressed() override;
    private:
        CollapsiblePanel& owner;
    };

    juce::String panelName;
    std::unique_ptr<juce::Component> content;
    std::unique_ptr<FloatingWindow> floatingWindow;
    bool collapsed = false;
    int expandedHeight = 200; // Remembers last expanded height

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CollapsiblePanel)
};
