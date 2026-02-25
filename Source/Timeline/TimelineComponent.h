#pragma once
#include <JuceHeader.h>
#include "../Audio/AudioEngine.h"
#include "../Project/AppState.h"

class TrackLaneComponent;

//==============================================================================
class TimelineComponent : public juce::Component,
                          public juce::FileDragAndDropTarget,
                          public juce::ScrollBar::Listener,
                          public AudioEngine::Listener,
                          public juce::ChangeBroadcaster,
                          private juce::Timer
{
public:
    // Edit Tools
    enum class EditTool { Select, Split, Draw, Erase };
    void setTool(EditTool t) { currentTool = t; sendChangeMessage(); } // Broadcast change? or just internal state. TrackLanes need to know? They reference timeline.
    EditTool getTool() const { return currentTool; }

    // Grid Snapping
    enum class SnapTo { Bar, Beat, Half, Quarter, Eighth, Sixteenth, Off };
    void setSnapTo(SnapTo s) { currentSnapMode = s; }
    SnapTo getSnapTo() const { return currentSnapMode; }
    double snapToGrid(double time) const;

    TimelineComponent(AudioEngine& engine, AppState& state,
                      juce::ApplicationCommandManager& commands);
    ~TimelineComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    bool keyPressed(const juce::KeyPress& key) override;

    // Selection & Clipboard
    void clearAllSelections();
    void copySelection();
    void pasteSelection();

    // FileDragAndDropTarget
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

    // Rebuild track lanes from AppState
    void rebuildTracks();

    // Zoom
    void setPixelsPerSecond(double pps);
    double getPixelsPerSecond() const { return pixelsPerSecond; }
    void zoomIn()  { setPixelsPerSecond(pixelsPerSecond * 1.25); }
    void zoomOut() { setPixelsPerSecond(pixelsPerSecond / 1.25); }

    // Scroll
    double getHorizontalScrollPosition() const { return horizontalScrollOffset; }
    void   scrollToPosition(double positionInSeconds);

    // Convert between pixel and time coords
    // Convert between pixel and time coords
    // "View" methods allow for calculating coordinates relative to the visible window (TimelineComponent 0,0)
    double pixelToTime(double x) const;         // View Pixel -> Time
    double timeToPixel(double t) const;         // Time -> View Pixel

    // "Absolute" methods return coordinates relative to the start of the time (Time 0)
    // Used by components inside the scrolling container (TrackLaneComponent)
    double absolutePixelToTime(double x) const; // Absolute Pixel -> Time
    double timeToAbsolutePixel(double t) const; // Time -> Absolute Pixel

    // AudioEngine::Listener
    static constexpr int  RULER_HEIGHT    = 30;
    static constexpr int  DEFAULT_TRACK_H = 80;
    static constexpr int  TAKE_LANE_H     = 40;
    static constexpr int  AUTOMATION_LANE_H = 40;

    int getTrackHeaderWidth() const { return trackHeaderWidth; }

private:
    void timerCallback() override;
    void scrollBarMoved(juce::ScrollBar* bar, double newRange) override;
    void paintRuler(juce::Graphics& g, juce::Rectangle<int> rulerBounds);
    void paintPlayhead(juce::Graphics& g);
    void paintLoopRegion(juce::Graphics& g);

    EditTool currentTool = EditTool::Select;
    juce::TextButton selectButton { "Select" };
    juce::TextButton splitButton  { "Split" };
    juce::TextButton drawButton   { "Draw" };
    juce::TextButton eraseButton  { "Erase" };
    
    juce::ComboBox   snapComboBox;
    SnapTo           currentSnapMode = SnapTo::Beat;

    // Navigation & zoom controls
    juce::TextButton jumpStartBtn  { "|<" };
    juce::TextButton jumpEndBtn    { ">|" };
    juce::TextButton zoomInBtn     { "+" };
    juce::TextButton zoomOutBtn    { "-" };
    juce::Slider     zoomSlider;
    juce::Slider     trackHeightSlider;
    int              userTrackHeight = 80;

    AudioEngine& audioEngine;
    AppState&    appState;
    juce::ApplicationCommandManager& commandManager;

    juce::OwnedArray<TrackLaneComponent> trackLanes;
    juce::Viewport trackViewport;
    juce::Component trackContainer; // contains all lane components

    juce::ScrollBar horizontalScrollBar { false };
    juce::ScrollBar verticalScrollBar   { true  };

    // Resizable layout for Track Headers
    int trackHeaderWidth = 180;
    juce::StretchableLayoutManager horizontalLayout;
    std::unique_ptr<juce::StretchableLayoutResizerBar> resizerBar;

    double pixelsPerSecond       = 100.0;
    double horizontalScrollOffset = 0.0;
    double verticalScrollOffset   = 0.0;
    
    struct ClipboardItem {
        std::unique_ptr<Clip> clip;
        int trackIndex;
    };
    std::vector<ClipboardItem> clipboard;

    // Loop region
    bool   loopEnabled = false;
    double loopStart   = 0.0;
    double loopEnd     = 4.0; // bars

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimelineComponent)
};
