#include "TrackLaneComponent.h"
#include "TimelineComponent.h"
#include "../Audio/TransientDetector.h"
#include "../UI/OrpheusLookAndFeel.h"

TrackLaneComponent::TrackLaneComponent(int idx, AudioEngine& e, AppState& s,
                                       TimelineComponent& t)
    : trackIndex(idx), audioEngine(e), appState(s), timeline(t)
{
    auto& trackInfo = audioEngine.getTrackInfo(trackIndex);

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
        audioEngine.setTrackSolo(trackIndex, soloButton.getToggleState());
    };
    addAndMakeVisible(soloButton);

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
    trackNameLabel.setText(trackInfo.name, juce::dontSendNotification);
    
    // Automation Combo
    automationCombo.addItem("Show Automation...", 1);
    automationCombo.addItem("Volume", 2);
    automationCombo.addItem("Pan", 3);
    automationCombo.addItem("Sweetener", 4);
    automationCombo.setSelectedId(1, juce::dontSendNotification);
    
    automationCombo.onChange = [this] {
        int id = automationCombo.getSelectedId();
        juce::String paramID;
        if (id == 2) paramID = "vol";
        else if (id == 3) paramID = "pan";
        else if (id == 4) paramID = "sweet";

        if (paramID.isNotEmpty())
        {
            auto& info = audioEngine.getTrackInfo(trackIndex);
            
            // Toggle Visibility
            auto it = std::find(info.visibleAutomationLanes.begin(), info.visibleAutomationLanes.end(), paramID);
            if (it == info.visibleAutomationLanes.end())
                info.visibleAutomationLanes.push_back(paramID);
            else
                info.visibleAutomationLanes.erase(it);

            timeline.resized();
            repaint();
        }
        automationCombo.setSelectedId(1, juce::dontSendNotification);
    };
    addAndMakeVisible(automationCombo);

    // Init thumbnails for existing clips
    for (auto* clip : trackInfo.clips)
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

    // Folder Expand Button
    folderExpandButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2d2d44));
    folderExpandButton.onClick = [this] {
        bool expanded = !audioEngine.getAllTracks()[trackIndex]->expanded;
        appState.setTrackExpanded(trackIndex, expanded);
        folderExpandButton.setButtonText(expanded ? "-" : "+");
        timeline.resized();
    };
    addAndMakeVisible(folderExpandButton);
}

TrackLaneComponent::~TrackLaneComponent() {}

void TrackLaneComponent::resized()
{
    auto& trackInfo = audioEngine.getTrackInfo(trackIndex);
    auto header = getLocalBounds().removeFromLeft(timeline.getTrackHeaderWidth()).reduced(4);

    if (trackInfo.depth > 0)
        header.removeFromLeft(trackInfo.depth * 12);

    trackNameLabel.setBounds(header.removeFromTop(18));
    header.removeFromTop(2);

    if (trackInfo.type == OrpheusTrackInfo::Type::Arranger)
    {
        muteButton.setVisible(false);
        soloButton.setVisible(false);
        armButton.setVisible(false);
        showTakesButton.setVisible(false);
        volumeSlider.setVisible(false);
        panSlider.setVisible(false);
        automationCombo.setVisible(false);
        folderExpandButton.setVisible(false);
        return;
    }

    if (trackInfo.type == OrpheusTrackInfo::Type::Folder)
    {
        muteButton.setVisible(true);
        soloButton.setVisible(true);
        armButton.setVisible(false);
        showTakesButton.setVisible(false);
        volumeSlider.setVisible(true);
        panSlider.setVisible(true);
        automationCombo.setVisible(false);
        
        folderExpandButton.setVisible(true);
        folderExpandButton.setButtonText(trackInfo.expanded ? "-" : "+");
        
        auto buttonRow = header.removeFromTop(22);
        folderExpandButton.setBounds(buttonRow.removeFromLeft(22));
        buttonRow.removeFromLeft(2);
        muteButton.setBounds(buttonRow.removeFromLeft(22));
        buttonRow.removeFromLeft(2);
        soloButton.setBounds(buttonRow.removeFromLeft(22));

        auto sliderRow = header.removeFromTop(20);
        panSlider.setBounds(sliderRow.removeFromRight(30));
        volumeSlider.setBounds(sliderRow);
        return;
    }

    // Standard track (Audio/Midi)
    folderExpandButton.setVisible(false);
    muteButton.setVisible(true);
    soloButton.setVisible(true);
    armButton.setVisible(true);
    showTakesButton.setVisible(true);
    volumeSlider.setVisible(true);
    panSlider.setVisible(true);
    automationCombo.setVisible(true);

    auto buttonRow = header.removeFromTop(22);
    muteButton.setBounds(buttonRow.removeFromLeft(22));
    buttonRow.removeFromLeft(2);
    soloButton.setBounds(buttonRow.removeFromLeft(22));
    buttonRow.removeFromLeft(2);
    armButton.setBounds(buttonRow.removeFromLeft(22));
    buttonRow.removeFromLeft(2);
    showTakesButton.setBounds(buttonRow.removeFromLeft(22));

    auto sliderRow = header.removeFromTop(20);
    panSlider.setBounds(sliderRow.removeFromRight(30));
    volumeSlider.setBounds(sliderRow);
    
    automationCombo.setBounds(header.removeFromTop(18));
}

