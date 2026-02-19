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

    void loadMidiSequence(const juce::MidiMessageSequence& seq);
    juce::MidiMessageSequence getMidiSequence() const;

    void setQuantization(double beatDivision) { quantizeDivision = beatDivision; }
    void selectAll();
    void deleteSelected();
    void quantizeSelected();

private:
    void timerCallback() override;
    void paintPianoKeys(juce::Graphics&, juce::Rectangle<int>);
    void paintGrid(juce::Graphics&, juce::Rectangle<int>);
    void paintNotes(juce::Graphics&, juce::Rectangle<int>);
    void paintPlayhead(juce::Graphics&);

    MidiNote* getNoteAt(double beat, int pitch);
    juce::Rectangle<float> getNoteBounds(const MidiNote& note) const;
    int   pixelToPitch(int y) const;
    double pixelToBeat(int x) const;
    int   pitchToPixel(int pitch) const;
    int   beatToPixel(double beat) const;

    static constexpr int PIANO_KEY_WIDTH = 52;
    static constexpr int NOTE_HEIGHT     = 14;
    static constexpr int NUM_OCTAVES     = 8;
    static constexpr int NUM_NOTES       = 128;

    AppState&    appState;
    AudioEngine& audioEngine;

    juce::OwnedArray<MidiNote> notes;
    MidiNote* draggingNote    = nullptr;
    MidiNote* resizingNote    = nullptr;
    bool      isDrawingMode   = true;

    double  pixelsPerBeat     = 80.0;
    double  verticalOffset    = (NUM_NOTES / 2) * NOTE_HEIGHT - 200;
    double  horizontalOffset  = 0.0;
    double  quantizeDivision  = 0.25; // 1/16th note

    juce::Colour noteColour { 0xff4fc3f7 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRollComponent)
};
