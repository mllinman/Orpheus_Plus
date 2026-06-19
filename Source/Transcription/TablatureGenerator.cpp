#include "TablatureGenerator.h"

namespace Transcription {

TablatureGenerator::TablatureGenerator() {
    updateTunings();
}

void TablatureGenerator::setInstrument(InstrumentType type) {
    currentInstrument = type;
    updateTunings();
}

void TablatureGenerator::setTuning(Tuning t) {
    currentTuning = t;
    updateTunings();
}

void TablatureGenerator::updateTunings() {
    stringTunings.clear();
    switch (currentInstrument) {
        case InstrumentType::AcousticGuitar:
        case InstrumentType::ElectricGuitar:
            // Standard E A D G B E
            stringTunings = {64, 59, 55, 50, 45, 40};
            maxFret = (currentInstrument == InstrumentType::AcousticGuitar) ? 20 : 24;
            break;
        case InstrumentType::Bass:
            // Standard E A D G
            stringTunings = {43, 38, 33, 28};
            maxFret = 24;
            break;
    }
    
    if (currentTuning == Tuning::DropD) {
        if (stringTunings.size() >= 6) stringTunings[5] = 38; // Drop E to D
        else if (stringTunings.size() == 4) stringTunings[3] = 26; // Drop Bass E to D
    }
    else if (currentTuning == Tuning::HalfStepDown) {
        for (auto& s : stringTunings) s -= 1;
    }
    else if (currentTuning == Tuning::OpenG) {
        if (stringTunings.size() >= 6) {
            stringTunings = {62, 59, 55, 50, 43, 38}; // D B G D G D
        }
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
    int lastFret = preferredPosition;
    float currentBeat = -1.0f;
    std::vector<int> stringsUsedOnCurrentBeat;

    for (const auto& note : midiNotes) {
        int midi = note.first;
        float beat = note.second;

        // Reset tracking if we are on a new beat
        if (std::abs(beat - currentBeat) > 0.05f) {
            currentBeat = beat;
            stringsUsedOnCurrentBeat.clear();
        }

        int bestString = -1;
        int bestFret = -1;
        int minDistance = 99999;

        for (int i = 0; i < stringTunings.size(); ++i) {
            // Cannot play multiple notes on the same string simultaneously
            if (std::find(stringsUsedOnCurrentBeat.begin(), stringsUsedOnCurrentBeat.end(), i) != stringsUsedOnCurrentBeat.end()) {
                continue;
            }

            int fret = midi - stringTunings[i];
            if (fret >= 0 && fret <= maxFret) {
                // Penalize distance from preferred position
                int posDistance = std::abs(fret - preferredPosition);
                // Penalize distance from last fret
                int moveDistance = std::abs(fret - lastFret);
                // Penalize string skips
                int stringDistance = std::abs(i - lastString) * 2;
                
                // Heavily penalize exceeding max stretch on the same beat (chords)
                int stretchPenalty = 0;
                if (!stringsUsedOnCurrentBeat.empty()) {
                    int maxCurrentFret = lastFret; // approximation
                    if (std::abs(fret - maxCurrentFret) > maxStretch && fret != 0 && maxCurrentFret != 0) {
                        stretchPenalty = 500;
                    }
                }

                int totalDistance = posDistance * 2 + moveDistance + stringDistance + stretchPenalty;
                
                // Prefer open strings if near nut
                if (fret == 0 && preferredPosition <= 3) totalDistance -= 5;

                if (totalDistance < minDistance) {
                    minDistance = totalDistance;
                    bestString = i;
                    bestFret = fret;
                }
            }
        }

        if (bestString != -1) {
            result.push_back({bestString, bestFret, beat, 1.0f});
            lastString = bestString;
            lastFret = bestFret;
            stringsUsedOnCurrentBeat.push_back(bestString);
        }
    }

    return result;
}

}