void TrackLaneComponent::paint(juce::Graphics& g)
{
    auto& trackInfo = audioEngine.getTrackInfo(trackIndex);
    auto bounds = getLocalBounds();
    int headerWidth = timeline.getTrackHeaderWidth();

    // Track background
    g.setColour(juce::Colour(0xff1e1e32));
    g.fillRect(bounds);

    // Header background
    auto header = bounds.removeFromLeft(headerWidth);
    g.setColour(trackInfo.colour.withBrightness(0.25f));
    g.fillRect(header);
    g.setColour(trackInfo.colour.withAlpha(0.8f));
    g.drawLine(0, 0, 0, (float)getHeight(), 3.0f);

    if (isDragHovering)
    {
        g.setColour(juce::Colours::white);
        if (hoverIsTopHalf)
            g.fillRect(0, 0, getWidth(), 2);
        else
            g.fillRect(0, getHeight() - 2, getWidth(), 2);
    }

    // Divider line
    g.setColour(juce::Colour(0xff0f0f1a));
    g.drawLine(0, (float)getHeight() - 1, (float)getWidth(), (float)getHeight() - 1);

    // Clip area
    auto clipArea = bounds;
    clipArea.setHeight(TimelineComponent::DEFAULT_TRACK_H);
    paintClips(g, clipArea, trackInfo.clips, false);

    if (trackInfo.showTakes)
    {
        for (int i = 0; i < trackInfo.takes.size(); ++i)
        {
            auto takeArea = bounds.withTop(TimelineComponent::DEFAULT_TRACK_H + i * TimelineComponent::TAKE_LANE_H)
                                  .withHeight(TimelineComponent::TAKE_LANE_H);
            
            // Draw take lane background
            g.setColour(juce::Colour(0xff22223a));
            g.fillRect(takeArea);
            g.setColour(juce::Colour(0xff0f0f1a));
            g.drawHorizontalLine(takeArea.getBottom() - 1, (float)takeArea.getX(), (float)takeArea.getRight());
            
            paintClips(g, takeArea, trackInfo.takes[i]->clips, true);
        }
    }

    // Automation Lanes
    int currentY = TimelineComponent::DEFAULT_TRACK_H;
    if (trackInfo.showTakes)
        currentY += (int)trackInfo.takes.size() * TimelineComponent::TAKE_LANE_H;

    for (const auto& paramID : trackInfo.visibleAutomationLanes)
    {
        auto laneArea = bounds.withTop(currentY).withHeight(TimelineComponent::AUTOMATION_LANE_H);
        paintAutomationLane(g, laneArea, paramID);
        currentY += TimelineComponent::AUTOMATION_LANE_H;
    }
}

void TrackLaneComponent::paintClips(juce::Graphics& g, juce::Rectangle<int> clipArea, juce::OwnedArray<Clip>& clipsToPaint, bool isTake)
{
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.drawRect(clipArea);

    for (auto* clip : clipsToPaint)
    {
        auto bounds = getClipScreenBounds(*clip);
        // Intersect with clipArea (which might be a sub-lane)
        bounds = bounds.withY((float)clipArea.getY() + 4).withHeight((float)clipArea.getHeight() - 8);

        clip->paint(g, bounds, clipArea);

        // Draw Fades Overlay
        if (clip->fadeIn > 0 || clip->fadeOut > 0)
        {
            float pps = (float)timeline.getPixelsPerSecond();
            float inW  = (float)clip->fadeIn * pps;
            float outW = (float)clip->fadeOut * pps;

            g.setColour(juce::Colours::white.withAlpha(0.2f));

            // Fade In Path
            if (inW > 0)
            {
                juce::Path p;
                p.startNewSubPath(bounds.getX(), bounds.getBottom());
                
                if (clip->fadeInCurve == Clip::FadeCurve::Linear)
                {
                    p.lineTo(bounds.getX() + inW, bounds.getY());
                }
                else if (clip->fadeInCurve == Clip::FadeCurve::Exponential)
                {
                    for (float i = 1; i <= 10; ++i) {
                        float t = i / 10.0f;
                        float val = std::pow(t, 2.0f); // Simple x^2 for expo
                        p.lineTo(bounds.getX() + inW * t, bounds.getBottom() - (bounds.getHeight() * val));
                    }
                }
                else if (clip->fadeInCurve == Clip::FadeCurve::S_Curve)
                {
                    for (float i = 1; i <= 10; ++i) {
                        float t = i / 10.0f;
                        float val = 0.5f - 0.5f * std::cos(t * juce::MathConstants<float>::pi);
                        p.lineTo(bounds.getX() + inW * t, bounds.getBottom() - (bounds.getHeight() * val));
                    }
                }
                
                p.lineTo(bounds.getX(), bounds.getY());
                p.closeSubPath();
                g.fillPath(p);
            }

            // Fade Out Path
            if (outW > 0)
            {
                juce::Path p;
                p.startNewSubPath(bounds.getRight(), bounds.getBottom());
                
                if (clip->fadeOutCurve == Clip::FadeCurve::Linear)
                {
                    p.lineTo(bounds.getRight() - outW, bounds.getY());
                }
                else if (clip->fadeOutCurve == Clip::FadeCurve::Exponential)
                {
                    for (float i = 1; i <= 10; ++i) {
                        float t = i / 10.0f;
                        float val = std::pow(t, 2.0f);
                        p.lineTo(bounds.getRight() - outW * t, bounds.getBottom() - (bounds.getHeight() * val));
                    }
                }
                else if (clip->fadeOutCurve == Clip::FadeCurve::S_Curve)
                {
                    for (float i = 1; i <= 10; ++i) {
                        float t = i / 10.0f;
                        float val = 0.5f - 0.5f * std::cos(t * juce::MathConstants<float>::pi);
                        p.lineTo(bounds.getRight() - outW * t, bounds.getBottom() - (bounds.getHeight() * val));
                    }
                }

                p.lineTo(bounds.getRight(), bounds.getY());
                p.closeSubPath();
                g.fillPath(p);
            }
        }

        // Draw Selection Halo
        if (clip->selected)
        {
            g.setColour(OrpheusLookAndFeel::accentPrimary().withAlpha(0.6f));
            g.drawRect(bounds.expanded(1.0f), 2.0f);
        }

        // Draw Fade Handles (only if hovered or selected)
        if (clip->selected || clip == selectedClip)
        {
            float handleSize = 10.0f;
            g.setColour(juce::Colours::white);

            // Left Handle (Fade In)
            float inX = bounds.getX() + (float)clip->fadeIn * (float)timeline.getPixelsPerSecond();
            g.fillEllipse(inX - handleSize*0.5f, bounds.getY(), handleSize, handleSize);

            // Right Handle (Fade Out)
            float outX = bounds.getRight() - (float)clip->fadeOut * (float)timeline.getPixelsPerSecond();
            g.fillEllipse(outX - handleSize*0.5f, bounds.getY(), handleSize, handleSize);
        }

        if (isTake)
        {
            // Dim takes slightly
            g.setColour(juce::Colours::black.withAlpha(0.3f));
            g.fillRect(bounds);
        }
    }
}


