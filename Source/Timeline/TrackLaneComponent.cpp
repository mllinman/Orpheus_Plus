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
    trackNameLabel.setFont(juce::Font(12.0f, juce::Font::bold));
    trackNameLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    trackNameLabel.setEditable(true);
    trackNameLabel.onTextChange = [this] {
        audioEngine.getTrackInfo(trackIndex).name = trackNameLabel.getText();
    };
    addAndMakeVisible(trackNameLabel);

    setAcceptedMouseButtons(juce::MouseButton::allButtons);
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
    for (auto& clip : clips)
    {
        auto clipBounds = getClipScreenBounds(*clip);
        if (clipBounds.getRight() < clipArea.getX() ||
            clipBounds.getX() > clipArea.getRight())
            continue;

        if (clip->type == Clip::Type::Audio)
            paintAudioClip(g, *clip, clipBounds);
        else
            paintMidiClip(g, *clip, clipBounds);
    }
}

void TrackLaneComponent::paintAudioClip(juce::Graphics& g, const Clip& clip,
                                         juce::Rectangle<float> clipBounds)
{
    auto& info = audioEngine.getTrackInfo(trackIndex);

    // Background
    g.setColour(clip.selected ? info.colour.brighter(0.3f) : info.colour.withBrightness(0.5f));
    g.fillRoundedRectangle(clipBounds, 3.0f);

    // Title
    g.setColour(juce::Colours::white.withAlpha(0.9f));
    g.setFont(juce::Font(10.0f));
    g.drawText(clip.name, clipBounds.toNearestInt().reduced(4, 2),
               juce::Justification::topLeft, true);

    // Waveform
    if (clip.thumbnail)
    {
        g.setColour(juce::Colours::white.withAlpha(0.6f));
        auto waveArea = clipBounds.reduced(0, 14);
        clip.thumbnail->drawChannel(g, waveArea.toNearestInt(), 0.0, clip.duration, 0, 1.0f);
    }

    // Border
    g.setColour(clip.selected ? juce::Colours::white : info.colour.brighter(0.2f));
    g.drawRoundedRectangle(clipBounds, 3.0f, 1.0f);
}

void TrackLaneComponent::paintMidiClip(juce::Graphics& g, const Clip& clip,
                                        juce::Rectangle<float> clipBounds)
{
    auto& info = audioEngine.getTrackInfo(trackIndex);

    g.setColour(clip.selected ? juce::Colour(0xffbb86fc) : juce::Colour(0xff7b2d8b));
    g.fillRoundedRectangle(clipBounds, 3.0f);

    g.setColour(juce::Colours::white.withAlpha(0.9f));
    g.setFont(juce::Font(10.0f));
    g.drawText(clip.name.isEmpty() ? "MIDI Clip" : clip.name,
               clipBounds.toNearestInt().reduced(4, 2),
               juce::Justification::topLeft, true);

    // Draw mini piano roll preview
    if (!clip.midiData.isEmpty())
    {
        int numNotes = clip.midiData.getNumEvents();
        int noteRange = 127;
        float noteH = juce::jmax(1.0f, clipBounds.getHeight() / 16.0f);

        for (int i = 0; i < numNotes; ++i)
        {
            auto* e = clip.midiData.getEventPointer(i);
            if (e->message.isNoteOn())
            {
                int note = e->message.getNoteNumber();
                double noteStart = e->message.getTimeStamp() / clip.duration;
                float nx = clipBounds.getX() + (float)noteStart * clipBounds.getWidth();
                float ny = clipBounds.getBottom() - (note / 127.0f) * clipBounds.getHeight();

                g.setColour(juce::Colours::white.withAlpha(0.7f));
                g.fillRect(nx, ny, 4.0f, noteH);
            }
        }
    }

    g.setColour(clip.selected ? juce::Colours::white : juce::Colour(0xffbb86fc).withAlpha(0.5f));
    g.drawRoundedRectangle(clipBounds, 3.0f, 1.0f);
}

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

    double clickTime = timeline.pixelToTime(e.x + HEADER_WIDTH);
    draggedClip = getClipAt(clickTime);

    if (draggedClip)
    {
        selectedClip = draggedClip;
        dragStartTime = clickTime - draggedClip->startTime;
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

    double newStart = timeline.pixelToTime(e.x + HEADER_WIDTH) - dragStartTime;
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

    double clickTime = timeline.pixelToTime(e.x + HEADER_WIDTH);
    if (auto* clip = getClipAt(clickTime))
    {
        if (clip->type == Clip::Type::Midi)
        {
            // TODO: open piano roll with this clip
        }
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
    double dropTime = timeline.pixelToTime(x + HEADER_WIDTH);
    for (auto& f : files)
        addAudioClip(juce::File(f), dropTime);
}

void TrackLaneComponent::addAudioClip(const juce::File& file, double startTime)
{
    auto* clip       = clips.add(new Clip());
    clip->type       = Clip::Type::Audio;
    clip->startTime  = startTime;
    clip->name       = file.getFileNameWithoutExtension();
    clip->sourceFile = file;
    clip->colour     = audioEngine.getTrackInfo(trackIndex).colour;

    // Create thumbnail
    clip->thumbnail = new juce::AudioThumbnail(512, audioEngine.getFormatManager(),
                                                thumbnailCache);
    clip->thumbnail->addChangeListener(this);
    clip->thumbnail->setSource(new juce::FileInputSource(file));

    // Get duration from file
    if (auto* reader = audioEngine.getFormatManager().createReaderFor(file))
    {
        clip->duration = reader->lengthInSamples / reader->sampleRate;
        delete reader;
    }

    repaint();
}

void TrackLaneComponent::addMidiClip(double startTime, double duration)
{
    auto* clip      = clips.add(new Clip());
    clip->type      = Clip::Type::Midi;
    clip->startTime = startTime;
    clip->duration  = duration;
    clip->name      = "MIDI Clip";
    repaint();
}

Clip* TrackLaneComponent::getClipAt(double timeSeconds)
{
    for (auto* clip : clips)
        if (timeSeconds >= clip->startTime &&
            timeSeconds < clip->startTime + clip->duration)
            return clip;
    return nullptr;
}

void TrackLaneComponent::changeListenerCallback(juce::ChangeBroadcaster*)
{
    repaint();
}
