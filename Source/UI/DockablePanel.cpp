#include "DockablePanel.h"
#include "OrpheusLookAndFeel.h"

DockablePanel::DockablePanel(const juce::String& name, std::unique_ptr<juce::Component> contentComponent)
    : panelName(name), content(std::move(contentComponent))
{
    addAndMakeVisible(undockButton);
    undockButton.onClick = [this] { setUndocked(true); };

    if (content != nullptr)
        addAndMakeVisible(content.get());
}

DockablePanel::~DockablePanel()
{
    floatingWindow.reset();
}

void DockablePanel::resized()
{
    auto bounds = getLocalBounds();
    auto titleArea = bounds.removeFromTop(30);

    undockButton.setBounds(titleArea.removeFromRight(80).reduced(4));

    if (content != nullptr && floatingWindow == nullptr)
    {
        content->setBounds(bounds);
    }
}

void DockablePanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    auto titleArea = bounds.removeFromTop(30);

    g.setColour(juce::Colour(0xff16213e));
    g.fillRect(titleArea);

    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(14.0f).boldened());
    g.drawText(panelName, titleArea.reduced(10, 0), juce::Justification::centredLeft);

    if (floatingWindow != nullptr)
    {
        g.setColour(juce::Colours::grey.withAlpha(0.2f));
        g.drawText("Panel is Undocked", bounds, juce::Justification::centred);
    }
}

void DockablePanel::setUndocked(bool shouldBeUndocked, juce::Point<int> screenPos)
{
    if (shouldBeUndocked && floatingWindow == nullptr)
    {
        floatingWindow = std::make_unique<FloatingWindow>(*this, panelName);
        
        if (content != nullptr)
        {
            removeChildComponent(content.get());
            floatingWindow->setContentNonOwned(content.get(), true);
        }

        floatingWindow->setVisible(true);
        
        if (screenPos != juce::Point<int>())
            floatingWindow->setTopLeftPosition(screenPos);
        else
            floatingWindow->centreWithSize(800, 600);

        listeners.call(&Listener::panelUndocked, this);
        repaint();
    }
    else if (!shouldBeUndocked && floatingWindow != nullptr)
    {
        if (content != nullptr)
        {
            floatingWindow->clearContentComponent();
            addAndMakeVisible(content.get());
        }

        floatingWindow.reset();
        resized();
        listeners.call(&Listener::panelRedocked, this);
        repaint();
    }
}

//==============================================================================
DockablePanel::FloatingWindow::FloatingWindow(DockablePanel& o, const juce::String& name)
    : DocumentWindow(name, juce::Colours::black, DocumentWindow::allButtons), owner(o)
{
    setUsingNativeTitleBar(true);
    setResizable(true, false);
}

void DockablePanel::FloatingWindow::closeButtonPressed()
{
    owner.setUndocked(false);
}
