#include "CollapsiblePanel.h"

CollapsiblePanel::CollapsiblePanel(const juce::String& name, std::unique_ptr<juce::Component> contentComponent)
    : panelName(name), content(std::move(contentComponent))
{
    if (content != nullptr)
        addAndMakeVisible(content.get());
}

CollapsiblePanel::~CollapsiblePanel()
{
    floatingWindow.reset();
}

void CollapsiblePanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    auto header = bounds.removeFromTop(headerHeight).toFloat();

    // Header gradient (darker metallic)
    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0xff1e1e2e), 0, header.getY(),
        juce::Colour(0xff141420), 0, header.getBottom(), false));
    g.fillRect(header);

    // Bottom line
    g.setColour(OrpheusLookAndFeel::borderSubtle());
    g.drawHorizontalLine((int)header.getBottom(), 0, (float)getWidth());

    // Collapse chevron
    float chevX = header.getX() + 10;
    float chevY = header.getCentreY();
    juce::Path chevron;
    if (collapsed)
    {
        chevron.addTriangle(chevX, chevY - 4, chevX, chevY + 4, chevX + 5, chevY);
    }
    else
    {
        chevron.addTriangle(chevX - 3, chevY - 2, chevX + 3, chevY - 2, chevX, chevY + 3);
    }
    g.setColour(OrpheusLookAndFeel::textSecondary());
    g.fillPath(chevron);

    // Panel name
    g.setColour(collapsed ? OrpheusLookAndFeel::textMuted() : OrpheusLookAndFeel::textPrimary());
    g.setFont(juce::Font(11.5f).boldened());
    g.drawText(panelName, header.withTrimmedLeft(24), juce::Justification::centredLeft);

    // Undock button hint
    if (isMouseOver(true) && !collapsed)
    {
        auto undockArea = header.removeFromRight(40);
        g.setColour(OrpheusLookAndFeel::textMuted());
        g.setFont(juce::Font(10.0f));
        g.drawText(juce::String::charToString(0x2197), undockArea, juce::Justification::centred); // ↗
    }

    // Body background when expanded
    if (!collapsed && floatingWindow == nullptr)
    {
        g.setColour(OrpheusLookAndFeel::bgDarkest().withAlpha(0.7f));
        g.fillRect(bounds);
    }
}

void CollapsiblePanel::resized()
{
    if (!collapsed && content != nullptr && floatingWindow == nullptr)
    {
        auto contentBounds = getLocalBounds().withTrimmedTop(headerHeight);
        content->setBounds(contentBounds);
        content->setVisible(true);
    }
    else if (content != nullptr && floatingWindow == nullptr)
    {
        content->setVisible(false);
    }
}

void CollapsiblePanel::mouseDown(const juce::MouseEvent& e)
{
    auto header = getLocalBounds().removeFromTop(headerHeight);
    if (header.contains(e.getPosition()))
    {
        // Check if clicking the undock area
        if (e.getPosition().getX() > getWidth() - 40 && !collapsed)
        {
            setUndocked(true);
            return;
        }
        setCollapsed(!collapsed);
    }
}

void CollapsiblePanel::setCollapsed(bool shouldCollapse)
{
    if (collapsed == shouldCollapse) return;
    if (!collapsed) expandedHeight = getHeight(); // Remember current height
    collapsed = shouldCollapse;
    
    if (auto* parent = getParentComponent())
        parent->resized(); // Trigger parent re-layout
    
    resized();
    repaint();
}

int CollapsiblePanel::getDesiredHeight() const
{
    if (collapsed) return headerHeight;
    return juce::jmax(expandedMinHeight, expandedHeight);
}

void CollapsiblePanel::setUndocked(bool shouldUndock)
{
    if (shouldUndock && floatingWindow == nullptr)
    {
        floatingWindow = std::make_unique<FloatingWindow>(*this, panelName);
        if (content != nullptr)
        {
            removeChildComponent(content.get());
            floatingWindow->setContentNonOwned(content.get(), true);
        }
        floatingWindow->setVisible(true);
        floatingWindow->centreWithSize(500, 400);
        repaint();
    }
    else if (!shouldUndock && floatingWindow != nullptr)
    {
        if (content != nullptr)
        {
            floatingWindow->clearContentComponent();
            addAndMakeVisible(content.get());
        }
        floatingWindow.reset();
        resized();
        repaint();
    }
}

CollapsiblePanel::FloatingWindow::FloatingWindow(CollapsiblePanel& o, const juce::String& name)
    : DocumentWindow(name, juce::Colours::black, DocumentWindow::allButtons), owner(o)
{
    setUsingNativeTitleBar(true);
    setResizable(true, false);
}

void CollapsiblePanel::FloatingWindow::closeButtonPressed()
{
    owner.setUndocked(false);
}
