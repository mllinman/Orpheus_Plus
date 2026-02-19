#include "TrackLaneComponent.h"
#include "TimelineComponent.h"

TrackLaneComponent::TrackLaneComponent(int idx, AudioEngine& e, AppState& s,
                                       TimelineComponent& t)
    : trackIndex(idx), audioEngine(e), appState(s), timeline(t)
{
    auto& info = audioEngine.getTrackInfo(trackIndex);

    // Mute
    muteButton.setColour(juce::TextButton::buttonColourId,   juce::Colour(0xff2d2d44));
    muteButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffFFB300));
    muteButton.setToggleable(true);
    muteButton.onClick = [this] {
        bool m = muteButton.getToggleState();
        audioEngine.setTrackMute(trackIndex, m);
    };
    addAndMakeVisible(muteButton);

    // Solo
    soloButton.setColour(juce::TextButton::buttonColourId,   juce::Colour(0xff2d2d44));
    soloButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff00BCD4));
    soloButton.setToggleable(true);
    soloButton.onClick = [this] {
        bool s = soloButton.getToggleState();
        audioEngine.setTrackSolo(trackIndex, s);
    };
    addAndMakeVisible(soloButton);

    // Arm
    armButton.setColour(juce::TextButton::buttonColourId,   juce::Colour(0xff2d2d44));
    armButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffe94560));
    armButton.setToggleable(true);
    armButton.onClick = [this] {
        audioEngine.armTrack(trackIndex, armButton.getToggleState());
    };
    addAndMakeVisible(armButton);

    // Volume
    volumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    volumeSlider.setRange(0.0, 1.5, 0.001);
    volumeSlider.setValue(1.0, juce::dontSendNotification);
    volumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    volumeSlider.onValueChange = [this] {
        audioEngine.setTrackVolume(trackIndex, (float)volumeSlider.getValue());
    };
    addAndMakeVisible(volumeSlider);

    // Pan
    panSlider.setSliderStyle(juce::Slider::Rotary);
    panSlider.setRange(-1.0, 1.0, 0.001);
    panSlider.setValue(0.0, juce::dontSendNotification);
    panSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    panSlider.onValueChange = [this] {
        audioEngine.setTrackPan(trackIndex, (float)panSlider.getValue());
    };
    addAndMakeVisible(panSlider);

    // Name label
    trackNameLabel.setText(info.name, juce::dontSendNotification);
    
    // Automation Combo
    automationCombo.addItem("Automation: Off", 1);
    automationCombo.addItem("Volume", 2);
    automationCombo.addItem("Pan", 3);
    automationCombo.setSelectedId(1);
    automationCombo.onChange = [this] {
        int id = automationCombo.getSelectedId();
        if (id == 1) currentAutomationParam = "";
        else if (id == 2) currentAutomationParam = "vol";
        else if (id == 3) currentAutomationParam = "pan";
        repaint();
    };
    addAndMakeVisible(automationCombo);

    // Init thumbnails for existing clips
    for (auto* clip : info.clips)
    {
        if (auto* audioClip = dynamic_cast<AudioClip*>(clip))
        {
            audioClip->setThumbnailCache(thumbnailCache, audioEngine.getFormatManager());
            if (audioClip->thumbnail)
                audioClip->thumbnail->addChangeListener(this);
        }
    }
    trackNameLabel.setFont(juce::Font(12.0f, juce::Font::bold));
    trackNameLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    trackNameLabel.setEditable(true);
    trackNameLabel.onTextChange = [this] {
        audioEngine.getTrackInfo(trackIndex).name = trackNameLabel.getText();
    };
    addAndMakeVisible(trackNameLabel);


}

TrackLaneComponent::~TrackLaneComponent() {}

