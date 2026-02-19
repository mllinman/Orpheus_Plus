#pragma once
#include <JuceHeader.h>
#include "../Audio/AudioEngine.h"

// TransportController manages keyboard shortcuts and external MIDI transport
class TransportController : public juce::MidiInputCallback
{
public:
    explicit TransportController(AudioEngine& engine) : audioEngine(engine) {}

    void handleIncomingMidiMessage(juce::MidiInput*, const juce::MidiMessage& m) override;

private:
    AudioEngine& audioEngine;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportController)
};
