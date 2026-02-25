#include "ArpeggiatorProcessor.h"

ArpeggiatorProcessor::ArpeggiatorProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

ArpeggiatorProcessor::~ArpeggiatorProcessor() {}

void ArpeggiatorProcessor::prepareToPlay(double, int)
{
    activeNotes.clear();
    currentNoteIndex = 0;
    timeSinceLastNote = 0.0;
    lastNotePlayed = -1;
}

void ArpeggiatorProcessor::releaseResources() {}

void ArpeggiatorProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear();
    
    // We assume 120 BPM for this simple prototype, so 1 beat = 0.5s.
    double bpm = 120.0;
    if (auto* playHead = getPlayHead())
    {
        if (auto pos = playHead->getPosition())
        {
            if (pos->getBpm().hasValue())
                bpm = *pos->getBpm();
        }
    }

    double sr = getSampleRate();
    if (sr <= 0) return;
    
    int numSamples = buffer.getNumSamples();
    double beatLengthSecs = 60.0 / bpm;
    double stepLengthSecs = beatLengthSecs * rateDivision * 4.0; // 0.25 * 4 = 1 beat length (if 1/4 note)
    
    for (const auto meta : midiMessages)
    {
        auto msg = meta.getMessage();
        if (msg.isNoteOn())
            activeNotes.insert(msg.getNoteNumber());
        else if (msg.isNoteOff())
            activeNotes.erase(msg.getNoteNumber());
    }
    
    midiMessages.clear(); // Overwrite with our sequenced notes
    
    if (activeNotes.empty())
    {
        currentNoteIndex = 0;
        timeSinceLastNote = stepLengthSecs; 
        
        if (lastNotePlayed != -1)
        {
            midiMessages.addEvent(juce::MidiMessage::noteOff(1, lastNotePlayed), 0);
            lastNotePlayed = -1;
        }
        return;
    }
    
    int currentSample = 0;
    while (currentSample < numSamples)
    {
        double timeToNextStep = stepLengthSecs - timeSinceLastNote;
        int samplesToNextStep = (int)(timeToNextStep * sr);
        
        if (currentSample + samplesToNextStep < numSamples && samplesToNextStep >= 0)
        {
            currentSample += samplesToNextStep;
            timeSinceLastNote = 0.0;
            
            if (lastNotePlayed != -1)
            {
                midiMessages.addEvent(juce::MidiMessage::noteOff(1, lastNotePlayed), currentSample);
                lastNotePlayed = -1;
            }
            
            std::vector<int> notes(activeNotes.begin(), activeNotes.end());
            if (currentNoteIndex >= notes.size()) currentNoteIndex = 0;
            
            lastNotePlayed = notes[currentNoteIndex];
            midiMessages.addEvent(juce::MidiMessage::noteOn(1, lastNotePlayed, (juce::uint8)100), currentSample);
            
            currentNoteIndex++;
        }
        else
        {
            timeSinceLastNote += (double)(numSamples - currentSample) / sr;
            break;
        }
    }
}