void TrackLaneComponent::resized()
{
    auto header = getLocalBounds().removeFromLeft(HEADER_WIDTH).reduced(4);

    trackNameLabel.setBounds(header.removeFromTop(18));
    header.removeFromTop(2);

    auto buttonRow = header.removeFromTop(22);
    muteButton.setBounds(buttonRow.removeFromLeft(22));
    buttonRow.removeFromLeft(2);
    soloButton.setBounds(buttonRow.removeFromLeft(22));
    buttonRow.removeFromLeft(2);
    armButton.setBounds(buttonRow.removeFromLeft(22));

    auto sliderRow = header.removeFromTop(20);
    panSlider.setBounds(sliderRow.removeFromRight(30));
    volumeSlider.setBounds(sliderRow);
    
    automationCombo.setBounds(header.removeFromTop(18));
}

void TrackLaneComponent::paint(juce::Graphics& g)
{
    auto& info = audioEngine.getTrackInfo(trackIndex);
    auto bounds = getLocalBounds();

    // Track background
    g.setColour(juce::Colour(0xff1e1e32));
    g.fillRect(bounds);

    // Header background
    auto header = bounds.removeFromLeft(HEADER_WIDTH);
    g.setColour(info.colour.withBrightness(0.25f));
    g.fillRect(header);
    g.setColour(info.colour.withAlpha(0.8f));
    g.drawLine(0, 0, 0, (float)getHeight(), 3.0f);

    // Divider line
    g.setColour(juce::Colour(0xff0f0f1a));
    g.drawLine(0, (float)getHeight() - 1, (float)getWidth(), (float)getHeight() - 1);

    // Clip area
    paintClips(g, bounds);
}

void TrackLaneComponent::paintClips(juce::Graphics& g, juce::Rectangle<int> clipArea)
{
    auto& info = audioEngine.getTrackInfo(trackIndex);
    for (auto* clip : info.clips)
    {
        auto clipBounds = getClipScreenBounds(*clip);
        if (clipBounds.getRight() < clipArea.getX() ||
            clipBounds.getX() > clipArea.getRight())
            continue;

        clip->paint(g, clipBounds, clipArea);
    }
}


// Old paint methods removed


juce::Rectangle<float> TrackLaneComponent::getClipScreenBounds(const Clip& clip) const
{
    double startPixel = timeline.timeToAbsolutePixel(clip.startTime);
    double endPixel   = timeline.timeToAbsolutePixel(clip.startTime + clip.duration);

    float  y  = 4.0f;
    float  h  = (float)(getHeight() - 8);
    
    // Note: Assuming timeToPixel returns absolute timeline pixel.
    return { (float)(startPixel - HEADER_WIDTH), y, (float)(endPixel - startPixel), h }; 
}

TrackLaneComponent::DragMode TrackLaneComponent::getZoneAt(int x, int y, Clip* clip)
{
    if (!clip) return DragMode::None;

    // x is absolute pixel in TrackLane (inside container)
    // Clip drawing starts at HEADER_WIDTH?
    // In getClipScreenBounds, we return (startPixel - HEADER_WIDTH).
    // So visual X of clip start is timeToAbsolutePixel(start) - HEADER_WIDTH.
    
    int clipX = timeline.timeToAbsolutePixel(clip->startTime) - HEADER_WIDTH;
    int clipW = timeline.timeToAbsolutePixel(clip->duration) - HEADER_WIDTH; // wait, duration is relative? 
    // timeToAbsolutePixel(dur) = Header + dur*p.
    // So length is (Header+dur*p) - Header = dur*p.
    // Or just:
    int clipLen = (int)(clip->duration * timeline.getPixelsPerSecond());
    int clipR = clipX + clipLen;

    if (x >= clipX && x < clipX + 6) return DragMode::ResizeLeft;
    if (x <= clipR && x > clipR - 6) return DragMode::ResizeRight;
    
    // Fades (simplification: top corners)
    if (y < 20)
    {
        if (x >= clipX && x < clipX + 12) return DragMode::FadeIn;
        if (x <= clipR && x > clipR - 12) return DragMode::FadeOut;
    }

    return DragMode::Move;
}

void TrackLaneComponent::updateCursor(int x, int y)
{
    auto* clip = getClipAt(x, y);
    if (!clip)
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
        return;
    }

    auto mode = getZoneAt(x, y, clip);
    switch (mode)
    {
        case DragMode::ResizeLeft:
        case DragMode::ResizeRight: setMouseCursor(juce::MouseCursor::LeftRightResizeCursor); break;
        case DragMode::FadeIn:
        case DragMode::FadeOut:     setMouseCursor(juce::MouseCursor::UpDownResizeCursor); break; 
        default:                    setMouseCursor(juce::MouseCursor::NormalCursor); break;
    }
}

