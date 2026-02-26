#include "TimelineComponent.h"
#include "TrackLaneComponent.h"

TimelineComponent::TimelineComponent(AudioEngine& e, AppState& s,
                                     juce::ApplicationCommandManager& c)
    : audioEngine(e), appState(s), commandManager(c)
{
    audioEngine.addListener(this);

    // Initialize Tools
    auto setupButton = [this](juce::TextButton& b, EditTool tool) {
        b.setRadioGroupId(1001);
        b.setToggleable(true);
        b.setClickingTogglesState(true);
        b.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffe94560));
        b.onClick = [this, tool] { setTool(tool); };
        addAndMakeVisible(b);
    };

    setupButton(selectButton, EditTool::Select);
    setupButton(splitButton,  EditTool::Split);
    setupButton(drawButton,   EditTool::Draw);
    setupButton(eraseButton,  EditTool::Erase);

    selectButton.setToggleState(true, juce::dontSendNotification);

    // Snap Combo
    snapComboBox.addItem("Bar",  1);
    snapComboBox.addItem("Beat", 2);
    snapComboBox.addItem("1/2",  3);
    snapComboBox.addItem("1/4",  4);
    snapComboBox.addItem("1/8",  5);
    snapComboBox.addItem("1/16", 6);
    snapComboBox.addItem("Off",  7);
    
    snapComboBox.setSelectedId(2, juce::dontSendNotification); // Beat default
    snapComboBox.onChange = [this] {
        switch (snapComboBox.getSelectedId())
        {
            case 1: currentSnapMode = SnapTo::Bar; break;
            case 2: currentSnapMode = SnapTo::Beat; break;
            case 3: currentSnapMode = SnapTo::Half; break;
            case 4: currentSnapMode = SnapTo::Quarter; break;
            case 5: currentSnapMode = SnapTo::Eighth; break;
            case 6: currentSnapMode = SnapTo::Sixteenth; break;
            case 7: currentSnapMode = SnapTo::Off; break;
        }
    };
    addAndMakeVisible(snapComboBox);

    // Navigation buttons
    jumpStartBtn.onClick = [this] {
        audioEngine.setPlayheadPosition(0.0);
        scrollToPosition(0.0);
    };
    addAndMakeVisible(jumpStartBtn);

    jumpEndBtn.onClick = [this] {
        // Find the end of the last clip
        double maxEnd = 0.0;
        for (int i = 0; i < audioEngine.getNumTracks(); ++i) {
            auto& info = audioEngine.getTrackInfo(i);
            for (auto* c : info.clips)
                maxEnd = juce::jmax(maxEnd, c->startTime + c->duration);
        }
        if (maxEnd < 1.0) maxEnd = 10.0;
        audioEngine.setPlayheadPosition(maxEnd);
        scrollToPosition(juce::jmax(0.0, maxEnd - 2.0));
    };
    addAndMakeVisible(jumpEndBtn);

    // Zoom buttons
    zoomInBtn.onClick  = [this] { zoomIn(); };
    zoomOutBtn.onClick = [this] { zoomOut(); };
    addAndMakeVisible(zoomInBtn);
    addAndMakeVisible(zoomOutBtn);

    // Zoom slider (log scale)
    zoomSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    zoomSlider.setRange(std::log2(5.0), std::log2(2000.0), 0.01);
    zoomSlider.setValue(std::log2(pixelsPerSecond), juce::dontSendNotification);
    zoomSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    zoomSlider.onValueChange = [this] {
        setPixelsPerSecond(std::pow(2.0, zoomSlider.getValue()));
    };
    addAndMakeVisible(zoomSlider);

    // Track height slider
    trackHeightSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    trackHeightSlider.setRange(40.0, 300.0, 1.0);
    trackHeightSlider.setValue(80.0, juce::dontSendNotification);
    trackHeightSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    trackHeightSlider.onValueChange = [this] {
        userTrackHeight = (int) trackHeightSlider.getValue();
        resized();
    };
    addAndMakeVisible(trackHeightSlider);

    addAndMakeVisible(horizontalScrollBar);
    addAndMakeVisible(verticalScrollBar);
    addAndMakeVisible(trackViewport);

    trackViewport.setViewedComponent(&trackContainer, false);
    trackViewport.setScrollBarsShown(false, false);

    horizontalScrollBar.addListener(this);
    verticalScrollBar.addListener(this);

    setWantsKeyboardFocus(true);
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
    
    // Tools row
    auto toolArea = ruler.removeFromLeft(trackHeaderWidth);
    int btnW = toolArea.getWidth() / 4;
    selectButton.setBounds(toolArea.removeFromLeft(btnW).reduced(2));
    splitButton.setBounds(toolArea.removeFromLeft(btnW).reduced(2));
    drawButton.setBounds(toolArea.removeFromLeft(btnW).reduced(2));
    eraseButton.setBounds(toolArea.removeFromLeft(btnW).reduced(2));

    // Controls row within ruler area
    snapComboBox.setBounds(ruler.removeFromLeft(80).reduced(2));
    jumpStartBtn.setBounds(ruler.removeFromLeft(28).reduced(2));
    jumpEndBtn.setBounds(ruler.removeFromLeft(28).reduced(2));
    zoomOutBtn.setBounds(ruler.removeFromLeft(24).reduced(2));
    zoomSlider.setBounds(ruler.removeFromLeft(100).reduced(2));
    zoomInBtn.setBounds(ruler.removeFromLeft(24).reduced(2));
    trackHeightSlider.setBounds(ruler.removeFromLeft(80).reduced(2));

    auto viewport = bounds;

    trackViewport.setBounds(viewport);

    double totalSeconds = 300.0;
    int totalWidth = trackHeaderWidth + (int) (totalSeconds * pixelsPerSecond);
    
    // Layout track container
    int currentY = 0;
    for (int i = 0; i < trackLanes.size(); ++i)
    {
        auto& info = audioEngine.getTrackInfo(i);
        if (!info.visible)
        {
            trackLanes[i]->setVisible(false);
            continue;
        }
        
        trackLanes[i]->setVisible(true);
        int h = userTrackHeight;
        if (info.showTakes)
            h += (int)info.takes.size() * TAKE_LANE_H;
        
        h += (int)info.visibleAutomationLanes.size() * AUTOMATION_LANE_H;
        
        trackLanes[i]->setBounds(0, currentY, totalWidth, h);
        currentY += h;
    }
    int totalHeight = currentY;

    trackContainer.setSize(totalWidth, totalHeight);

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
        double xPix = trackHeaderWidth + (tSec - horizontalScrollOffset) * pixelsPerSecond;

        if (xPix > getWidth()) break;
        if (xPix < trackHeaderWidth) continue;

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
    double xPix    = trackHeaderWidth + (playPos - horizontalScrollOffset) * pixelsPerSecond;

    if (xPix >= trackHeaderWidth && xPix <= getWidth())
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
    double x1 = trackHeaderWidth + (loopStart - horizontalScrollOffset) * pixelsPerSecond;
    double x2 = trackHeaderWidth + (loopEnd   - horizontalScrollOffset) * pixelsPerSecond;

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
    if (e.mods.isCommandDown() || e.mods.isCtrlDown()) // Ctrl for Windows/Linux
    {
        // Zoom towards mouse pointer
        if (e.x >= trackHeaderWidth)
        {
            double timeAtMouse = pixelToTime(e.x);
            double zoomFactor = std::pow(2.0, wheel.deltaY * 2.0); // Smooth zoom factor
            double newPPS = juce::jlimit(5.0, 5000.0, pixelsPerSecond * zoomFactor);
            
            // Recalculate offset so the time under the mouse stays the same
            double newOffset = timeAtMouse - (e.x - trackHeaderWidth) / newPPS;
            horizontalScrollOffset = juce::jmax(0.0, newOffset);
            
            pixelsPerSecond = newPPS;
            
            resized(); 
            trackViewport.setViewPosition((int)(horizontalScrollOffset * pixelsPerSecond), (int)verticalScrollOffset);
            horizontalScrollBar.setCurrentRange(horizontalScrollOffset, trackViewport.getWidth() / pixelsPerSecond);
            repaint();
        }
    }
    else
    {
        // Pan
        double dx = wheel.deltaX;
        double dy = wheel.deltaY;
        
        if (e.mods.isShiftDown())
        {
            dx += dy;
            dy = 0.0;
        }
        
        if (dx != 0.0)
            horizontalScrollOffset -= dx * 100.0 / pixelsPerSecond;
            
        if (dy != 0.0)
            verticalScrollOffset -= dy * 100.0;
            
        horizontalScrollOffset = juce::jmax(0.0, horizontalScrollOffset);
        verticalScrollOffset = juce::jmax(0.0, verticalScrollOffset);
        
        trackViewport.setViewPosition((int)(horizontalScrollOffset * pixelsPerSecond), (int)verticalScrollOffset);
        horizontalScrollBar.setCurrentRange(horizontalScrollOffset, trackViewport.getWidth() / pixelsPerSecond);
        verticalScrollBar.setCurrentRange(verticalScrollOffset, trackViewport.getHeight());
        repaint();
    }
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

