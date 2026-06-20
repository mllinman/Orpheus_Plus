#pragma once
#include <JuceHeader.h>

class MelodyCompleter
{
public:
    MelodyCompleter() = default;
    ~MelodyCompleter() = default;

    // Takes a short MIDI sequence and generates a logical continuation based on key/scale.
    // In the future, this calls an ONNX Sequence-to-Sequence model.
    juce::MidiMessageSequence generateCompletion(const juce::MidiMessageSequence& inputSequence, int scaleRoot, int scaleType, int numNotesToGenerate);

private:
    // Simple Markov-chain style stub for generating notes in-key
    int getNextLogicalNoteInScale(int lastNote, int scaleRoot, int scaleType);
};