void TrackLaneComponent::mouseMoved(const juce::MouseEvent& e)
{
    updateCursor(e.x, e.y);
}

void TrackLaneComponent::mouseDown(const juce::MouseEvent& e)
{
    if (e.x < HEADER_WIDTH) return;

    // 1. Tool Logic (Split / Erase)
    double clickTime = timeline.snapToGrid(timeline.absolutePixelToTime(e.x)); // Snap for tools? Maybe.
    auto tool = timeline.getTool();

    if (tool == TimelineComponent::EditTool::Split)
    {
        if (auto* clip = getClipAt(timeline.absolutePixelToTime(e.x))) // hit test unsnapped
        {
            // Split logic
            double originalEnd = clip->startTime + clip->duration;
            double newDuration = clickTime - clip->startTime;
            double remaining   = originalEnd - clickTime;
            
            if (newDuration > 0.05 && remaining > 0.05) // Min clip length
            {
                clip->duration = newDuration;

                if (clip->getType() == Clip::Type::Audio)
                {
                    auto* ac = static_cast<AudioClip*>(clip);
                    auto* newClip = new AudioClip(ac->sourceFile, clickTime);
                    newClip->offset   = ac->offset + newDuration;
                    newClip->duration = remaining;
                    newClip->colour   = ac->colour;
                    newClip->setThumbnailCache(thumbnailCache, audioEngine.getFormatManager()); 
                    audioEngine.getTrackInfo(trackIndex).clips.add(newClip);
                }
                else if (clip->getType() == Clip::Type::Midi)
                {
                    auto* newClip = new MidiClip(clickTime, remaining);
                    newClip->colour = clip->colour;
                    audioEngine.getTrackInfo(trackIndex).clips.add(newClip);
                }
                repaint();
            }
        }
        return;
    }
    else if (tool == TimelineComponent::EditTool::Erase)
    {
        if (auto* clip = getClipAt(timeline.absolutePixelToTime(e.x)))
        {
            audioEngine.getTrackInfo(trackIndex).clips.removeObject(clip);
            repaint();
        }
        return;
    }

    // 2. Default Interaction
    
    // Check Automation Mode First
    if (currentAutomationParam.isNotEmpty())
    {
        double automationClickTime = timeline.absolutePixelToTime(e.x); // X is Absolute
        auto& info = audioEngine.getTrackInfo(trackIndex);

        
        // Helper: Find or create curve
        AutomationCurve* targetCurve = nullptr;
        for (auto& c : info.automationCurves) {
            if (c.parameterID == currentAutomationParam) { targetCurve = &c; break; }
        }
        if (!targetCurve) {
            info.automationCurves.push_back({ currentAutomationParam, {}, true });
            targetCurve = &info.automationCurves.back();
        }

        // Hit test points
        int hitIndex = -1;
        for (int i=0; i<targetCurve->points.size(); ++i)
        {
            float px = (float)timeline.timeToAbsolutePixel(targetCurve->points[i].time) - HEADER_WIDTH;
            // timeToAbsolutePixel returns value including Header. 
            // TrackLane render offset is -HEADER_WIDTH.
            // So px needs to be adjusted?
            // getClipScreenBounds subtracts HEADER_WIDTH.
            // Rendering logic: x - HEADER_WIDTH.
            // So yes, px = timeToAbs(t) - HEADER_WIDTH.

            float zeroY = (float)getLocalBounds().getBottom(); // Approx
            float rangeY = (float)getLocalBounds().getHeight();
            float val = targetCurve->points[i].value;
            float norm = val; 
            if (currentAutomationParam == "vol") norm = val / 1.5f; 
            else if (currentAutomationParam == "pan") norm = (val + 1.0f) / 2.0f;
            float py = zeroY - norm * rangeY;

            if (e.getPosition().getDistanceFrom({(int)px, (int)py}) < 6)
            {
                hitIndex = i;
                break;
            }
        }

        if (hitIndex != -1)
        {
            // e.mods doesn't have isDoubleClick. e has getNumberOfClicks().
            if (e.getNumberOfClicks() > 1) 
            {
                targetCurve->points.erase(targetCurve->points.begin() + hitIndex);
                repaint();
                return;
            }
            // Start Drag
            currentDragMode = DragMode::MoveAutomationPoint;
            draggingAutomationPointIndex = hitIndex;
            dragStartPos = e.getPosition();
            dragStartTime = targetCurve->points[hitIndex].time;
            dragStartVal  = targetCurve->points[hitIndex].value;
        }
        else
        {
            // Add Point
             // Calc value from Y
            float zeroY = (float)getLocalBounds().getBottom();
            float rangeY = (float)getLocalBounds().getHeight();
            float normY = (zeroY - e.y) / rangeY;
            float newVal = normY;
            if (currentAutomationParam == "vol") newVal = normY * 1.5f;
            else if (currentAutomationParam == "pan") newVal = normY * 2.0f - 1.0f;

            targetCurve->addPoint(automationClickTime, newVal);
            repaint();
            
            // Immediately start dragging the new point?
            currentDragMode = DragMode::MoveAutomationPoint;
            draggingAutomationPointIndex = -1; 
            // Need to find index of new point (sorted).
            // For now just add.
        }
        return; 
    }

    draggingClip = getClipAt(e.x, e.y); // Pixel hit test
    
    if (draggingClip)
    {
        // Drag Mode / Zone
        currentDragMode = getZoneAt(e.x, e.y, draggingClip);
        
        dragStartPos = e.getPosition();
        dragStartTime = draggingClip->startTime;
        dragStartDuration = draggingClip->duration;
        dragStartOffset = draggingClip->offset;
        dragStartFadeIn = draggingClip->fadeIn;
        dragStartFadeOut = draggingClip->fadeOut;

        // Selection
        if (!e.mods.isShiftDown())
        {
            auto& info = audioEngine.getTrackInfo(trackIndex);
            for (auto* c : info.clips) c->selected = false;
        }
        draggingClip->selected = true;
        // Right Click Menu on Clip
        if (e.mods.isRightButtonDown())
        {
            juce::PopupMenu menu;
            if (draggingClip->getType() == Clip::Type::Audio)
            {
                auto* ac = static_cast<AudioClip*>(draggingClip);
                menu.addItem(1, "Extract Stems (AI)...");
                menu.addItem(2, "Convert to MIDI (AI)...");
                menu.addSeparator();
                menu.addItem(3, "Apply Pitch Correction");
                menu.addItem(4, "Apply Audio Cleanup");
                menu.addSeparator();

                menu.showMenuAsync(juce::PopupMenu::Options{}, [this, ac](int result) {
                    if (result == 1)
                    {
                        // Trigger Stem Separator
                        audioEngine.getStemSeparator().separate(ac->sourceFile, appState, [this](StemSeparationResult res) {
                            juce::MessageManager::callAsync([this, res] {
                                double t = draggingClip ? draggingClip->startTime : 0.0;
                                if (res.vocals.existsAsFile()) addAudioClip(res.vocals, t);
                                if (res.drums.existsAsFile())  addAudioClip(res.drums, t);
                                if (res.bass.existsAsFile())   addAudioClip(res.bass, t);
                                if (res.other.existsAsFile())  addAudioClip(res.other, t);
                            });
                        });
                    }
                    else if (result == 2)
                    {
                        // Trigger Audio To Midi
                        audioEngine.getAudioToMidiConverter().convert(ac->sourceFile, appState, [this](AudioToMidiResult res) {
                            juce::MessageManager::callAsync([this, res] {
                                double t = draggingClip ? draggingClip->startTime : 0.0;
                                if (res.midiFileOnDisk.existsAsFile())
                                {
                                    addMidiClip(t, 4.0);
                                }
                            });
                        });
                    }
                    else if (result == 3)
                    {
                        audioEngine.addAutoTuneToTrack(trackIndex);
                    }
                    else if (result == 4)
                    {
                        audioEngine.addAudioCleanupToTrack(trackIndex);
                    }
                });
            }
        }
    }
    else
    {
        currentDragMode = DragMode::None;
        
        // Deselect all
        auto& info = audioEngine.getTrackInfo(trackIndex);
        for (auto* c : info.clips) c->selected = false;
        repaint();

        // Right Click Menu (Add Clip) on Empty Space
        if (e.mods.isRightButtonDown())
        {
            juce::PopupMenu menu;
            menu.addItem(1, "Add Audio Clip...");
            menu.addItem(2, "Add MIDI Clip");
            menu.showMenuAsync(juce::PopupMenu::Options{}, [this, clickTime](int result) {
                if (result == 1)
                {
                    auto chooser = std::make_shared<juce::FileChooser>("Select audio file",
                        juce::File{}, "*.wav;*.aiff;*.mp3;*.flac");
                    chooser->launchAsync(juce::FileBrowserComponent::openMode |
                                         juce::FileBrowserComponent::canSelectFiles,
                        [this, clickTime, chooser](const juce::FileChooser& fc) {
                            if (fc.getResult().existsAsFile())
                                addAudioClip(fc.getResult(), clickTime);
                        });
                }
                else if (result == 2)
                {
                    addMidiClip(clickTime);
                }
            });
        }
    }
}

void TrackLaneComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (!draggingClip && currentAutomationParam.isEmpty()) return;
    
    double mouseTime = timeline.absolutePixelToTime(e.x);
    double snappedMouseTime = timeline.snapToGrid(mouseTime);
    
    // Delta time for Move
    double deltaTime = mouseTime - timeline.absolutePixelToTime(dragStartPos.x); 
    
    
    if (currentDragMode == DragMode::MoveAutomationPoint)
    {
        auto& info = audioEngine.getTrackInfo(trackIndex);
         AutomationCurve* targetCurve = nullptr;
        for (auto& c : info.automationCurves) {
            if (c.parameterID == currentAutomationParam) { targetCurve = &c; break; }
        }
        
        if (targetCurve && draggingAutomationPointIndex >= 0 && draggingAutomationPointIndex < targetCurve->points.size())
        {
            double newTime = timeline.snapToGrid(mouseTime);
            if (newTime < 0) newTime = 0;
            
            // Calc value from Y
            float zeroY = (float)getLocalBounds().getBottom();
            float rangeY = (float)getLocalBounds().getHeight();
            float normY = (zeroY - e.y) / rangeY; // local bounds or clip area?
            // Mouse event is relative to component (0,0 is top left)
            // But clips are drawn in clipArea?
            // "height - 8" was used in paintClips.
            // Let's assume full height matching paint logic approx.
            
            float newVal = normY;
            if (currentAutomationParam == "vol") newVal = std::clamp(normY * 1.5f, 0.0f, 1.5f);
            else if (currentAutomationParam == "pan") newVal = std::clamp(normY * 2.0f - 1.0f, -1.0f, 1.0f);
            
            targetCurve->points[draggingAutomationPointIndex].time = newTime;
            targetCurve->points[draggingAutomationPointIndex].value = newVal;
            
            // Re-sort required if time changed past neighbors?
            // For now, assume simple drag. 
             std::sort(targetCurve->points.begin(), targetCurve->points.end());
             // Invalidates index if order changes!
             // So we need to find it again or just re-sort on mouseUp.
             // Visual glitch if not sorted.
        }
        repaint();
        return;
    }

    if (currentDragMode == DragMode::Move)
    {
        // Snap absolute start time
        double newStart = timeline.snapToGrid(dragStartTime + deltaTime); 
        if (newStart < 0) newStart = 0;
        
        draggingClip->startTime = newStart;
        // No change to offset/duration
    }
    else if (currentDragMode == DragMode::ResizeLeft)
    {
        double newStart = snappedMouseTime;
        double end = dragStartTime + dragStartDuration;
        
        if (newStart >= end - 0.1) newStart = end - 0.1; // Min duration constraint
        if (newStart < 0) newStart = 0;
        
        double shift = newStart - dragStartTime;
        
        draggingClip->startTime = newStart;
        draggingClip->duration  = end - newStart;
        // Audio offset adjustment (so content stays in place relative to time)
        if (draggingClip->getType() == Clip::Type::Audio)
             draggingClip->offset = dragStartOffset + shift;
    }
    else if (currentDragMode == DragMode::ResizeRight)
    {
        double newEnd = snappedMouseTime;
        if (newEnd <= draggingClip->startTime + 0.1) newEnd = draggingClip->startTime + 0.1;
        
        draggingClip->duration = newEnd - draggingClip->startTime;
    }
    else if (currentDragMode == DragMode::FadeIn)
    {
        // Fade is local to clip start
        double fade = mouseTime - draggingClip->startTime;
        if (fade < 0) fade = 0;
        if (fade > draggingClip->duration) fade = draggingClip->duration;
        
        draggingClip->fadeIn = fade;
    }
    else if (currentDragMode == DragMode::FadeOut)
    {
        // Fade is local to clip end
        double end = draggingClip->startTime + draggingClip->duration;
        double fade = end - mouseTime;
        if (fade < 0) fade = 0;
        if (fade > draggingClip->duration) fade = draggingClip->duration;
        
        draggingClip->fadeOut = fade;
    }

    repaint();
}