// View Conversions (Relative to Window)
double TimelineComponent::pixelToTime(double x) const
{
    return (x - trackHeaderWidth) / pixelsPerSecond + horizontalScrollOffset;
}

double TimelineComponent::timeToPixel(double t) const
{
    return trackHeaderWidth + (t - horizontalScrollOffset) * pixelsPerSecond;
}

// Absolute Conversions (Relative to Track Start)
double TimelineComponent::absolutePixelToTime(double x) const
{
    // x is absolute pixel
    return (x - trackHeaderWidth) / pixelsPerSecond;
}

double TimelineComponent::timeToAbsolutePixel(double t) const
{
    return trackHeaderWidth + t * pixelsPerSecond;
}

void TimelineComponent::setPixelsPerSecond(double pps)
{
    double oldPPS = pixelsPerSecond;
    double centerTime = pixelToTime(getWidth() / 2.0); // Center of view

    pixelsPerSecond = juce::jlimit(10.0, 2000.0, pps);
    
    // Attempt to keep center of view ? 
    // New Offset = centerTime - (Width/2)/NewPPS ?
    // Or just let it be.
    
    resized(); // Recalculate container size
    repaint();
}

double TimelineComponent::snapToGrid(double time) const
{
    if (currentSnapMode == SnapTo::Off) return time;

    double bpm = audioEngine.getBpm();
    if (bpm <= 0.0) return time;

    double secondsPerBeat = 60.0 / bpm;
    // double timeSigNum = (double)audioEngine.getTimeSigNumerator();
    double timeSigDen = (double)audioEngine.getTimeSigDenominator();

    double gridInterval = 0.0;

    switch (currentSnapMode)
    {
        case SnapTo::Bar:       gridInterval = secondsPerBeat * (double)audioEngine.getTimeSigNumerator(); break;
        case SnapTo::Beat:      gridInterval = secondsPerBeat; break;
        case SnapTo::Half:      gridInterval = secondsPerBeat * (4.0 / timeSigDen) * 2.0; break; // Assumes X/4 sig roughly? Or just 2 beats? 1/2 note. 
                                // 1/2 note = 2 * (1/4 note). If den=4, beat=1/4. So 2*secondsPerBeat.
                                // If den=8, beat=1/8. 1/2 note = 4 beats. 
                                // Standard: beat value = 4/den. secondsPerWholenote = secondsPerBeat * den? No.
                                // simple way: 
                                // secondsPerQuarter = secondsPerBeat * (timeSigDen / 4.0);
                                // Interval = secondsPerQuarter * 4 * factor?
                                // Let's stick to musical time relative to quarter note = 1 beat at 4/4.
                                // But generic:
                                // gridInterval = secondsPerBeat * (4.0/timeSigDen) * ratio;
                                // 1/1 = 4.0
                                // 1/2 = 2.0
                                // 1/4 = 1.0 (Beat if x/4)
                                // 1/8 = 0.5
                                // 1/16 = 0.25
                                // So:
                                gridInterval = (60.0 / bpm) * (4.0 / timeSigDen) * 2.0; 
                                break;
        case SnapTo::Quarter:   gridInterval = (60.0 / bpm) * (4.0 / timeSigDen) * 1.0; break;
        case SnapTo::Eighth:    gridInterval = (60.0 / bpm) * (4.0 / timeSigDen) * 0.5; break;
        case SnapTo::Sixteenth: gridInterval = (60.0 / bpm) * (4.0 / timeSigDen) * 0.25; break;
        default: return time;
    }

    if (gridInterval <= 0.0001) return time;

    return std::round(time / gridInterval) * gridInterval;
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
    double dropTime = snapToGrid(pixelToTime(x));

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
            
            // Load audio data so the clip can actually play
            clip->loadAudioData(audioEngine.getFormatManager());

            audioEngine.getTrackInfo(trackIdx).clips.add(clip);
        }
    }
}

