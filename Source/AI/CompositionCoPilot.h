#pragma once
#include <JuceHeader.h>
#include "../Timeline/MidiClip.h"
#include "../Timeline/AudioClip.h"
#include <vector>
#include <memory>

/**
 * CompositionCoPilot provides AI-assisted composition tools:
 * 1. Generative Progression Engine
 * 2. Melody Autocomplete
 * 3. Rhythm Style Transfer
 */
class CompositionCoPilot
{
public:
    CompositionCoPilot() = default;
    ~CompositionCoPilot() = default;

    /**
     * Generates a chord progression based on genre/style.
     * Returns a new MidiClip containing the progression.
     */
    std::unique_ptr<MidiClip> generateProgression(const juce::String& genre, const juce::String& style, int numBars, double bpm);

    /**
     * Takes an existing MidiClip (e.g., a 4-bar melody) and generates 
     * a stylistically matching continuation.
     * Returns a new MidiClip with the original and the new continuation.
     */
    std::unique_ptr<MidiClip> autocompleteMelody(const MidiClip& seedMelody, int additionalBars, double bpm);

    /**
     * Extracts rhythmic transients/onsets from an AudioClip and quantizes
     * the events in a target MidiClip to match that groove.
     */
    void extractRhythmAndApply(const AudioClip& grooveSource, MidiClip& targetMidi);

private:
    // Heuristics for the progression generation
    void addChordToClip(MidiClip& clip, int rootNote, const juce::String& type, double startTime, double duration);

    // Basic scale/key data
    std::vector<int> getScaleNotes(int rootNote, const juce::String& scaleType);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompositionCoPilot)
};
