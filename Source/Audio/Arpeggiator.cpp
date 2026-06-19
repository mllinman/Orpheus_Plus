#include "Arpeggiator.h"
#include <algorithm>
#include <random>

Arpeggiator::Arpeggiator() {}
Arpeggiator::~Arpeggiator() {}

void Arpeggiator::setSampleRate(double newSampleRate)
{
    sampleRate = newSampleRate;
    samplesPerStep = (int)((60.0 / tempo) * 4.0 * syncRate * sampleRate);
}

void Arpeggiator::setTempo(double newTempo)
{
    tempo = newTempo;
    if (tempo > 0.0)
        samplesPerStep = (int)((60.0 / tempo) * 4.0 * syncRate * sampleRate);
}

void Arpeggiator::process(juce::MidiBuffer& midiMessages, int numSamples)
{
    if (!enabled || samplesPerStep <= 0)
        return;

    juce::MidiBuffer outBuffer;
    int samplePos = 0;
    
    // Process incoming events
    for (const auto meta : midiMessages)
    {
        const auto msg = meta.getMessage();
        int eventTime = meta.samplePosition;

        if (msg.isNoteOn())
            handleNoteOn(msg.getNoteNumber(), msg.getVelocity());
        else if (msg.isNoteOff())
            handleNoteOff(msg.getNoteNumber());
        else
            outBuffer.addEvent(msg, eventTime); // Pass through CCs/PitchBend
    }
    
    midiMessages.clear();

    if (heldNotes.empty())
    {
        if (currentPlayingNote >= 0)
        {
            outBuffer.addEvent(juce::MidiMessage::noteOff(1, currentPlayingNote, (uint8)0), 0);
            currentPlayingNote = -1;
        }
        midiMessages.swapWith(outBuffer);
        return;
    }

    // Process the sequence across the block
    while (samplePos < numSamples)
    {
        if (sampleCounter == 0)
        {
            if (currentPlayingNote >= 0)
            {
                outBuffer.addEvent(juce::MidiMessage::noteOff(1, currentPlayingNote, (juce::uint8)0), samplePos);
            }
            
            int nextNote = getNextNote();
            if (nextNote >= 0)
            {
                juce::uint8 velocity = 100; // default
                for (const auto& h : heldNotes) {
                    if (h.noteNumber == nextNote % 12 || h.noteNumber == nextNote) {
                        velocity = h.velocity;
                        break;
                    }
                }
                outBuffer.addEvent(juce::MidiMessage::noteOn(1, nextNote, velocity), samplePos);
                currentPlayingNote = nextNote;
            }
        }
        else if (sampleCounter == (int)(samplesPerStep * gateLength))
        {
            if (currentPlayingNote >= 0)
            {
                outBuffer.addEvent(juce::MidiMessage::noteOff(1, currentPlayingNote, (juce::uint8)0), samplePos);
                currentPlayingNote = -1;
            }
        }

        samplePos++;
        sampleCounter++;
        if (sampleCounter >= samplesPerStep)
        {
            sampleCounter = 0;
            currentStep++;
            if (currentSequence.empty()) currentStep = 0;
            else currentStep %= currentSequence.size();
        }
    }

    midiMessages.swapWith(outBuffer);
}

void Arpeggiator::handleNoteOn(int noteNumber, juce::uint8 velocity)
{
    // Remove if exists to re-add at end (AsPlayed order)
    handleNoteOff(noteNumber);
    heldNotes.push_back({noteNumber, velocity, (int)juce::Time::getMillisecondCounter()});
    updateSequence();
}

void Arpeggiator::handleNoteOff(int noteNumber)
{
    heldNotes.erase(std::remove_if(heldNotes.begin(), heldNotes.end(),
        [noteNumber](const ActiveNote& n) { return n.noteNumber == noteNumber; }), 
        heldNotes.end());
    updateSequence();
}

void Arpeggiator::updateSequence()
{
    currentSequence.clear();
    if (heldNotes.empty()) return;

    std::vector<int> sortedNotes;
    if (pattern == Pattern::AsPlayed)
    {
        for (const auto& n : heldNotes) sortedNotes.push_back(n.noteNumber);
    }
    else
    {
        for (const auto& n : heldNotes) sortedNotes.push_back(n.noteNumber);
        std::sort(sortedNotes.begin(), sortedNotes.end());
    }

    // Expand by octaves
    for (int oct = 0; oct < octaves; ++oct)
    {
        for (int note : sortedNotes)
        {
            int p = note + (oct * 12);
            if (p <= 127) currentSequence.push_back(p);
        }
    }

    if (pattern == Pattern::Down)
    {
        std::reverse(currentSequence.begin(), currentSequence.end());
    }
    else if (pattern == Pattern::UpDown)
    {
        auto seqCopy = currentSequence;
        std::reverse(seqCopy.begin(), seqCopy.end());
        // Remove first and last to avoid duplicates
        if (seqCopy.size() > 2)
        {
            seqCopy.erase(seqCopy.begin());
            seqCopy.pop_back();
            currentSequence.insert(currentSequence.end(), seqCopy.begin(), seqCopy.end());
        }
    }
    else if (pattern == Pattern::Random)
    {
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(currentSequence.begin(), currentSequence.end(), g);
    }

    if (currentStep >= currentSequence.size())
        currentStep = 0;
}

int Arpeggiator::getNextNote()
{
    if (currentSequence.empty()) return -1;
    if (currentStep >= currentSequence.size()) currentStep = 0;
    return currentSequence[currentStep];
}