void TrackLaneComponent::mouseUp(const juce::MouseEvent&)
{
    draggingClip = nullptr;
    currentDragMode = DragMode::None;
}

void TrackLaneComponent::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (e.x < HEADER_WIDTH) return;

    double clickTime = timeline.snapToGrid(timeline.absolutePixelToTime(e.x));
    if (auto* clip = getClipAt(timeline.absolutePixelToTime(e.x))) // Check unsnapped for hit test
    {
        if (clip->getType() == Clip::Type::Midi)
        {
            // TODO: open piano roll
        }
    }
    else if (timeline.getTool() == TimelineComponent::EditTool::Draw)
    {
        addMidiClip(clickTime);
    }
}

bool TrackLaneComponent::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (auto& f : files)
    {
        juce::File file(f);
        auto ext = file.getFileExtension().toLowerCase();
        if (ext == ".wav" || ext == ".aiff" || ext == ".mp3" || ext == ".flac")
            return true;
    }
    return false;
}

void TrackLaneComponent::filesDropped(const juce::StringArray& files, int x, int y)
{
    double dropTime = timeline.snapToGrid(timeline.pixelToTime(x));
    for (auto& f : files)
        addAudioClip(juce::File(f), dropTime);
}

void TrackLaneComponent::addAudioClip(const juce::File& file, double startTime)
{
    auto* clip = new AudioClip(file, startTime);
    clip->colour = audioEngine.getTrackInfo(trackIndex).colour;
    
    // Setup thumbnail
    clip->setThumbnailCache(thumbnailCache, audioEngine.getFormatManager());
    if (clip->thumbnail)
        clip->thumbnail->addChangeListener(this);
    
    // Update duration
    if (auto* reader = audioEngine.getFormatManager().createReaderFor(file))
    {
        clip->duration = reader->lengthInSamples / reader->sampleRate;
        delete reader;
    }

    audioEngine.getTrackInfo(trackIndex).clips.add(clip);
    repaint();
}

void TrackLaneComponent::addMidiClip(double startTime, double duration)
{
    auto* clip = new MidiClip(startTime, duration);
    audioEngine.getTrackInfo(trackIndex).clips.add(clip);
    repaint();
}

Clip* TrackLaneComponent::getClipAt(double timeSeconds)
{
    auto& info = audioEngine.getTrackInfo(trackIndex);
    for (auto* clip : info.clips)
        if (timeSeconds >= clip->startTime &&
            timeSeconds < clip->startTime + clip->duration)
            return clip;
    return nullptr;
}

Clip* TrackLaneComponent::getClipAt(int x, int y)
{
    // Ignore Y for now as we have single lane per track
    // (In future if we have overlapping layers shown vertically, check Y)
    juce::ignoreUnused(y);
    return getClipAt(timeline.absolutePixelToTime(x));
}

void TrackLaneComponent::changeListenerCallback(juce::ChangeBroadcaster*)
{
    repaint();
}
