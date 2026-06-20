#include "ToolbarComponent.h"

ToolbarComponent::ToolbarComponent()
{
    // ── File Group ───────────────────────────────────────────────────────
    addIcon("File", DAWIcons::newProject,  "New Project",  onNewProject);
    addIcon("File", DAWIcons::openFile,    "Open Project", onOpenProject);
    addIcon("File", DAWIcons::save,        "Save",         onSaveProject);
    addIcon("File", DAWIcons::exportIcon,  "Export",       onExport);
    addSeparator();

    // ── Edit Group ───────────────────────────────────────────────────────
    addIcon("Edit", DAWIcons::undo,  "Undo",  onUndo);
    addIcon("Edit", DAWIcons::redo,  "Redo",  onRedo);
    addIcon("Edit", DAWIcons::cut,   "Cut",   onCut);
    addIcon("Edit", DAWIcons::copy,  "Copy",  onCopy);
    addIcon("Edit", DAWIcons::paste, "Paste", onPaste);
    addSeparator();

    // ── Tools Group ──────────────────────────────────────────────────────
    addIcon("Tools", DAWIcons::selectArrow, "Select (V)",    onSelectTool);
    addIcon("Tools", DAWIcons::pencilDraw,  "Draw (D)",      onDrawTool);
    addIcon("Tools", DAWIcons::sliceTool,   "Slice (S)",     onSliceTool);
    addIcon("Tools", DAWIcons::eraser,      "Eraser (E)",    onEraserTool);
    addIcon("Tools", DAWIcons::muteTool,    "Mute Tool (M)", onMuteTool);
    addSeparator();

    // ── View Group ───────────────────────────────────────────────────────
    addIcon("View", DAWIcons::mixerIcon,       "Mixer",        onShowMixer);
    addIcon("View", DAWIcons::pianoRollIcon,   "Piano Roll",   onShowPianoRoll);
    addIcon("View", DAWIcons::sessionViewIcon, "Session View", onShowSession);
    addIcon("View", DAWIcons::spectrumIcon,    "Spectrum",     onShowSpectrum);
    addIcon("View", DAWIcons::aiBrain,         "AI Co-Pilot",  onShowCoPilot);
    addSeparator();

    // ── AI Group ─────────────────────────────────────────────────────────
    addIcon("AI", DAWIcons::stemSplit,     "Stem Separation", onShowStemSep);
    addIcon("AI", DAWIcons::autoTuneIcon,  "AutoTune",        onShowAutoTune);
    addIcon("AI", DAWIcons::humanizer,     "AI Humanizer",    onShowHumanizer);
    addIcon("AI", DAWIcons::textToSample,  "Text-to-Sample",  onShowTextToSample);
    addIcon("AI", DAWIcons::autoMix,       "Auto-Mix",        onShowAutoMix);
    addSeparator();

    // ── Settings ─────────────────────────────────────────────────────────
    addIcon("Settings", DAWIcons::settings, "Settings", onShowSettings);
}

ToolbarComponent::IconButton* ToolbarComponent::addIcon(
    const juce::String& /*group*/, IconButton::PathFn fn,
    const juce::String& tooltip, std::function<void()>& callback)
{
    auto* btn = new IconButton(std::move(fn), tooltip);
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
    int x = 8;
    int btnSize = getHeight() - 4;
    int sepW = 12;

    for (auto* item : items)
    {
        if (dynamic_cast<ToolbarSeparator*>(item))
        {
            item->setBounds(x, 0, sepW, getHeight());
            x += sepW;
        }
        else
        {
            item->setBounds(x, 2, btnSize, btnSize);
            x += btnSize + 2;
        }
    }
}
