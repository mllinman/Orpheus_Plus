#include "ToolbarComponent.h"

ToolbarComponent::ToolbarComponent()
{
    // ── File Group ───────────────────────────────────────────────────────
    addCategoryLabel("File", getCategoryColour("File"));
    addIcon("File", DAWIcons::newProject,  "New Project",  "New",    onNewProject);
    addIcon("File", DAWIcons::openFile,    "Open Project", "Open",   onOpenProject);
    addIcon("File", DAWIcons::save,        "Save",         "Save",   onSaveProject);
    addIcon("File", DAWIcons::exportIcon,  "Export",       "Export",  onExport);
    addSeparator();

    // ── Edit Group ───────────────────────────────────────────────────────
    addCategoryLabel("Edit", getCategoryColour("Edit"));
    addIcon("Edit", DAWIcons::undo,  "Undo",  "Undo",  onUndo);
    addIcon("Edit", DAWIcons::redo,  "Redo",  "Redo",  onRedo);
    addIcon("Edit", DAWIcons::cut,   "Cut",   "Cut",   onCut);
    addIcon("Edit", DAWIcons::copy,  "Copy",  "Copy",  onCopy);
    addIcon("Edit", DAWIcons::paste, "Paste", "Paste", onPaste);
    addSeparator();

    // ── Tools Group ──────────────────────────────────────────────────────
    addCategoryLabel("Tools", getCategoryColour("Tools"));
    addIcon("Tools", DAWIcons::selectArrow, "Select (V)",    "Select",  onSelectTool);
    addIcon("Tools", DAWIcons::pencilDraw,  "Draw (D)",      "Draw",    onDrawTool);
    addIcon("Tools", DAWIcons::sliceTool,   "Slice (S)",     "Slice",   onSliceTool);
    addIcon("Tools", DAWIcons::eraser,      "Eraser (E)",    "Erase",   onEraserTool);
    addIcon("Tools", DAWIcons::muteTool,    "Mute Tool (M)", "Mute",    onMuteTool);
    addSeparator();

    // ── View Group ───────────────────────────────────────────────────────
    addCategoryLabel("View", getCategoryColour("View"));
    addIcon("View", DAWIcons::mixerIcon,       "Mixer",        "Mixer",    onShowMixer);
    addIcon("View", DAWIcons::pianoRollIcon,   "Piano Roll",   "Piano",    onShowPianoRoll);
    addIcon("View", DAWIcons::sessionViewIcon, "Session View", "Session",  onShowSession);
    addIcon("View", DAWIcons::spectrumIcon,    "Spectrum",     "Spectrum", onShowSpectrum);
    addIcon("View", DAWIcons::aiBrain,         "AI Co-Pilot",  "CoPilot", onShowCoPilot);
    addSeparator();

    // ── AI Group ─────────────────────────────────────────────────────────
    addCategoryLabel("AI", getCategoryColour("AI"));
    addIcon("AI", DAWIcons::stemSplit,     "Stem Separation", "Stems",   onShowStemSep);
    addIcon("AI", DAWIcons::autoTuneIcon,  "AutoTune",        "Tune",    onShowAutoTune);
    addIcon("AI", DAWIcons::humanizer,     "AI Humanizer",    "Human",   onShowHumanizer);
    addIcon("AI", DAWIcons::textToSample,  "Text-to-Sample",  "TTS",     onShowTextToSample);
    addIcon("AI", DAWIcons::autoMix,       "Auto-Mix",        "AutoMix", onShowAutoMix);
    addSeparator();

    // ── Settings ─────────────────────────────────────────────────────────
    addIcon("Settings", DAWIcons::settings, "Settings", "Settings", onShowSettings);
}

ToolbarComponent::IconButton* ToolbarComponent::addIcon(
    const juce::String& group, IconButton::PathFn fn,
    const juce::String& tooltip, const juce::String& label,
    std::function<void()>& callback)
{
    auto colour = getCategoryColour(group);
    auto* btn = new IconButton(std::move(fn), tooltip, label, group, colour);
    btn->onClick = [&callback]() { if (callback) callback(); };
    btn->setTooltip(tooltip);
    items.add(btn);
    addAndMakeVisible(btn);
    return btn;
}

void ToolbarComponent::addSeparator()
{
    auto* sep = new ToolbarSeparator();
    items.add(sep);
    addAndMakeVisible(sep);
}

void ToolbarComponent::addCategoryLabel(const juce::String& name, juce::Colour colour)
{
    auto* label = new CategoryLabel(name, colour);
    items.add(label);
    addAndMakeVisible(label);
}

void ToolbarComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Metallic toolbar gradient
    g.setGradientFill(juce::ColourGradient(
        OrpheusLookAndFeel::bgDarker().brighter(0.08f), 0, 0,
        OrpheusLookAndFeel::bgDarkest(), 0, bounds.getHeight(), false));
    g.fillRect(bounds);

    // Bottom border with subtle accent glow
    g.setColour(OrpheusLookAndFeel::borderSubtle());
    g.drawHorizontalLine(getHeight() - 1, 0, (float)getWidth());
}

void ToolbarComponent::resized()
{
    int x = 6;
    int totalH = getHeight();
    int btnSize = totalH - 4; // Full height for icon + label
    int sepW = 10;
    int catLabelW = 28;

    for (auto* item : items)
    {
        if (dynamic_cast<ToolbarSeparator*>(item))
        {
            item->setBounds(x, 0, sepW, totalH);
            x += sepW;
        }
        else if (dynamic_cast<CategoryLabel*>(item))
        {
            item->setBounds(x, 0, catLabelW, totalH);
            x += catLabelW;
        }
        else
        {
            item->setBounds(x, 2, btnSize, btnSize);
            x += btnSize + 1;
        }
    }
}
