#pragma once
#include <JuceHeader.h>
#include <vector>

struct GeneratedNote {
    int pitch;
    int velocity;
    double beat;
    double duration;
};

class ChordGeneratorProcessor
{
public:
    enum class ScaleType { Major, Minor, HarmonicMinor, Dorian, Phrygian, Lydian, Mixolydian };

    static std::vector<int> getScaleDegrees(ScaleType type);
    static std::vector<GeneratedNote> generateChords(int rootNote, ScaleType scale, int numChords, double startBeat, double chordDuration);
    static std::vector<GeneratedNote> generateMelody(int rootNote, ScaleType scale, double startBeat, double duration);

private:
    static int getNoteInScale(int rootNote, const std::vector<int>& scaleDegrees, int degreeIndex);
};

