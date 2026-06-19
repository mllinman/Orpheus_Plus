#pragma once
#include <JuceHeader.h>
#include <vector>

class Arpeggiator
{
public:
    enum class Pattern { Up, Down, UpDown, Random, AsPlayed };

    Arpeggiator();
    ~Arpeggiator();

    void setSampleRate(double newSampleRate);
    void setTempo(double newTempo);
    
    void setEnabled(bool shouldBeEnabled) { enabled = shouldBeEnabled; }
    void setPattern(Pattern p) { pattern = p; }
    void setRate(float rateInBeats) { syncRate = rateInBeats; } // e.g. 0.25 for 16th notes
    void setGate(float gateLengthRatio) { gateLength = gateLengthRatio; } // 0.1 to 1.0
    void setOctaves(int numOctaves) { octaves = numOctaves; }
    
    // Process a block of MIDI messages
    void process(juce::MidiBuffer& midiMessages, int numSamples);

private:
    void handleNoteOn(int noteNumber, juce::uint8 velocity);
    void handleNoteOff(int noteNumber);
    void updateSequence();
    int getNextNote();

    bool enabled { false };
    Pattern pattern { Pattern::Up };
    float syncRate { 0.25f };
    float gateLength { 0.5f };
    int octaves { 1 };
    
    double sampleRate { 44100.0 };
    double tempo { 120.0 };
    
    int currentStep { 0 };
    int samplesPerStep { 0 };
    int sampleCounter { 0 };
    
    struct ActiveNote {
        int noteNumber;
        juce::uint8 velocity;
        int timeAdded;
    };
    
    std::vector<ActiveNote> heldNotes;
    std::vector<int> currentSequence; // pitches
    
    int currentPlayingNote { -1 };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Arpeggiator)
};
