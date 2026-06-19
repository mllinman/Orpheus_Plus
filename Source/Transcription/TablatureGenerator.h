#pragma once
#include <JuceHeader.h>
#include <vector>
#include <utility>

namespace Transcription {

enum class InstrumentType {
    AcousticGuitar,
    ElectricGuitar,
    Bass
};

struct TabNote {
    int stringIndex; // 0 is highest pitch string
    int fret;
    float beatPosition;
    float durationBeats;
};

enum class Tuning {
    Standard,
    DropD,
    HalfStepDown,
    OpenG
};

class TablatureGenerator {
public:
    TablatureGenerator();
    
    void setInstrument(InstrumentType type);
    void setTuning(Tuning t);
    void setMaxStretch(int frets) { maxStretch = frets; }
    void setPreferredPosition(int fret) { preferredPosition = fret; }
    
    // Generates a simple tab from a sequence of midi notes
    // midiNotes: pair of {midiNoteNumber, startBeat}
    std::vector<TabNote> generateTabs(const std::vector<std::pair<int, float>>& midiNotes);

    int getNumStrings() const;
    juce::String getStringName(int index) const;

private:
    InstrumentType currentInstrument = InstrumentType::AcousticGuitar;
    Tuning currentTuning = Tuning::Standard;
    
    std::vector<int> stringTunings; 
    int maxFret = 24;
    int maxStretch = 4;
    int preferredPosition = 0;

    void updateTunings();
};

}
