#pragma once
#include <JuceHeader.h>

/**
    A component that can be "torn off" from a tabbed view into its own floating window.
*/
class DockablePanel : public juce::Component
{
public:
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void panelUndocked(DockablePanel* panel) = 0;
        virtual void panelRedocked(DockablePanel* panel) = 0;
    };

    DockablePanel(const juce::String& name, std::unique_ptr<juce::Component> content);
    ~DockablePanel() override;

    void resized() override;
    void paint(juce::Graphics& g) override;

    void setUndocked(bool shouldBeUndocked, juce::Point<int> screenPos = {});
    bool isUndocked() const { return floatingWindow != nullptr; }

    void addListener(Listener* l) { listeners.add(l); }
    void removeListener(Listener* l) { listeners.remove(l); }

    juce::Component* getContent() { return content.get(); }
    juce::String getPanelName() const { return panelName; }

private:
    class FloatingWindow : public juce::DocumentWindow
    {
    public:
        FloatingWindow(DockablePanel& owner, const juce::String& name);
        void closeButtonPressed() override;

    private:
        DockablePanel& owner;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FloatingWindow)
    };

    juce::String panelName;
    std::unique_ptr<juce::Component> content;
    std::unique_ptr<FloatingWindow> floatingWindow;

    juce::TextButton undockButton { "Undock" };
    juce::ListenerList<Listener> listeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DockablePanel)
};
