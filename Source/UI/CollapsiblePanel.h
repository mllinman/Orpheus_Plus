#pragma once
#include <JuceHeader.h>
#include "OrpheusLookAndFeel.h"

//==============================================================================
// CollapsiblePanel — A panel with a clickable header that collapses/expands.
// Panels stack vertically in sidebars. Retains undock and close capability.
// Header bar is a drag handle for reordering within sidebars.
//==============================================================================
class CollapsiblePanel : public juce::Component,
                         public juce::SettableTooltipClient
{
public:
    CollapsiblePanel(const juce::String& name, std::unique_ptr<juce::Component> content);
    ~CollapsiblePanel() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;

    bool isCollapsed() const { return collapsed; }
    void setCollapsed(bool shouldCollapse);
    int getDesiredHeight() const;

    juce::String getPanelName() const { return panelName; }
    juce::Component* getContent() { return content.get(); }

    // Undock support
    void setUndocked(bool shouldUndock);
    bool isUndocked() const { return floatingWindow != nullptr; }

    // Close callback (fired when × is clicked)
    std::function<void()> onClose;

    static constexpr int headerHeight = 22;
    static constexpr int expandedMinHeight = 120;

private:
    class FloatingWindow : public juce::DocumentWindow
    {
    public:
        FloatingWindow(CollapsiblePanel& owner, const juce::String& name);
        ~FloatingWindow() override;
        void closeButtonPressed() override;
        
        struct RightClickDragger : public juce::MouseListener
        {
            juce::ComponentDragger dragger;
            FloatingWindow* window = nullptr;
            
            void mouseDown(const juce::MouseEvent& e) override
            {
                if (e.mods.isRightButtonDown() && window)
                    dragger.startDraggingComponent(window, e.getEventRelativeTo(window));
            }
            void mouseDrag(const juce::MouseEvent& e) override
            {
                if (e.mods.isRightButtonDown() && window)
                    dragger.dragComponent(window, e.getEventRelativeTo(window), nullptr);
            }
        };

    private:
        CollapsiblePanel& owner;
        RightClickDragger rightClickDragger;
    };

    juce::String panelName;
    std::unique_ptr<juce::Component> content;
    std::unique_ptr<FloatingWindow> floatingWindow;
    bool collapsed = false;
    int expandedHeight = 200; // Remembers last expanded height
    bool dragging = false;

    juce::ResizableEdgeComponent bottomResizer { this, nullptr, juce::ResizableEdgeComponent::bottomEdge };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CollapsiblePanel)
};
