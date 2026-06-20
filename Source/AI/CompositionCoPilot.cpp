#include "CompositionCoPilot.h"
#include <random>

std::unique_ptr<MidiClip> CompositionCoPilot::generateProgression(const juce::String& genre, const juce::String& style, int numBars, double bpm)
{
    // A bar duration in seconds
    double barDuration = (60.0 / bpm) * 4.0;
    auto clip = std::make_unique<MidiClip>(0.0, barDuration * numBars);
    
    // Simplistic heuristic to generate a progression
    // In a full implementation, an ONNX model would be called here.
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 5); // Pick a diatonic chord

    int rootNote = 60; // C4
    
    // Simple 4-bar loop patterns for testing
    std::vector<juce::String> chordTypes = { "Maj", "Min", "Min", "Maj", "Maj", "Min" };
    std::vector<int> offsets = { 0, 2, 4, 5, 7, 9 };

    for (int i = 0; i < numBars; ++i)
    {
        int chordIdx = dis(gen);
        int chordRoot = rootNote + offsets[chordIdx];
        addChordToClip(*clip, chordRoot, chordTypes[chordIdx], i * barDuration, barDuration);
    }
    
    clip->name = genre + " " + style + " Progression";
    return clip;
}

std::unique_ptr<MidiClip> CompositionCoPilot::autocompleteMelody(const MidiClip& seedMelody, int additionalBars, double bpm)
{
    double barDuration = (60.0 / bpm) * 4.0;
    double newDuration = seedMelody.duration + (barDuration * additionalBars);
    
    auto result = std::make_unique<MidiClip>(seedMelody.startTime, newDuration);
    result->name = seedMelody.name + " (Autocompleted)";
    
    // Copy original sequence
    result->midiData.addSequence(seedMelody.midiData, 0.0);
    
    // Synthesize continuation based on seed (heuristic for now)
    // Here an ONNX melody generation model would parse the sequence and generate tokens
    
    // Generate some random notes at the end
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(60, 72);
    
    double currentTime = seedMelody.duration;
    while (currentTime < newDuration)
    {
        int noteNum = dis(gen);
        auto noteOn = juce::MidiMessage::noteOn(1, noteNum, 0.8f);
        auto noteOff = juce::MidiMessage::noteOff(1, noteNum);
        
        result->midiData.addEvent(noteOn, currentTime * result->ppq);
        result->midiData.addEvent(noteOff, (currentTime + 0.25) * result->ppq);
        
        currentTime += 0.25; // 16th notes or so depending on BPM, simplistic
    }
    
    result->midiData.updateMatchedPairs();
    return result;
}

void CompositionCoPilot::extractRhythmAndApply(const AudioClip& grooveSource, MidiClip& targetMidi)
{
    // Rhythmic Style Transfer
    // Extract transients from AudioClip
    // For now, this is a placeholder for spectral transient detection
    
    std::vector<double> detectedOnsets;
    
    // Dummy onsets based on a 16th note grid with some "swing"
    double swing = 0.05; // 50ms swing
    for (double t = 0; t < grooveSource.duration; t += 0.25)
    {
        double offset = (std::fmod(t, 0.5) > 0.1) ? swing : 0.0;
        detectedOnsets.push_back(t + offset);
    }
    
    // Apply groove to MIDI
    // Snap MIDI notes to nearest onset
    for (int i = 0; i < targetMidi.midiData.getNumEvents(); ++i)
    {
        auto ev = targetMidi.midiData.getEventPointer(i);
        if (ev->message.isNoteOn())
        {
            double eventTimeSecs = ev->message.getTimeStamp() / targetMidi.ppq;
            
            // Find closest onset
            double closest = eventTimeSecs;
            double minDist = 9999.0;
            for (double onset : detectedOnsets)
            {
                if (std::abs(onset - eventTimeSecs) < minDist)
                {
                    minDist = std::abs(onset - eventTimeSecs);
                    closest = onset;
                }
            }
            
            // Move note
            ev->message.setTimeStamp(closest * targetMidi.ppq);
        }
    }
    
    targetMidi.midiData.updateMatchedPairs();
}

void CompositionCoPilot::addChordToClip(MidiClip& clip, int rootNote, const juce::String& type, double startTime, double duration)
{
    std::vector<int> intervals;
    if (type == "Maj") intervals = { 0, 4, 7 };
    else if (type == "Min") intervals = { 0, 3, 7 };
    else if (type == "Dim") intervals = { 0, 3, 6 };
    else intervals = { 0, 4, 7 }; // Default Maj
    
    for (int interval : intervals)
    {
        auto noteOn = juce::MidiMessage::noteOn(1, rootNote + interval, 0.8f);
        auto noteOff = juce::MidiMessage::noteOff(1, rootNote + interval);
        
        // Convert seconds to PPQ ticks
        double startTicks = startTime * clip.ppq;
        double endTicks = (startTime + duration - 0.1) * clip.ppq; // slight gap
        
        clip.midiData.addEvent(noteOn, startTicks);
        clip.midiData.addEvent(noteOff, endTicks);
    }
    clip.midiData.updateMatchedPairs();
}

std::vector<int> CompositionCoPilot::getScaleNotes(int rootNote, const juce::String& scaleType)
{
    std::vector<int> notes;
    std::vector<int> intervals;
    if (scaleType == "Major") intervals = { 0, 2, 4, 5, 7, 9, 11 };
    else if (scaleType == "Minor") intervals = { 0, 2, 3, 5, 7, 8, 10 };
    else intervals = { 0, 2, 4, 5, 7, 9, 11 };
    
    for (int i : intervals) notes.push_back(rootNote + i);
    return notes;
}
