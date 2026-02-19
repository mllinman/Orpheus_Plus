#include "TimelineComponent.h"
#include "TrackLaneComponent.h"

TimelineComponent::TimelineComponent(AudioEngine& e, AppState& s,
                                     juce::ApplicationCommandManager& c)
    : audioEngine(e), appState(s), commandManager(c)
{
    audioEngine.addListener(this);

    addAndMakeVisible(horizontalScrollBar);
    addAndMakeVisible(verticalScrollBar);
    addAndMakeVisible(trackViewport);

    trackViewport.setViewedComponent(&trackContainer, false);
    trackViewport.setScrollBarsShown(false, false);

    horizontalScrollBar.addListener(this);
    verticalScrollBar.addListener(this);

    startTimerHz(60);
}

TimelineComponent::~TimelineComponent()
{
    audioEngine.removeListener(this);
    stopTimer();
}

void TimelineComponent::rebuildTracks()
{
    trackLanes.clear();
    trackContainer.removeAllChildren();

    for (int i = 0; i < audioEngine.getNumTracks(); ++i)
    {
        auto* lane = trackLanes.add(
            new TrackLaneComponent(i, audioEngine, appState, *this));
        trackContainer.addAndMakeVisible(lane);
    }

    resized();
}

void TimelineComponent::resized()
{
    auto bounds = getLocalBounds();

    const int scrollBarThickness = 12;
    horizontalScrollBar.setBounds(bounds.removeFromBottom(scrollBarThickness)
                                        .withTrimmedRight(scrollBarThickness));
    verticalScrollBar.setBounds(bounds.removeFromRight(scrollBarThickness));

    auto ruler    = bounds.removeFromTop(RULER_HEIGHT);
    auto viewport = bounds;

    trackViewport.setBounds(viewport);

    // Layout track container
    int totalHeight = audioEngine.getNumTracks() * DEFAULT_TRACK_H;
    double totalSeconds = 300.0;
    int totalWidth = TRACK_HEADER_W + (int)(totalSeconds * pixelsPerSecond);
    trackContainer.setSize(totalWidth, totalHeight);

    for (int i = 0; i < trackLanes.size(); ++i)
        trackLanes[i]->setBounds(0, i * DEFAULT_TRACK_H, totalWidth, DEFAULT_TRACK_H);

    // Update scroll bars
    horizontalScrollBar.setRangeLimits(0.0, totalSeconds);
    horizontalScrollBar.setCurrentRange(horizontalScrollOffset,
        viewport.getWidth() / pixelsPerSecond);

    verticalScrollBar.setRangeLimits(0.0, totalHeight);
    verticalScrollBar.setCurrentRange(verticalScrollOffset, viewport.getHeight());
}

void TimelineComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    g.fillAll(juce::Colour(0xff1a1a2e));

    // Ruler
    paintRuler(g, bounds.removeFromTop(RULER_HEIGHT));
    paintPlayhead(g);
    if (loopEnabled)
        paintLoopRegion(g);
}

void TimelineComponent::paintRuler(juce::Graphics& g, juce::Rectangle<int> rulerBounds)
{
    g.setColour(juce::Colour(0xff16213e));
    g.fillRect(rulerBounds);

    g.setColour(juce::Colour(0xff0f3460));
    g.drawLine(0, (float)rulerBounds.getBottom(), (float)getWidth(), (float)rulerBounds.getBottom(), 1.0f);

    // Draw bar markers
    double bpm = audioEngine.getBpm();
    double secondsPerBeat = 60.0 / bpm;
    double secondsPerBar  = secondsPerBeat * audioEngine.getTimeSigNumerator();
    double startBar       = horizontalScrollOffset / secondsPerBar;
    int    firstBar       = (int)startBar;

    g.setFont(juce::Font(10.0f));

    for (int bar = firstBar; bar < firstBar + 200; ++bar)
    {
        double tSec = bar * secondsPerBar;
        double xPix = TRACK_HEADER_W + (tSec - horizontalScrollOffset) * pixelsPerSecond;

        if (xPix > getWidth()) break;
        if (xPix < TRACK_HEADER_W) continue;

        g.setColour(juce::Colour(0xff533483));
        g.drawVerticalLine((int)xPix, (float)rulerBounds.getY(),
                           (float)rulerBounds.getBottom());

        g.setColour(juce::Colours::lightgrey);
        g.drawText(juce::String(bar + 1), (int)xPix + 3, rulerBounds.getY() + 4,
                   40, 16, juce::Justification::left);

        // Beat subdivisions
        for (int beat = 1; beat < audioEngine.getTimeSigNumerator(); ++beat)
        {
            double beatPix = xPix + beat * secondsPerBeat * pixelsPerSecond;
            g.setColour(juce::Colour(0xff533483).withAlpha(0.4f));
            g.drawVerticalLine((int)beatPix,
                               (float)(rulerBounds.getBottom() - 8),
                               (float)rulerBounds.getBottom());
        }
    }
}

