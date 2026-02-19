#include "TransportController.h"

void TransportController::handleIncomingMidiMessage(juce::MidiInput*, const juce::MidiMessage& m)
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
