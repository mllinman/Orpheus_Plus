#pragma once
#include <JuceHeader.h>
#include "../Audio/AudioEngine.h"
#include "../Project/AppState.h"

class TrackLaneComponent;

//==============================================================================
class TimelineComponent : public juce::Component,
                          public juce::ScrollBar::Listener,
                          public AudioEngine::Listener,
                          private juce::Timer
{
public:
    TimelineComponent(AudioEngine& engine, AppState& state,
                      juce::ApplicationCommandManager& commands);
    ~TimelineComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

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
    double pixelToTime(double x) const;
    double timeToPixel(double t) const;

    // AudioEngine::Listener
    void trackListChanged() override { rebuildTracks(); }

private:
    void timerCallback() override;
    void scrollBarMoved(juce::ScrollBar* bar, double newRange) override;
    void paintRuler(juce::Graphics& g, juce::Rectangle<int> rulerBounds);
    void paintPlayhead(juce::Graphics& g);
    void paintLoopRegion(juce::Graphics& g);

    static constexpr int  RULER_HEIGHT    = 30;
    static constexpr int  TRACK_HEADER_W  = 180;
    static constexpr int  DEFAULT_TRACK_H = 80;

    AudioEngine& audioEngine;
    AppState&    appState;
    juce::ApplicationCommandManager& commandManager;

    juce::OwnedArray<TrackLaneComponent> trackLanes;
    juce::Viewport trackViewport;
    juce::Component trackContainer; // contains all lane components

    juce::ScrollBar horizontalScrollBar { false };
    juce::ScrollBar verticalScrollBar   { true  };

    double pixelsPerSecond       = 100.0;
    double horizontalScrollOffset = 0.0;
    double verticalScrollOffset   = 0.0;

    // Loop region
    bool   loopEnabled = false;
    double loopStart   = 0.0;
    double loopEnd     = 4.0; // bars

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimelineComponent)
};
