#include "TablatureGenerator.h"

namespace Transcription {

TablatureGenerator::TablatureGenerator() {
    updateTunings();
}

void TablatureGenerator::setInstrument(InstrumentType type) {
    currentInstrument = type;
    updateTunings();
}

void TablatureGenerator::updateTunings() {
    stringTunings.clear();
    switch (currentInstrument) {
        case InstrumentType::AcousticGuitar:
            // Standard E A D G B E
            // String 0: E4 (64), 1: B3 (59), 2: G3 (55), 3: D3 (50), 4: A2 (45), 5: E2 (40)
            stringTunings = {64, 59, 55, 50, 45, 40};
            maxFret = 20;
            break;
        case InstrumentType::ElectricGuitar:
            stringTunings = {64, 59, 55, 50, 45, 40};
            maxFret = 24;
            break;
        case InstrumentType::Bass:
            // Standard E A D G
            // String 0: G2 (43), 1: D2 (38), 2: A1 (33), 3: E1 (28)
            stringTunings = {43, 38, 33, 28};
            maxFret = 24;
            break;
    }
}

int TablatureGenerator::getNumStrings() const {
    return (int)stringTunings.size();
}

juce::String TablatureGenerator::getStringName(int index) const {
    if (index < 0 || index >= stringTunings.size()) return "";
    int midi = stringTunings[index];
    const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    int noteIndex = midi % 12;
    return juce::String(noteNames[noteIndex]);
}

std::vector<TabNote> TablatureGenerator::generateTabs(const std::vector<std::pair<int, float>>& midiNotes) {
    std::vector<TabNote> result;
    int lastString = 0;
    int lastFret = 0;

    for (const auto& note : midiNotes) {
        int midi = note.first;
        float beat = note.second;

        int bestString = -1;
        int bestFret = -1;
        int minDistance = 9999;

        // Simple heuristic: find a string where this note is playable (0 <= fret <= maxFret)
        // Optimize for minimum fret distance from the last note played
        for (int i = 0; i < stringTunings.size(); ++i) {
            int fret = midi - stringTunings[i];
            if (fret >= 0 && fret <= maxFret) {
                int distance = std::abs(fret - lastFret) + std::abs(i - lastString) * 2;
                if (distance < minDistance) {
                    minDistance = distance;
                    bestString = i;
                    bestFret = fret;
                }
            }
        }

        if (bestString != -1) {
            result.push_back({bestString, bestFret, beat, 1.0f});
            lastString = bestString;
            lastFret = bestFret;
        }
    }

    return result;
}

}