void TimelineComponent::paintPlayhead(juce::Graphics& g)
{
    double playPos = audioEngine.getPlayheadPosition();
    double xPix    = TRACK_HEADER_W + (playPos - horizontalScrollOffset) * pixelsPerSecond;

    if (xPix >= TRACK_HEADER_W && xPix <= getWidth())
    {
        g.setColour(juce::Colour(0xffe94560));
        g.drawVerticalLine((int)xPix, RULER_HEIGHT, getHeight());

        // Playhead triangle
        juce::Path triangle;
        triangle.addTriangle((float)xPix - 6, (float)RULER_HEIGHT,
                             (float)xPix + 6, (float)RULER_HEIGHT,
                             (float)xPix, (float)(RULER_HEIGHT + 12));
        g.fillPath(triangle);
    }
}

void TimelineComponent::paintLoopRegion(juce::Graphics& g)
{
    double x1 = TRACK_HEADER_W + (loopStart - horizontalScrollOffset) * pixelsPerSecond;
    double x2 = TRACK_HEADER_W + (loopEnd   - horizontalScrollOffset) * pixelsPerSecond;

    g.setColour(juce::Colour(0x22e94560));
    g.fillRect(juce::Rectangle<float>((float)x1, (float)RULER_HEIGHT,
                                      (float)(x2 - x1), (float)(getHeight() - RULER_HEIGHT)));
}

void TimelineComponent::timerCallback()
{
    if (audioEngine.isPlaying())
    {
        // Auto-scroll to follow playhead
        double playPos = audioEngine.getPlayheadPosition();
        double viewEnd = horizontalScrollOffset + (trackViewport.getWidth() / pixelsPerSecond);

        if (playPos > viewEnd - 1.0)
        {
            horizontalScrollOffset = playPos - 2.0;
            horizontalScrollOffset = juce::jmax(0.0, horizontalScrollOffset);
            trackViewport.setViewPosition(
                (int)((horizontalScrollOffset) * pixelsPerSecond), (int)verticalScrollOffset);
        }
    }
    repaint();
}

void TimelineComponent::mouseDown(const juce::MouseEvent& e)
{
    if (e.y < RULER_HEIGHT)
    {
        // Click on ruler sets playhead
        double t = pixelToTime(e.x);
        audioEngine.setPlayheadPosition(t);
    }
}

void TimelineComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (e.y < RULER_HEIGHT)
    {
        double t = pixelToTime(e.x);
        audioEngine.setPlayheadPosition(t);
    }
}

void TimelineComponent::mouseWheelMove(const juce::MouseEvent& e,
                                        const juce::MouseWheelDetails& wheel)
{
    if (e.mods.isCommandDown())
    {
        // Zoom
        double zoomFactor = 1.0 + wheel.deltaY * 0.3;
        setPixelsPerSecond(pixelsPerSecond * zoomFactor);
    }
    else if (e.mods.isShiftDown())
    {
        // Horizontal scroll
        horizontalScrollOffset -= wheel.deltaY * 2.0;
        horizontalScrollOffset = juce::jmax(0.0, horizontalScrollOffset);
        trackViewport.setViewPosition(
            (int)(horizontalScrollOffset * pixelsPerSecond), (int)verticalScrollOffset);
        repaint();
    }
    else
    {
        // Vertical scroll
        verticalScrollOffset -= wheel.deltaY * 30.0;
        verticalScrollOffset = juce::jmax(0.0, verticalScrollOffset);
        trackViewport.setViewPosition(
            (int)(horizontalScrollOffset * pixelsPerSecond), (int)verticalScrollOffset);
        repaint();
    }
}

