#pragma once
#include <JuceHeader.h>
#include "../Project/AppState.h"
#include "../Audio/AudioEngine.h"

//==============================================================================
struct MidiNote
{
    int    pitch     = 60;
    double startBeat = 0.0;
    double duration  = 0.25; // in beats
    int    velocity  = 100;
    bool   selected  = false;
};

//==============================================================================
class PianoRollComponent : public juce::Component,
                           public juce::MidiInputCallback,
                           private juce::Timer
{
public:
    PianoRollComponent(AppState& state, AudioEngine& engine);
    ~PianoRollComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    bool keyPressed(const juce::KeyPress& key) override;

    // MIDI Input
    void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) override;

    void loadMidiSequence(const juce::MidiMessageSequence& seq);
    juce::MidiMessageSequence getMidiSequence() const;
    void setActiveClip(class MidiClip* clip);
    void syncToClip();

    void setQuantization(double beatDivision) { quantizeDivision = beatDivision; }
    void selectAll();
    void deleteSelected();
    void quantizeSelected();

    // AI Generative Tools
    void generateAIChords();
    void generateAIMelody();
    void arpeggiate();
    void humanize();

private:
    void timerCallback() override;
    void paintPianoKeys(juce::Graphics&, juce::Rectangle<int>);
    void paintGrid(juce::Graphics&, juce::Rectangle<int>);
    void paintNotes(juce::Graphics&, juce::Rectangle<int>);
    void paintVelocityLane(juce::Graphics&, juce::Rectangle<int>);
    void paintPlayhead(juce::Graphics&);
    void enableMidiInput();

    MidiNote* getNoteAt(double beat, int pitch);
    juce::Rectangle<float> getNoteBounds(const MidiNote& note) const;
    int   pixelToPitch(int y) const;
    double pixelToBeat(int x) const;
    int   pitchToPixel(int pitch) const;
    int   beatToPixel(double beat) const;

    int keyboardWidth = 52;
    juce::StretchableLayoutManager horizontalLayout;
    std::unique_ptr<juce::StretchableLayoutResizerBar> resizerBar;
    static constexpr int NOTE_HEIGHT     = 14;
    static constexpr int NUM_OCTAVES     = 8;
    static constexpr int NUM_NOTES       = 128;

    AppState&    appState;
    AudioEngine& audioEngine;

    juce::OwnedArray<MidiNote> notes;
    class MidiClip* activeClip = nullptr;
    MidiNote* draggingNote    = nullptr;
    MidiNote* resizingNote    = nullptr;
    bool      isDrawingMode   = true;

    double  pixelsPerBeat     = 80.0;
    double  verticalOffset    = (NUM_NOTES / 2) * NOTE_HEIGHT - 200;
    double  horizontalOffset  = 0.0;
    double  quantizeDivision  = 0.25; // 1/16th note

    // Drag and selection state
    juce::Rectangle<int> selectionLasso;
    bool isLassoDragging = false;
    
    struct NoteDragState {
        MidiNote* note;
        double originalBeat;
        int originalPitch;
    };
    std::vector<NoteDragState> draggingNotes;
    double dragStartBeat = 0.0;
    int dragStartPitch = 0;

    bool isEditingVelocity = false;
    static constexpr int VELOCITY_LANE_HEIGHT = 60;

    juce::Colour noteColour { 0xff6c5ce7 };  // Legacy accent purple

    // Live MIDI input state — tracks which keys are currently pressed
    std::array<bool, 128>  liveNoteState {};    // true = key held down
    std::array<uint8_t, 128> liveNoteVelocity {}; // velocity of held key
    juce::CriticalSection  midiStateLock;

    // UI Tools Toolbar
    juce::Component toolBar;
    juce::TextButton btnAIChords { "AI Chords" };
    juce::TextButton btnAIMelody { "AI Melody" };
    juce::TextButton btnArpeggiate { "Arpeggiate" };
    juce::TextButton btnHumanize { "Humanize" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRollComponent)
};
