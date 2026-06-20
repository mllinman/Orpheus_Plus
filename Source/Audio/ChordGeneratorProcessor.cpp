#include "ChordGeneratorProcessor.h"
#include <random>

std::vector<int> ChordGeneratorProcessor::getScaleDegrees(ScaleType type)
{
    switch (type)
    {
        case ScaleType::Major:         return { 0, 2, 4, 5, 7, 9, 11 };
        case ScaleType::Minor:         return { 0, 2, 3, 5, 7, 8, 10 };
        case ScaleType::HarmonicMinor: return { 0, 2, 3, 5, 7, 8, 11 };
        case ScaleType::Dorian:        return { 0, 2, 3, 5, 7, 9, 10 };
        case ScaleType::Phrygian:      return { 0, 1, 3, 5, 7, 8, 10 };
        case ScaleType::Lydian:        return { 0, 2, 4, 6, 7, 9, 11 };
        case ScaleType::Mixolydian:    return { 0, 2, 4, 5, 7, 9, 10 };
    }
    return { 0, 2, 4, 5, 7, 9, 11 };
}

int ChordGeneratorProcessor::getNoteInScale(int rootNote, const std::vector<int>& scaleDegrees, int degreeIndex)
{
    int octaves = degreeIndex / 7;
    int scaleIdx = degreeIndex % 7;
    if (scaleIdx < 0) {
        scaleIdx += 7;
        octaves -= 1;
    }
    return rootNote + octaves * 12 + scaleDegrees[scaleIdx];
}

std::vector<GeneratedNote> ChordGeneratorProcessor::generateChords(int rootNote, ScaleType scale, int numChords, double startTime, double chordDuration)
{
    auto degrees = getScaleDegrees(scale);
    std::vector<int> standardProgression = { 0, 5, 3, 4 }; // I, VI, IV, V
    std::vector<GeneratedNote> generatedNotes;

    for (int i = 0; i < numChords; ++i)
    {
        int chordRootDegree = standardProgression[i % standardProgression.size()];
        
        int root = getNoteInScale(rootNote, degrees, chordRootDegree);
        int third = getNoteInScale(rootNote, degrees, chordRootDegree + 2);
        int fifth = getNoteInScale(rootNote, degrees, chordRootDegree + 4);

        double t = startTime + i * chordDuration;
        
        generatedNotes.push_back({ root,  100, t, chordDuration });
        generatedNotes.push_back({ third, 90,  t, chordDuration });
        generatedNotes.push_back({ fifth, 90,  t, chordDuration });
        
        // Add a 7th note for extra flavor every 4th chord
        if (i % 4 == 3) {
            int seventh = getNoteInScale(rootNote, degrees, chordRootDegree + 6);
            generatedNotes.push_back({ seventh, 85, t, chordDuration });
        }
    }
    return generatedNotes;
}

std::vector<GeneratedNote> ChordGeneratorProcessor::generateMelody(int rootNote, ScaleType scale, double startTime, double duration)
{
    auto degrees = getScaleDegrees(scale);
    std::vector<GeneratedNote> generatedNotes;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> disScale(-2, 9);
    std::uniform_int_distribution<> disDur(1, 3);
    
    double t = startTime;
    double maxT = startTime + duration;
    
    while (t < maxT)
    {
        int note = getNoteInScale(rootNote + 12, degrees, disScale(gen)); // +12 to start an octave higher
        double noteDur = disDur(gen) * 0.25; // 16th, 8th, or dotted 8th note
        if (t + noteDur > maxT) noteDur = maxT - t;
        
        generatedNotes.push_back({ note, 110, t, noteDur });
        t += noteDur;
    }
    return generatedNotes;
}
