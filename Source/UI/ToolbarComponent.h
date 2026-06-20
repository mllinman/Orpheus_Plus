#pragma once
#include <JuceHeader.h>
#include "DAWIcons.h"
#include "OrpheusLookAndFeel.h"

//==============================================================================
// ToolbarComponent — Professional DAW icon toolbar at the top of the window.
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

private:
    struct IconButton : public juce::Component,
                        public juce::SettableTooltipClient
    {
        using PathFn = std::function<juce::Path(juce::Rectangle<float>)>;
        PathFn pathFn;
        juce::String tooltip;
        std::function<void()> onClick;
        bool isToggle = false;
        bool toggled = false;
        bool hovered = false;

        IconButton(PathFn fn, const juce::String& tip)
            : pathFn(std::move(fn)), tooltip(tip) 
        {
            setRepaintsOnMouseActivity(true);
        }

        void paint(juce::Graphics& g) override
        {
            auto bounds = getLocalBounds().toFloat().reduced(2.0f);
            hovered = isMouseOver();

            if (toggled)
            {
                g.setColour(OrpheusLookAndFeel::accentPrimary().withAlpha(0.25f));
                g.fillRoundedRectangle(bounds, 4.0f);
                g.setColour(OrpheusLookAndFeel::accentPrimary());
                g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
            }
            else if (hovered)
            {
                g.setColour(OrpheusLookAndFeel::bgHover());
                g.fillRoundedRectangle(bounds, 4.0f);
            }

            // Draw the icon path
            auto iconArea = bounds.reduced(4.0f);
            auto path = pathFn(iconArea);
            g.setColour(toggled ? OrpheusLookAndFeel::accentPrimary()
                        : hovered ? OrpheusLookAndFeel::textPrimary()
                        : OrpheusLookAndFeel::textSecondary());
            g.strokePath(path, juce::PathStrokeType(1.5f));
            g.fillPath(path);
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

    // Separator
    struct ToolbarSeparator : public juce::Component
    {
        void paint(juce::Graphics& g) override
        {
            g.setColour(OrpheusLookAndFeel::borderDefault());
            g.drawVerticalLine(getWidth() / 2, 4.0f, (float)getHeight() - 4.0f);
        }
    };

    IconButton* addIcon(const juce::String& group, IconButton::PathFn fn,
                        const juce::String& tooltip, std::function<void()>& callback);
    void addSeparator();

    juce::OwnedArray<juce::Component> items; // mix of IconButton and ToolbarSeparator

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ToolbarComponent)
};
