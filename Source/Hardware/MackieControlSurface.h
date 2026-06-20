#pragma once
#include <JuceHeader.h>
#include "../Audio/AudioEngine.h"

class MackieControlSurface : public juce::MidiInputCallback
{
public:
    MackieControlSurface(AudioEngine& engine);
    ~MackieControlSurface() override;

    void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) override;

    // 2-way feedback methods
    void setMidiOutput(juce::MidiOutput* out) { midiOutput = out; }
    void updateFader(int trackIndex, float volumeDB);
    void updatePan(int trackIndex, float pan);
    void updateMute(int trackIndex, bool isMute);
    void updateSolo(int trackIndex, bool isSolo);

private:
    void handleNoteOn(int note, int velocity);
    void handlePitchWheel(int channel, int value);
    void handleControlChange(int controller, int value);

    AudioEngine& audioEngine;
    juce::MidiOutput* midiOutput = nullptr;
    int currentBank = 0; // Each bank is 8 tracks
    
    // Command mappings
    static constexpr int CMD_PLAY = 94;
    static constexpr int CMD_STOP = 93;
    static constexpr int CMD_RECORD = 95;
    static constexpr int CMD_REWIND = 91;
    static constexpr int CMD_FASTFORWARD = 92;
    static constexpr int CMD_BANK_LEFT = 46;
    static constexpr int CMD_BANK_RIGHT = 47;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MackieControlSurface)
};
