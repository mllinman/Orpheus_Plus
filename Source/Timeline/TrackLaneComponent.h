#pragma once
#include <JuceHeader.h>
#include "../Audio/AudioEngine.h"
#include "../Project/AppState.h"

class TimelineComponent;

//==============================================================================
// Represents a single audio or MIDI clip on the timeline
struct Clip
{
    enum class Type { Audio, Midi };
    Type   type        = Type::Audio;
    double startTime   = 0.0;    // seconds
    double duration    = 4.0;    // seconds
    double offset      = 0.0;    // start offset within source
    juce::String name;
    juce::File   sourceFile;     // for audio clips
    juce::MidiMessageSequence midiData; // for MIDI clips
    juce::Colour colour;
    bool selected = false;
    juce::AudioThumbnail* thumbnail = nullptr;
};

//==============================================================================
class TrackLaneComponent : public juce::Component,
                           private juce::ChangeListener
{
public:
    TrackLaneComponent(int trackIndex,
                       AudioEngine& engine,
                       AppState& state,
                       TimelineComponent& timeline);
    ~TrackLaneComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

    void addAudioClip(const juce::File& file, double startTime);
    void addMidiClip(double startTime, double duration = 4.0);

private:
    void changeListenerCallback(juce::ChangeBroadcaster*) override;
    void paintHeader(juce::Graphics& g, juce::Rectangle<int> headerBounds);
    void paintClips(juce::Graphics& g, juce::Rectangle<int> clipArea);
    void paintAudioClip(juce::Graphics& g, const Clip& clip, juce::Rectangle<float> clipBounds);
    void paintMidiClip(juce::Graphics& g, const Clip& clip, juce::Rectangle<float> clipBounds);
    void paintVolumeAutomation(juce::Graphics& g, juce::Rectangle<int> clipArea);

    Clip* getClipAt(double timeSeconds);
    juce::Rectangle<float> getClipScreenBounds(const Clip& clip) const;

    static constexpr int HEADER_WIDTH = 180;

    int          trackIndex;
    AudioEngine& audioEngine;
    AppState&    appState;
    TimelineComponent& timeline;

    juce::OwnedArray<Clip> clips;
    Clip* draggedClip    = nullptr;
    Clip* selectedClip   = nullptr;
    double dragStartTime = 0.0;

    // Header controls
    juce::TextButton muteButton  { "M" };
    juce::TextButton soloButton  { "S" };
    juce::TextButton armButton   { "R" };
    juce::Slider     volumeSlider;
    juce::Slider     panSlider;
    juce::Label      trackNameLabel;

    // Waveform thumbnails
    juce::AudioThumbnailCache thumbnailCache { 32 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackLaneComponent)
};
