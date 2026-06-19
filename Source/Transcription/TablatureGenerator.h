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

class TablatureGenerator {
public:
    TablatureGenerator();
    
    void setInstrument(InstrumentType type);
    
    // Generates a simple tab from a sequence of midi notes
    // midiNotes: pair of {midiNoteNumber, startBeat}
    std::vector<TabNote> generateTabs(const std::vector<std::pair<int, float>>& midiNotes);

    int getNumStrings() const;
    juce::String getStringName(int index) const;

private:
    InstrumentType currentInstrument = InstrumentType::AcousticGuitar;
    
    std::vector<int> stringTunings; 
    int maxFret = 22;

    void updateTunings();
};

}
