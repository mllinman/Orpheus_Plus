#pragma once
#include <JuceHeader.h>
#include "../Audio/AudioEngine.h"

class MackieControlSurface : public juce::MidiInputCallback
{
public:
    MackieControlSurface(AudioEngine& engine);
    ~MackieControlSurface() override;

    void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) override;

private:
    void handleNoteOn(int note, int velocity);
    void handlePitchWheel(int channel, int value);

    AudioEngine& audioEngine;
    
    // Command mappings
    static constexpr int CMD_PLAY = 94;
    static constexpr int CMD_STOP = 93;
    static constexpr int CMD_RECORD = 95;
    static constexpr int CMD_REWIND = 91;
    static constexpr int CMD_FASTFORWARD = 92;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MackieControlSurface)
};
