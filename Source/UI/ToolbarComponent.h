#pragma once
#include <JuceHeader.h>
#include "DAWIcons.h"
#include "OrpheusLookAndFeel.h"

//==============================================================================
// ToolbarComponent — Professional DAW icon toolbar at the top of the window.
// Icons are color-coded by category and display names on hover.
//==============================================================================
class ToolbarComponent : public juce::Component
{
public:
    ToolbarComponent();
    ~ToolbarComponent() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

    // Callbacks for toolbar actions — wired by MainComponent
    std::function<void()> onNewProject, onOpenProject, onSaveProject, onExport;
    std::function<void()> onUndo, onRedo, onCut, onCopy, onPaste;
    std::function<void()> onSelectTool, onDrawTool, onSliceTool, onEraserTool, onMuteTool;
    std::function<void()> onShowMixer, onShowPianoRoll, onShowSession, onShowSpectrum, onShowCoPilot;
    std::function<void()> onShowStemSep, onShowAutoTune, onShowHumanizer, onShowTextToSample, onShowAutoMix;
    std::function<void()> onShowSettings;

    //── Category color palette ────────────────────────────────────────────
    static juce::Colour getCategoryColour(const juce::String& category)
    {
        if (category == "File")     return juce::Colour(0xff3b82f6);  // Blue
        if (category == "Edit")     return juce::Colour(0xfff59e0b);  // Amber
        if (category == "Tools")    return juce::Colour(0xff10b981);  // Emerald
        if (category == "View")     return juce::Colour(0xff06b6d4);  // Cyan
        if (category == "AI")       return juce::Colour(0xff8b5cf6);  // Violet
        if (category == "Settings") return juce::Colour(0xffa1a1aa);  // Zinc 400
        return OrpheusLookAndFeel::textSecondary();
    }

private:
    struct IconButton : public juce::Component,
                        public juce::SettableTooltipClient
    {
        using PathFn = std::function<juce::Path(juce::Rectangle<float>)>;
        PathFn pathFn;
        juce::String tooltip;
        juce::String label;       // Short display name
        juce::String group;       // Category group name
        juce::Colour catColour;   // Category accent colour
        std::function<void()> onClick;
        bool isToggle = false;
        bool toggled = false;
        bool hovered = false;

        IconButton(PathFn fn, const juce::String& tip, const juce::String& displayLabel,
                   const juce::String& groupName, juce::Colour colour)
            : pathFn(std::move(fn)), tooltip(tip), label(displayLabel),
              group(groupName), catColour(colour)
        {
            setRepaintsOnMouseActivity(true);
        }

        void paint(juce::Graphics& g) override
        {
            auto bounds = getLocalBounds().toFloat();
            auto iconArea = bounds.removeFromTop(bounds.getHeight() - 11.0f).reduced(2.0f);
            auto labelArea = bounds;

            hovered = isMouseOver();

            // Background highlight
            if (toggled)
            {
                g.setColour(catColour.withAlpha(0.2f));
                g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), 5.0f);
                g.setColour(catColour.withAlpha(0.5f));
                g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), 5.0f, 1.0f);
            }
            else if (hovered)
            {
                g.setColour(catColour.withAlpha(0.1f));
                g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), 5.0f);
            }

            // Draw the icon path with category colour
            auto path = pathFn(iconArea.reduced(3.0f));

            if (toggled)
                g.setColour(catColour);
            else if (hovered)
                g.setColour(catColour.brighter(0.3f));
            else
                g.setColour(catColour.withAlpha(0.65f));

            g.strokePath(path, juce::PathStrokeType(1.4f));
            g.fillPath(path);

            // Draw label text below icon
            g.setFont(juce::Font(8.5f));

            if (hovered || toggled)
                g.setColour(catColour);
            else
                g.setColour(OrpheusLookAndFeel::textMuted().withAlpha(0.7f));

            g.drawText(label, labelArea, juce::Justification::centredTop, false);
        }

        void mouseDown(const juce::MouseEvent&) override
        {
            if (isToggle) toggled = !toggled;
            if (onClick) onClick();
            repaint();
        }

        void mouseEnter(const juce::MouseEvent&) override { repaint(); }
        void mouseExit(const juce::MouseEvent&) override { repaint(); }
    };

    // Separator with optional category label
    struct ToolbarSeparator : public juce::Component
    {
        void paint(juce::Graphics& g) override
        {
            g.setColour(OrpheusLookAndFeel::borderDefault().withAlpha(0.5f));
            g.drawVerticalLine(getWidth() / 2, 6.0f, (float)getHeight() - 10.0f);
        }
    };

    // Category group label (drawn above a group of icons)
    struct CategoryLabel : public juce::Component
    {
        juce::String text;
        juce::Colour colour;

        CategoryLabel(const juce::String& t, juce::Colour c) : text(t), colour(c) {}

        void paint(juce::Graphics& g) override
        {
            g.setFont(juce::Font(8.0f).boldened());
            g.setColour(colour.withAlpha(0.45f));
            g.drawText(text.toUpperCase(), getLocalBounds(), juce::Justification::centredBottom, false);
        }
    };

    IconButton* addIcon(const juce::String& group, IconButton::PathFn fn,
                        const juce::String& tooltip, const juce::String& label,
                        std::function<void()>& callback);
    void addSeparator();
    void addCategoryLabel(const juce::String& name, juce::Colour colour);

    juce::OwnedArray<juce::Component> items; // mix of IconButton, ToolbarSeparator, CategoryLabel

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ToolbarComponent)
};
