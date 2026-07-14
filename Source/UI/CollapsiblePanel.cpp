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
    float chevX = header.getX() + 8;
    float chevY = header.getCentreY();
    juce::Path chevron;
    if (collapsed)
    {
        chevron.addTriangle(chevX, chevY - 3, chevX, chevY + 3, chevX + 4, chevY);
    }
    else
    {
        chevron.addTriangle(chevX - 2, chevY - 2, chevX + 2, chevY - 2, chevX, chevY + 2);
    }
    g.setColour(OrpheusLookAndFeel::textSecondary());
    g.fillPath(chevron);

    // Panel name
    g.setColour(collapsed ? OrpheusLookAndFeel::textMuted() : OrpheusLookAndFeel::textPrimary());
    g.setFont(juce::Font(10.5f).boldened());
    g.drawText(panelName, header.withTrimmedLeft(20).withTrimmedRight(60),
               juce::Justification::centredLeft);

    // Right-side buttons (only on hover)
    if (isMouseOver(true) && !collapsed)
    {
        // Close × button
        auto closeArea = header.removeFromRight(24);
        g.setColour(OrpheusLookAndFeel::textMuted());
        g.setFont(juce::Font(12.0f));
        g.drawText(juce::String::charToString(0x2715), closeArea, juce::Justification::centred); // ✕

        // Undock button
        auto undockArea = header.removeFromRight(24);
        g.setFont(juce::Font(10.0f));
        g.drawText(juce::String::charToString(0x2197), undockArea, juce::Justification::centred); // ↗
    }

    // Drag handle indicator (subtle dots when hovering)
    if (isMouseOver(true))
    {
        g.setColour(OrpheusLookAndFeel::textMuted().withAlpha(0.3f));
        float dotX = header.getX() + 3;
        for (int i = 0; i < 3; ++i)
        {
            float dotY = header.getCentreY() - 4.0f + i * 4.0f;
            g.fillEllipse(dotX, dotY, 2.0f, 2.0f);
        }
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
        auto contentBounds = getLocalBounds().withTrimmedTop(headerHeight).reduced(2);
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
    if (!header.contains(e.getPosition())) return;

    int x = e.getPosition().getX();

    // Close button area (rightmost 24px of header)
    if (x > getWidth() - 24 && !collapsed)
    {
        if (onClose)
            onClose();
        return;
    }

    // Undock area (next 24px from right)
    if (x > getWidth() - 48 && !collapsed)
    {
        setUndocked(true);
        return;
    }

    // Otherwise toggle collapse
    setCollapsed(!collapsed);
}

void CollapsiblePanel::mouseDrag(const juce::MouseEvent& e)
{
    auto header = getLocalBounds().removeFromTop(headerHeight);
    if (!header.contains(e.getMouseDownPosition())) return;

    if (e.getDistanceFromDragStart() > 8 && !dragging)
    {
        dragging = true;
        if (auto* dragContainer = juce::DragAndDropContainer::findParentDragContainerFor(this))
        {
            dragContainer->startDragging("SidebarPanel:" + panelName, this);
        }
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
    
    rightClickDragger.window = this;
    addMouseListener(&rightClickDragger, true); // Catch events from all children
}

CollapsiblePanel::FloatingWindow::~FloatingWindow()
{
    removeMouseListener(&rightClickDragger);
}

void CollapsiblePanel::FloatingWindow::closeButtonPressed()
{
    owner.setUndocked(false);
}
