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
    double x1 = timeline.timeToPixel(clip.startTime) - HEADER_WIDTH;
    double x2 = timeline.timeToPixel(clip.startTime + clip.duration) - HEADER_WIDTH;
    float  y  = 4.0f;
    float  h  = (float)(getHeight() - 8);
    return { (float)x1, y, (float)(x2 - x1), h };
}

void TrackLaneComponent::mouseDown(const juce::MouseEvent& e)
{
    if (e.x < HEADER_WIDTH) return;

    double clickTime = timeline.snapToGrid(timeline.pixelToTime(e.x));
    auto tool = timeline.getTool();

    if (tool == TimelineComponent::EditTool::Split)
    {
        if (auto* clip = getClipAt(clickTime))
        {
            // Split logic
            // 1. Shorten current clip
            double originalEnd = clip->startTime + clip->duration;
            double newDuration = clickTime - clip->startTime;
            double remaining   = originalEnd - clickTime;
            
            if (newDuration > 0.05 && remaining > 0.05) // Min clip length
            {
                clip->duration = newDuration;

                // 2. Create new clip
                if (clip->getType() == Clip::Type::Audio) // ...
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
        if (auto* clip = getClipAt(clickTime))
        {
            audioEngine.getTrackInfo(trackIndex).clips.removeObject(clip);
            repaint();
        }
        return;
    }

    draggedClip = getClipAt(timeline.pixelToTime(e.x)); // Don't snap for selection check!
    
    // If we clicked a clip, calculate offset. 
    // If we snap here, we might miss the clip if it's not on grid? 
    // No, getClipAt checks bounds. 
    // But dragStartTime should be precise relative to clip start.
    
    if (draggedClip)
    {
        selectedClip = draggedClip;
        dragStartTime = timeline.pixelToTime(e.x) - draggedClip->startTime; 
        // Use unsnapped time for drag offset calculation to avoid jumpiness
    }
    else if (e.mods.isRightButtonDown())
    {
        // Right click: context menu
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

    repaint();
}

void TrackLaneComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (!draggedClip || e.x < HEADER_WIDTH) return;

    double currentPos = timeline.pixelToTime(e.x);
    double newStart   = currentPos - dragStartTime;
    newStart = timeline.snapToGrid(newStart);
    draggedClip->startTime = juce::jmax(0.0, newStart);
    repaint();
}

void TrackLaneComponent::mouseUp(const juce::MouseEvent&)
{
    draggedClip = nullptr;
}

void TrackLaneComponent::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (e.x < HEADER_WIDTH) return;

    double clickTime = timeline.snapToGrid(timeline.pixelToTime(e.x));
    if (auto* clip = getClipAt(timeline.pixelToTime(e.x))) // Check unsnapped for hit test
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

void TrackLaneComponent::changeListenerCallback(juce::ChangeBroadcaster*)
{
    repaint();
}
