#pragma once
#include <JuceHeader.h>
#include "../Audio/AudioEngine.h"

// TransportController manages keyboard shortcuts and external MIDI transport
class TransportController : public juce::MidiInputCallback
{
public:
    explicit TransportController(AudioEngine& engine) : audioEngine(engine) {}

    void handleIncomingMidiMessage(juce::MidiInput*, const juce::MidiMessage& m) override
    {
        // MMC transport control
        if (m.isMidiMachineControlMessage())
        {
            switch (m.getMidiMachineControlCommand())
            {
                case juce::MidiMessage::mmc_play:  audioEngine.play();  break;
                case juce::MidiMessage::mmc_stop:  audioEngine.stop();  break;
                case juce::MidiMessage::mmc_recordStart: audioEngine.toggleRecord(); break;
                default: break;
            }
        }
    }

private:
    AudioEngine& audioEngine;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportController)
};
