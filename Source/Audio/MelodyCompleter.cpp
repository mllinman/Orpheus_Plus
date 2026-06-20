#include "MelodyCompleter.h"
#include <random>

juce::MidiMessageSequence MelodyCompleter::generateCompletion(const juce::MidiMessageSequence& inputSequence, int scaleRoot, int scaleType, int numNotesToGenerate)
{
    juce::MidiMessageSequence generatedSeq;
    
    if (inputSequence.getNumEvents() == 0 || numNotesToGenerate <= 0)
        return generatedSeq;

    // Get the very last note played to start the Markov chain
    auto lastEvent = inputSequence.getEventPointer(inputSequence.getNumEvents() - 1);
    double lastTime = lastEvent->message.getTimeStamp();
    int lastNoteNumber = 60;

    for (int i = inputSequence.getNumEvents() - 1; i >= 0; --i) {
        if (inputSequence.getEventPointer(i)->message.isNoteOn()) {
            lastNoteNumber = inputSequence.getEventPointer(i)->message.getNoteNumber();
            break;
        }
    }

    double tickSpacing = 480.0; // Standard quarter note at 120bpm

    for (int i = 0; i < numNotesToGenerate; ++i) {
        int nextNote = getNextLogicalNoteInScale(lastNoteNumber, scaleRoot, scaleType);
        
        // Add Note On
        juce::MidiMessage noteOn = juce::MidiMessage::noteOn(1, nextNote, 0.8f);
        noteOn.setTimeStamp(lastTime + tickSpacing * (i + 1));
        generatedSeq.addEvent(noteOn);

        // Add Note Off (8th note length) using noteOff method or noteOn with 0 velocity
        juce::MidiMessage noteOffMsg = juce::MidiMessage::noteOff(1, nextNote, 0.0f);
        noteOffMsg.setTimeStamp(lastTime + tickSpacing * (i + 1) + (tickSpacing / 2.0));
        generatedSeq.addEvent(noteOffMsg);

        lastNoteNumber = nextNote;
    }

    generatedSeq.updateMatchedPairs();
    return generatedSeq;
}

int MelodyCompleter::getNextLogicalNoteInScale(int lastNote, int scaleRoot, int scaleType)
{
    // MOCK: Just random walks up or down the scale by 1 or 2 steps
    // A real implementation passes the input sequence to a Transformer model
    std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<int> stepDist(-2, 2);
    
    int step = stepDist(gen);
    
    // Very simplified generic major scale constraint mapping
    int nextNote = lastNote + (step * 2); 
    return juce::jlimit(21, 108, nextNote);
}