void TimelineComponent::clearAllSelections()
{
    for (int i = 0; i < audioEngine.getNumTracks(); ++i)
    {
        auto& info = audioEngine.getTrackInfo(i);
        for (auto* c : info.clips)
            c->selected = false;
    }
    repaint();
}

void TimelineComponent::copySelection()
{
    clipboard.clear();
    for (int i = 0; i < audioEngine.getNumTracks(); ++i)
    {
        auto& info = audioEngine.getTrackInfo(i);
        for (auto* c : info.clips)
        {
            if (c->selected)
            {
                ClipboardItem item;
                item.clip = c->clone();
                item.trackIndex = i;
                clipboard.push_back(std::move(item));
            }
        }
    }
}

void TimelineComponent::pasteSelection()
{
    if (clipboard.empty()) return;
    
    double minStartTime = std::numeric_limits<double>::max();
    for (const auto& item : clipboard)
    {
        if (item.clip->startTime < minStartTime)
            minStartTime = item.clip->startTime;
    }

    double pasteTime = snapToGrid(audioEngine.getPlayheadPosition());

    for (const auto& item : clipboard)
    {
        if (item.trackIndex >= 0 && item.trackIndex < audioEngine.getNumTracks())
        {
            double relativeStart = item.clip->startTime - minStartTime;
            std::unique_ptr<Clip> newClip = item.clip->clone();
            newClip->startTime = pasteTime + relativeStart;
            newClip->selected = true; 
            
            audioEngine.getTrackInfo(item.trackIndex).clips.add(newClip.release());
        }
    }
    repaint();
}

bool TimelineComponent::keyPressed(const juce::KeyPress& key)
{
    if (key.getModifiers().isCommandDown() || key.getModifiers().isCtrlDown())
    {
        if (key.getKeyCode() == 'c' || key.getKeyCode() == 'C')
        {
            copySelection();
            return true;
        }
        else if (key.getKeyCode() == 'v' || key.getKeyCode() == 'V')
        {
            clearAllSelections();
            pasteSelection();
            return true;
        }
    }
    else if (key.getKeyCode() == juce::KeyPress::deleteKey || key.getKeyCode() == juce::KeyPress::backspaceKey)
    {
        for (int i = 0; i < audioEngine.getNumTracks(); ++i)
        {
            auto& info = audioEngine.getTrackInfo(i);
            for (int c = info.clips.size(); --c >= 0;)
            {
                if (info.clips[c]->selected)
                    info.clips.remove(c);
            }
        }
        repaint();
        return true;
    }
    return false;
}