void TrackLaneComponent::paintAutomationLane(juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& paramID)
{
    g.setColour(juce::Colour(0xff22223a));
    g.fillRect(bounds);
    
    g.setColour(juce::Colour(0xff0f0f1a));
    g.drawHorizontalLine(bounds.getBottom() - 1, (float)bounds.getX(), (float)bounds.getRight());

    // Parameter Name
    g.setColour(juce::Colours::grey);
    g.setFont(10.0f);
    g.drawText(paramID.toUpperCase(), bounds.getX() + 5, bounds.getY() + 2, 60, 15, juce::Justification::left);

    auto& trackInfo = audioEngine.getTrackInfo(trackIndex);
    AutomationCurve* targetCurve = nullptr;
    for (auto& c : trackInfo.automationCurves) {
        if (c.parameterID == paramID) { targetCurve = &c; break; }
    }

    if (!targetCurve || targetCurve->points.empty()) return;

    g.setColour(juce::Colour(0xff00BCD4));
    juce::Path path;
    bool first = true;

    float zeroY = (float)bounds.getBottom();
    float rangeY = (float)bounds.getHeight();

    for (const auto& pt : targetCurve->points)
    {
        float px = (float)timeline.timeToAbsolutePixel(pt.time) - timeline.getTrackHeaderWidth();
        float norm = pt.value;
        if (paramID == "vol") norm = pt.value / 1.5f;
        else if (paramID == "pan") norm = (pt.value + 1.0f) / 2.0f;

        float py = zeroY - norm * rangeY;

        if (first) {
            path.startNewSubPath(px, py);
            first = false;
        } else {
            path.lineTo(px, py);
        }
    }

    g.strokePath(path, juce::PathStrokeType(1.5f));

    // Draw points
    for (const auto& pt : targetCurve->points)
    {
        float px = (float)timeline.timeToAbsolutePixel(pt.time) - timeline.getTrackHeaderWidth();
        float norm = pt.value;
        if (paramID == "vol") norm = pt.value / 1.5f;
        else if (paramID == "pan") norm = (pt.value + 1.0f) / 2.0f;
        float py = zeroY - norm * rangeY;

        g.setColour(juce::Colours::white);
        g.fillEllipse(px - 3, py - 3, 6, 6);
    }
}


juce::Rectangle<float> TrackLaneComponent::getClipScreenBounds(const Clip& clip) const
{
    double startPixel = timeline.timeToAbsolutePixel(clip.startTime);
    double endPixel   = timeline.timeToAbsolutePixel(clip.startTime + clip.duration);

    float  y  = 4.0f;
    float  h  = (float)(getHeight() - 8);
    
    // Note: Assuming timeToPixel returns absolute timeline pixel.
    return { (float)(startPixel - timeline.getTrackHeaderWidth()), y, (float)(endPixel - startPixel), h }; 
}

TrackLaneComponent::DragMode TrackLaneComponent::getZoneAt(int x, int y, Clip* clip)
{
    if (!clip) return DragMode::None;

    int clipX = timeline.timeToAbsolutePixel(clip->startTime) - timeline.getTrackHeaderWidth();
    int clipW = (int)(clip->duration * timeline.getPixelsPerSecond());
    int clipR = clipX + clipW;

    // Fade Handles (Circles at the top)
    float inHandleX  = (float)clipX + (float)clip->fadeIn * (float)timeline.getPixelsPerSecond();
    float outHandleX = (float)clipR - (float)clip->fadeOut * (float)timeline.getPixelsPerSecond();
    
    float handleSize = 12.0f;
    float handleY = 4.0f; // matches bounds.getY() in paint

    if (juce::Rectangle<float>(inHandleX - handleSize, handleY, handleSize * 2, handleSize * 2).contains((float)x, (float)y))
        return DragMode::FadeIn;
        
    if (juce::Rectangle<float>(outHandleX - handleSize, handleY, handleSize * 2, handleSize * 2).contains((float)x, (float)y))
        return DragMode::FadeOut;

    // Edges (Resize)
    if (x >= clipX && x < clipX + 8) return DragMode::ResizeLeft;
    if (x <= clipR && x > clipR - 8) return DragMode::ResizeRight;
    
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
        case DragMode::FadeOut:     setMouseCursor(juce::MouseCursor::PointingHandCursor); break; 
        default:                    setMouseCursor(juce::MouseCursor::NormalCursor); break;
    }
}