void TimelineComponent::setPixelsPerSecond(double pps)
{
    pixelsPerSecond = juce::jlimit(10.0, 2000.0, pps);
    resized();
    repaint();
}

void TimelineComponent::scrollBarMoved(juce::ScrollBar* bar, double newRange)
{
    if (bar == &horizontalScrollBar)
    {
        horizontalScrollOffset = newRange;
        trackViewport.setViewPosition(
            (int)(horizontalScrollOffset * pixelsPerSecond), (int)verticalScrollOffset);
    }
    else
    {
        verticalScrollOffset = newRange;
        trackViewport.setViewPosition(
            (int)(horizontalScrollOffset * pixelsPerSecond), (int)verticalScrollOffset);
    }
    repaint();
}

double TimelineComponent::pixelToTime(double x) const
{
    return horizontalScrollOffset + (x - TRACK_HEADER_W) / pixelsPerSecond;
}

double TimelineComponent::timeToPixel(double t) const
{
    return TRACK_HEADER_W + (t - horizontalScrollOffset) * pixelsPerSecond;
}

void TimelineComponent::scrollToPosition(double posSeconds)
{
    horizontalScrollOffset = juce::jmax(0.0, posSeconds);
    trackViewport.setViewPosition(
        (int)(horizontalScrollOffset * pixelsPerSecond), (int)verticalScrollOffset);
    repaint();
}

bool TimelineComponent::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (auto& f : files)
    {
        auto ext = juce::File(f).getFileExtension().toLowerCase();
        if (ext == ".wav" || ext == ".aiff" || ext == ".mp3" || ext == ".flac" || ext == ".mid")
            return true;
    }
    return false;
}

void TimelineComponent::filesDropped(const juce::StringArray& files, int x, int y)
{
    // Calculate time from x position
    // x is relative to TimelineComponent
    double dropTime = pixelToTime(x);

    // Filter valid files
    for (auto& f : files)
    {
        juce::File file(f);
        auto ext = file.getFileExtension().toLowerCase();
        bool isMidi  = (ext == ".mid" || ext == ".midi");
        bool isAudio = (ext == ".wav" || ext == ".aiff" || ext == ".mp3" || ext == ".flac");

        if (!isMidi && !isAudio) continue;

        if (isMidi)
        {
            int trackIdx = audioEngine.addMidiTrack(file.getFileNameWithoutExtension());
            auto* clip = new MidiClip(dropTime, 4.0); // Default duration, should load from file if possible
            // TODO: Load MIDI content
            audioEngine.getTrackInfo(trackIdx).clips.add(clip);
        }
        else
        {
            int trackIdx = audioEngine.addAudioTrack(file.getFileNameWithoutExtension());
            auto* clip = new AudioClip(file, dropTime);
            clip->colour = audioEngine.getTrackInfo(trackIdx).colour;
            
            // Setup thumbnail (requires cache, but TimelineComponent doesn't own one?)
            // TrackLaneComponent owns cache.
            // But now AudioClip is created here.
            // We can't set thumbnail cache here easily unless TimelineComponent has one shared or we defer it.
            // AudioClip::setThumbnailCache updates immediately.
            // Ideally, AudioClip created in AudioEngine context shouldn't depend on UI cache?
            // But AudioClip is a UI-hybrid.
            // We can skip thumbnail setup here, and let TrackLaneComponent do it on rebuild?
            // No, TrackLaneComponent paints expecting thumbnail.
            // TrackLaneComponent::paintClips calls clip->paint.
            // AudioClip::paint uses thumbnail.
            // If thumbnail not set, likely empty.
            
            // Workaround: TrackLaneComponent constructor could set cache on all clips in track?
            // Yes, let's update TrackLaneComponent constructor to iterate clips and set cache.
            
            // Get duration
            if (auto* reader = audioEngine.getFormatManager().createReaderFor(file))
            {
                clip->duration = reader->lengthInSamples / reader->sampleRate;
                delete reader;
            }

            audioEngine.getTrackInfo(trackIdx).clips.add(clip);
        }
    }
}