void TrackLaneComponent::mouseMoved(const juce::MouseEvent& e)
{
    updateCursor(e.x, e.y);
}

void TrackLaneComponent::mouseDown(const juce::MouseEvent& e)
{
    int headerWidth = timeline.getTrackHeaderWidth();

    if (e.mods.isRightButtonDown())
    {
        if (auto* clip = getClipAt(e.x, e.y))
        {
            auto zone = getZoneAt(e.x, e.y, clip);
            if (zone == DragMode::FadeIn || zone == DragMode::FadeOut)
            {
                bool isIn = (zone == DragMode::FadeIn);
                juce::PopupMenu m;
                m.addSectionHeader(isIn ? "Fade In Curve" : "Fade Out Curve");
                m.addItem(1, "Linear", true, (isIn ? clip->fadeInCurve : clip->fadeOutCurve) == Clip::FadeCurve::Linear);
                m.addItem(2, "Exponential", true, (isIn ? clip->fadeInCurve : clip->fadeOutCurve) == Clip::FadeCurve::Exponential);
                m.addItem(3, "S-Curve", true, (isIn ? clip->fadeInCurve : clip->fadeOutCurve) == Clip::FadeCurve::S_Curve);
                
                m.showMenuAsync(juce::PopupMenu::Options{}, [this, clip, isIn](int res) {
                    if (res == 0) return;
                    auto curve = (res == 1) ? Clip::FadeCurve::Linear : 
                                 (res == 2) ? Clip::FadeCurve::Exponential : Clip::FadeCurve::S_Curve;
                    if (isIn) clip->fadeInCurve = curve;
                    else clip->fadeOutCurve = curve;
                    repaint();
                });
                return;
            }
        }

        // Global Track Right Click
        juce::PopupMenu m;
        m.addItem(1, "Change track options...");
        m.addItem(2, "Reset track to empty");
        m.addItem(3, "Reset playhead to beginning");
        m.addSeparator();
        m.addItem(4, "Delete track");

        m.showMenuAsync(juce::PopupMenu::Options{}, [this](int result)
        {
            if (result == 1)
            {
                auto* alert = new juce::AlertWindow("Track Options", "Change track name:", juce::MessageBoxIconType::NoIcon);
                alert->addTextEditor("name", audioEngine.getTrackInfo(trackIndex).name, "Track Name:");
                alert->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
                alert->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

                alert->enterModalState(true, juce::ModalCallbackFunction::create([this, alert](int res) {
                    if (res == 1)
                    {
                        auto newName = alert->getTextEditorContents("name");
                        if (newName.isNotEmpty())
                        {
                            appState.renameTrack(trackIndex, newName);
                            audioEngine.getTrackInfo(trackIndex).name = newName;
                            timeline.repaint();
                        }
                    }
                    delete alert;
                }));
            }
            else if (result == 2)
            {
                auto& engineTrack = audioEngine.getTrackInfo(trackIndex);
                engineTrack.clips.clear();
                if (engineTrack.type == OrpheusTrackInfo::Type::Midi)
                    engineTrack.takes.clear();
                timeline.repaint();
            }
            else if (result == 3)
            {
                auto& engineTrack = audioEngine.getTrackInfo(trackIndex);
                double earliest = 0.0;
                if (!engineTrack.clips.isEmpty())
                {
                    earliest = engineTrack.clips.getFirst()->startTime;
                    for (auto* c : engineTrack.clips) earliest = juce::jmin(earliest, c->startTime);
                }
                audioEngine.setPlayheadPosition(earliest);
                timeline.repaint();
            }
            else if (result == 4)
            {
                appState.removeTrack(trackIndex);
                audioEngine.removeTrack(trackIndex);
                timeline.rebuildTracks();
            }
        });
        return;
    }

    if (e.x < headerWidth) return;

    auto& trackInfo = audioEngine.getTrackInfo(trackIndex);

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
    
    // Y-Coord based lane detection
    int clipBottom = TimelineComponent::DEFAULT_TRACK_H;
    int currentY = clipBottom;
    
    // 2a. Take Lanes
    if (trackInfo.showTakes)
    {
        int takesBottom = currentY + (int)trackInfo.takes.size() * TimelineComponent::TAKE_LANE_H;
        if (e.y >= currentY && e.y < takesBottom)
        {
            int takeIdx = (e.y - currentY) / TimelineComponent::TAKE_LANE_H;
            if (takeIdx >= 0 && takeIdx < trackInfo.takes.size())
            {
                currentDragMode = DragMode::SwipeComp;
                draggingTakeIndex = takeIdx;
                dragStartPos = e.getPosition();
                return;
            }
        }
        currentY = takesBottom;
    }

    // 2b. Automation Lanes
    for (int i = 0; i < (int)trackInfo.visibleAutomationLanes.size(); ++i)
    {
        int laneTop = currentY;
        int laneBottom = currentY + TimelineComponent::AUTOMATION_LANE_H;
        
        if (e.y >= laneTop && e.y < laneBottom)
        {
            currentAutomationParam = trackInfo.visibleAutomationLanes[i];
            double automationClickTime = timeline.absolutePixelToTime(e.x);
            
            AutomationCurve* targetCurve = nullptr;
            for (auto& c : trackInfo.automationCurves) {
                if (c.parameterID == currentAutomationParam) { targetCurve = &c; break; }
            }
            if (!targetCurve) {
                trackInfo.automationCurves.push_back({ currentAutomationParam, {}, true });
                targetCurve = &trackInfo.automationCurves.back();
            }

            int hitIndex = -1;
            for (int j = 0; j < (int)targetCurve->points.size(); ++j)
            {
                float px = (float)timeline.timeToAbsolutePixel(targetCurve->points[j].time) - timeline.getTrackHeaderWidth();
                float norm = targetCurve->points[j].value;
                if (currentAutomationParam == "vol") norm = targetCurve->points[j].value / 1.5f;
                else if (currentAutomationParam == "pan") norm = (targetCurve->points[j].value + 1.0f) / 2.0f;
                
                float py = (float)laneBottom - norm * TimelineComponent::AUTOMATION_LANE_H;

                if (e.getPosition().getDistanceFrom({(int)px, (int)py}) < 8) {
                    hitIndex = j;
                    break;
                }
            }

            if (hitIndex != -1) {
                if (e.getNumberOfClicks() > 1) {
                    targetCurve->points.erase(targetCurve->points.begin() + hitIndex);
                } else {
                    currentDragMode = DragMode::MoveAutomationPoint;
                    draggingAutomationPointIndex = hitIndex;
                    dragStartPos = e.getPosition();
                    dragStartTime = targetCurve->points[hitIndex].time;
                    dragStartVal = targetCurve->points[hitIndex].value;
                }
            } else {
                float normY = ((float)laneBottom - (float)e.y) / (float)TimelineComponent::AUTOMATION_LANE_H;
                float newVal = normY;
                if (currentAutomationParam == "vol") newVal = normY * 1.5f;
                else if (currentAutomationParam == "pan") newVal = normY * 2.0f - 1.0f;
                targetCurve->addPoint(automationClickTime, newVal);
            }
            repaint();
            return;
        }
        currentY = laneBottom;
    }

    // 2c. Main Clip Lane (Default Fallback)
    if (e.y < clipBottom)
    {
        draggingClip = getClipAt(e.x, e.y);
        if (draggingClip)
        {
            currentDragMode = getZoneAt(e.x, e.y, draggingClip);
            draggingTakeIndex = -1;
            dragStartPos = e.getPosition();
            dragStartTime = draggingClip->startTime;
            dragStartDuration = draggingClip->duration;
            dragStartOffset = draggingClip->offset;
            dragStartFadeIn = draggingClip->fadeIn;
            dragStartFadeOut = draggingClip->fadeOut;
            dragStartSourceBpm = 120.0;
            if (auto* ac = dynamic_cast<AudioClip*>(draggingClip))
                dragStartSourceBpm = ac->sourceBpm;

            if (!e.mods.isShiftDown())
                timeline.clearAllSelections();
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
                    menu.addItem(5, "Detect Transients & Slice");
                    menu.addItem(6, "Extract Groove to Quantize Template");

                    menu.showMenuAsync(juce::PopupMenu::Options{}, [this, ac](int result) {
                        if (result == 1)
                        {
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
                            audioEngine.getAudioToMidiConverter().convert(ac->sourceFile, appState, [this](AudioToMidiResult res) {
                                juce::MessageManager::callAsync([this, res] {
                                    double t = draggingClip ? draggingClip->startTime : 0.0;
                                    if (res.midiFileOnDisk.existsAsFile())
                                        addMidiClip(t, 4.0);
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
                        else if (result == 5)
                        {
                            if (ac->isLoaded && ac->sampleRate > 0)
                            {
                                auto transients = TransientDetector::detectTransients(ac->audioData, ac->sampleRate);
                                if (!transients.empty())
                                {
                                    auto& trackClips = audioEngine.getTrackInfo(trackIndex).clips;
                                    double currentStart = ac->startTime;
                                    double currentOffset = ac->offset;
                                    double endTime = ac->startTime + ac->duration;
                                    
                                    juce::String msg = "Detected " + juce::String((int)transients.size()) + " transients!";
                                    DBG(msg);
                                    
                                    std::vector<Clip*> newClips;
                                    
                                    for (double t : transients)
                                    {
                                        if (t > currentOffset + 0.05 && t < ac->offset + ac->duration - 0.05)
                                        {
                                            double splitTime = ac->startTime + (t - ac->offset);
                                            auto* chunk = new AudioClip(ac->sourceFile, currentStart);
                                            chunk->offset = currentOffset;
                                            chunk->duration = splitTime - currentStart;
                                            chunk->colour = ac->colour;
                                            chunk->isLoaded = ac->isLoaded;
                                            chunk->sampleRate = ac->sampleRate;
                                            chunk->audioData = ac->audioData;
                                            chunk->setThumbnailCache(thumbnailCache, audioEngine.getFormatManager());
                                            newClips.push_back(chunk);
                                            
                                            currentStart = splitTime;
                                            currentOffset = t;
                                        }
                                    }
                                    
                                    // Add the final chunk
                                    if (currentStart < endTime)
                                    {
                                        auto* finalChunk = new AudioClip(ac->sourceFile, currentStart);
                                        finalChunk->offset = currentOffset;
                                        finalChunk->duration = endTime - currentStart;
                                        finalChunk->colour = ac->colour;
                                        finalChunk->isLoaded = ac->isLoaded;
                                        finalChunk->sampleRate = ac->sampleRate;
                                        finalChunk->audioData = ac->audioData;
                                        finalChunk->setThumbnailCache(thumbnailCache, audioEngine.getFormatManager());
                                        newClips.push_back(finalChunk);
                                    }
                                    
                                    // Remove old clip and add new ones if we actually sliced
                                    if (newClips.size() > 1)
                                    {
                                        trackClips.removeObject(ac); // Also deletes the old clip!
                                        for (auto* c : newClips) trackClips.add(c);
                                        repaint();
                                    }
                                    else
                                    {
                                        for (auto* c : newClips) delete c;
                                    }
                                }
                            }
                        }
                        else if (result == 6)
                        {
                            if (ac->isLoaded && ac->sampleRate > 0)
                            {
                                auto transients = TransientDetector::detectTransients(ac->audioData, ac->sampleRate);
                                if (!transients.empty())
                                {
                                    double bpm = audioEngine.getBpm();
                                    double bps = bpm / 60.0;
                                    double secPerBeat = 1.0 / bps;
                                    
                                    appState.userGrooveTemplate.clear();
                                    for (double t : transients)
                                    {
                                        double absoluteTime = ac->startTime + (t - ac->offset);
                                        double beatVal = absoluteTime / secPerBeat;
                                        double frac = beatVal - std::floor(beatVal);
                                        appState.userGrooveTemplate.push_back(frac);
                                    }
                                    
                                    juce::String msg = "Extracted groove template from " + juce::String((int)appState.userGrooveTemplate.size()) + " transients!";
                                    DBG(msg);
                                }
                            }
                        }
                    });
                }
                else if (draggingClip->getType() == Clip::Type::Midi)
                {
                    auto* mc = static_cast<MidiClip*>(draggingClip);
                    menu.addItem(1, "Quantize to 1/16th Grid");
                    if (!appState.userGrooveTemplate.empty())
                    {
                        menu.addItem(2, "Quantize to Extracted Groove Template");
                    }
                    else
                    {
                        menu.addItem(2, "Quantize to Extracted Groove Template (None Extracted)", false, false);
                    }
                    
                    menu.showMenuAsync(juce::PopupMenu::Options{}, [this, mc](int result) {
                        if (result == 1 || result == 2)
                        {
                            double bpm = audioEngine.getBpm();
                            double bps = bpm / 60.0;
                            double secPerBeat = 1.0 / bps;
                            
                            juce::MidiMessageSequence newSeq;
                            for (int i = 0; i < mc->midiData.getNumEvents(); ++i)
                            {
                                auto* evt = mc->midiData.getEventPointer(i);
                                auto m = evt->message;
                                
                                if (m.isNoteOn() || m.isNoteOff())
                                {
                                    double evAbsoluteTime = mc->startTime + (m.getTimeStamp() / mc->ppq) * secPerBeat;
                                    double beatVal = evAbsoluteTime / secPerBeat;
                                    double currentFrac = beatVal - std::floor(beatVal);
                                    
                                    double targetFrac = 0.0;
                                    
                                    if (result == 1) // 1/16 grid
                                    {
                                        targetFrac = std::round(currentFrac * 4.0) / 4.0;
                                    }
                                    else if (result == 2) // Groove
                                    {
                                        double bestDiff = 999.0;
                                        for (double gf : appState.userGrooveTemplate)
                                        {
                                            double diff = std::abs(currentFrac - gf);
                                            // wrap around logic for fractions near 0 and 1
                                            if (diff > 0.5) diff = 1.0 - diff;
                                            
                                            if (diff < bestDiff)
                                            {
                                                bestDiff = diff;
                                                targetFrac = gf;
                                                
                                                // Handle edge wrap
                                                if (currentFrac > 0.5 && gf < 0.5) targetFrac += 1.0;
                                                if (currentFrac < 0.5 && gf > 0.5) targetFrac -= 1.0;
                                            }
                                        }
                                    }
                                    
                                    double newAbsoluteTime = (std::floor(beatVal) + targetFrac) * secPerBeat;
                                    double newRelativeTime = ((newAbsoluteTime - mc->startTime) / secPerBeat) * mc->ppq;
                                    m.setTimeStamp(newRelativeTime);
                                }
                                newSeq.addEvent(m);
                            }
                            
                            newSeq.updateMatchedPairs();
                            mc->midiData = newSeq;
                            repaint();
                        }
                    });
                }
            }
        }
        else
        {
            currentDragMode = DragMode::None;
            for (auto* c : trackInfo.clips) c->selected = false;
            repaint();

            // Right Click Menu on Empty Space
            if (e.mods.isRightButtonDown())
            {
                double menuClickTime = timeline.absolutePixelToTime(e.x);
                juce::PopupMenu menu;
                menu.addItem(1, "Add Audio Clip...");
                menu.addItem(2, "Add MIDI Clip");
                menu.showMenuAsync(juce::PopupMenu::Options{}, [this, menuClickTime](int result) {
                    if (result == 1)
                    {
                        auto chooser = std::make_shared<juce::FileChooser>("Select audio file",
                            juce::File{}, "*.wav;*.aiff;*.mp3;*.flac");
                        chooser->launchAsync(juce::FileBrowserComponent::openMode |
                                             juce::FileBrowserComponent::canSelectFiles,
                            [this, menuClickTime, chooser](const juce::FileChooser& fc) {
                                if (fc.getResult().existsAsFile())
                                    addAudioClip(fc.getResult(), menuClickTime);
                            });
                    }
                    else if (result == 2)
                        addMidiClip(menuClickTime);
                });
            }
        }
    }
}

void TrackLaneComponent::mouseDrag(const juce::MouseEvent& e)
{
    int headerWidth = timeline.getTrackHeaderWidth();

    if (e.x < headerWidth && !e.mods.isRightButtonDown())
    {
        if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor(this))
        {
            if (!container->isDragAndDropActive())
            {
                juce::String dragDescription = "TrackLane:" + juce::String(trackIndex);
                auto dragImage = createComponentSnapshot(getLocalBounds().withWidth(headerWidth));
                container->startDragging(dragDescription, this, dragImage);
            }
        }
        return;
    }

    if (!draggingClip && currentAutomationParam.isEmpty() && draggingTakeIndex == -1) return;

    auto& trackInfo = audioEngine.getTrackInfo(trackIndex);
    
    double mouseTime = timeline.absolutePixelToTime(e.x);
    double snappedMouseTime = timeline.snapToGrid(mouseTime);
    
    // Delta time for Move
    double deltaTime = mouseTime - timeline.absolutePixelToTime(dragStartPos.x); 
    
    
    if (currentDragMode == DragMode::MoveAutomationPoint)
    {
         AutomationCurve* targetCurve = nullptr;
        for (auto& c : trackInfo.automationCurves) {
            if (c.parameterID == currentAutomationParam) { targetCurve = &c; break; }
        }
        
        if (targetCurve && draggingAutomationPointIndex >= 0 && draggingAutomationPointIndex < (int)targetCurve->points.size())
        {
            double newTime = timeline.snapToGrid(mouseTime);
            if (newTime < 0) newTime = 0;
            
            // Find lane relative Y
            int laneIdx = -1;
            for (int i=0; i<(int)trackInfo.visibleAutomationLanes.size(); ++i) {
                if (trackInfo.visibleAutomationLanes[i] == currentAutomationParam) { laneIdx = i; break; }
            }

            float zeroY = (float)getHeight();
            float rangeY = (float)getHeight();

            if (laneIdx != -1)
            {
                int currentY = TimelineComponent::DEFAULT_TRACK_H;
                if (trackInfo.showTakes) currentY += (int)trackInfo.takes.size() * TimelineComponent::TAKE_LANE_H;
                currentY += laneIdx * TimelineComponent::AUTOMATION_LANE_H;
                
                int laneBottom = currentY + TimelineComponent::AUTOMATION_LANE_H;
                zeroY = (float)laneBottom;
                rangeY = (float)TimelineComponent::AUTOMATION_LANE_H;
            }
            
            float normY = (zeroY - (float)e.y) / rangeY;
            float newVal = normY;
            if (currentAutomationParam == "vol") newVal = std::clamp(normY * 1.5f, 0.0f, 1.5f);
            else if (currentAutomationParam == "pan") newVal = std::clamp(normY * 2.0f - 1.0f, -1.0f, 1.0f);
            else newVal = std::clamp(normY, 0.0f, 1.0f);
            
            targetCurve->points[draggingAutomationPointIndex].time = newTime;
            targetCurve->points[draggingAutomationPointIndex].value = newVal;
            
             std::sort(targetCurve->points.begin(), targetCurve->points.end());
             // Re-finding index after sort to avoid invalidating it
             for (int i=0; i<(int)targetCurve->points.size(); ++i) {
                 if (targetCurve->points[i].time == newTime && targetCurve->points[i].value == newVal) {
                     draggingAutomationPointIndex = i;
                     break;
                 }
             }
        }
        repaint();
        return;
    }

    if (currentDragMode == DragMode::Move)
    {
        if (e.mods.isAltDown() && draggingClip->getType() == Clip::Type::Audio)
        {
            // Slip Editing: Change offset instead of startTime
            double timeDelta = mouseTime - timeline.absolutePixelToTime(dragStartPos.x);
            draggingClip->offset = dragStartOffset - timeDelta; 
            // Negative delta because moving mouse right (positive delta) should shift waveform left (negative offset adjustment)
            // Wait: dragging content right means starting earlier in the source file? 
            // If I drag right, I want to see earlier parts of the file. So offset decreases. 
            // Yes, offset -= delta.
        }
        else
        {
            // Regular Move
            double newStart = timeline.snapToGrid(dragStartTime + deltaTime); 
            if (newStart < 0) newStart = 0;
            draggingClip->startTime = newStart;
        }
        
        // ── Crossfade Logic (existing) ──────────────────────────────
        // ... (I'll re-apply this in the full block below)

        // ── Crossfade Logic ──────────────────────────────────────────
        // Check for overlaps with neighbors
        auto& clips = trackInfo.clips;
        Clip* nextClip = nullptr;
        for (auto* c : clips) {
            if (c != draggingClip && c->startTime >= draggingClip->startTime) {
                if (!nextClip || c->startTime < nextClip->startTime) nextClip = c;
            }
        }

        if (nextClip) {
            double overlap = (draggingClip->startTime + draggingClip->duration) - nextClip->startTime;
            if (overlap > 0 && overlap < 2.0) { // Limit auto-crossfade to 2 seconds
                draggingClip->fadeOut = overlap;
                nextClip->fadeIn = overlap;
            } else if (overlap <= 0) {
                // Remove fade if no longer overlapping? 
                // Maybe keep it if user manually set it. 
                // Suggestion: only auto-fade if it was 0 or already an auto-fade.
            }
        }
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
        
        double newDuration = newEnd - draggingClip->startTime;
        
        if (e.mods.isAltDown() && draggingClip->getType() == Clip::Type::Audio)
        {
            auto* ac = static_cast<AudioClip*>(draggingClip);
            // newBpm = oldBpm * (oldDuration / newDuration)
            ac->sourceBpm = dragStartSourceBpm * (dragStartDuration / newDuration);
        }

        draggingClip->duration = newDuration;
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

    else if (currentDragMode == DragMode::SwipeComp)
    {
        // Highlight range logic (maybe store current swipe range and repaint)
        repaint();
    }

    repaint();
}

void TrackLaneComponent::mouseUp(const juce::MouseEvent& e)
{
    if (currentDragMode == DragMode::SwipeComp)
    {
        auto& trackInfo = audioEngine.getTrackInfo(trackIndex);
        double startTime = timeline.absolutePixelToTime(juce::jmin(dragStartPos.x, e.x));
        double endTime   = timeline.snapToGrid(timeline.absolutePixelToTime(juce::jmax(dragStartPos.x, e.x)));
        double duration  = endTime - startTime;

        if (duration > 0.01 && draggingTakeIndex >= 0 && draggingTakeIndex < trackInfo.takes.size())
        {
            // Perform Comping
            // 1. Remove/Split existing clips in range
            juce::OwnedArray<Clip> newClips;
            for (auto* c : trackInfo.clips)
            {
                double cEnd = c->startTime + c->duration;
                if (cEnd <= startTime || c->startTime >= endTime)
                {
                    // No overlap
                    newClips.add(c->clone());
                }
                else
                {
                    // Overlap: Split or Trim
                    if (c->startTime < startTime)
                    {
                        auto left = c->clone();
                        left->duration = startTime - c->startTime;
                        newClips.add(std::move(left));
                    }
                    if (cEnd > endTime)
                    {
                        auto right = c->clone();
                        double shift = endTime - c->startTime;
                        right->startTime = endTime;
                        right->duration = cEnd - endTime;
                        right->offset += shift;
                        newClips.add(std::move(right));
                    }
                }
            }
            
            // 2. Add segments from take
            for (auto* takeClip : trackInfo.takes[draggingTakeIndex]->clips)
            {
                double tcEnd = takeClip->startTime + takeClip->duration;
                double overlapStart = juce::jmax(startTime, takeClip->startTime);
                double overlapEnd   = juce::jmin(endTime, tcEnd);
                
                if (overlapEnd > overlapStart)
                {
                    auto segment = takeClip->clone();
                    segment->startTime = overlapStart;
                    segment->duration  = overlapEnd - overlapStart;
                    segment->offset   += (overlapStart - takeClip->startTime);
                    newClips.add(std::move(segment));
                }
            }

            // Sync back to trackInfo.clips
            trackInfo.clips.clear();
            while (newClips.size() > 0) trackInfo.clips.add(newClips.removeAndReturn(0));
            
            repaint();
        }
    }

    draggingClip = nullptr;
    draggingTakeIndex = -1;
    currentDragMode = DragMode::None;
}

void TrackLaneComponent::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (e.x < timeline.getTrackHeaderWidth()) return;

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
    double dropTime = timeline.snapToGrid(timeline.absolutePixelToTime(x));
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
    
    // Load audio data for playback
    clip->loadAudioData(audioEngine.getFormatManager());
    
    audioEngine.getTrackInfo(trackIndex).clips.add(clip);
    repaint();
}

void TrackLaneComponent::addMidiClip(double startTime, double duration)
{
    auto* clip = new MidiClip(startTime, duration);
    audioEngine.getTrackInfo(trackIndex).clips.add(clip);
    repaint();
}

//──────────────────────────────────────────────────────────────────────────────
// Drag and drop reordering
//──────────────────────────────────────────────────────────────────────────────
bool TrackLaneComponent::isInterestedInDragSource(const SourceDetails& dragSourceDetails)
{
    return dragSourceDetails.description.toString().startsWith("TrackLane:");
}

void TrackLaneComponent::itemDragEnter(const SourceDetails& dragSourceDetails)
{
    isDragHovering = true;
    repaint();
}

void TrackLaneComponent::itemDragMove(const SourceDetails& dragSourceDetails)
{
    bool newHover = (dragSourceDetails.localPosition.y < getHeight() / 2);
    if (newHover != hoverIsTopHalf)
    {
        hoverIsTopHalf = newHover;
        repaint();
    }
}

void TrackLaneComponent::itemDragExit(const SourceDetails& dragSourceDetails)
{
    isDragHovering = false;
    repaint();
}

void TrackLaneComponent::itemDropped(const SourceDetails& dragSourceDetails)
{
    isDragHovering = false;
    repaint();

    juce::String descStr = dragSourceDetails.description.toString();
    if (descStr.startsWith("TrackLane:"))
    {
        int sourceIndex = descStr.substring(10).getIntValue();
        if (sourceIndex != trackIndex) // did we drop it on a different track?
        {
            int insertIndex = trackIndex;
            if (!hoverIsTopHalf)
                insertIndex++;

            if (sourceIndex < insertIndex)
                insertIndex--; // Adjust since removing source shifts array

            if (sourceIndex != insertIndex)
            {
                appState.moveTrack(sourceIndex, insertIndex);
                audioEngine.moveTrack(sourceIndex, insertIndex);
                timeline.rebuildTracks();
            }
        }
    }
}

Clip* TrackLaneComponent::getClipAt(double timeSeconds)
{
    auto& trackInfo = audioEngine.getTrackInfo(trackIndex);
    for (auto* clip : trackInfo.clips)
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
