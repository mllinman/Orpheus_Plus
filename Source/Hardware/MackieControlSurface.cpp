#include "MackieControlSurface.h"

MackieControlSurface::MackieControlSurface(AudioEngine& engine)
    : audioEngine(engine)
{
    // Normally we'd scan available MIDI ports and connect
}

MackieControlSurface::~MackieControlSurface()
{
}

void MackieControlSurface::handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message)
{
    juce::ignoreUnused(source);

    if (message.isNoteOn())
    {
        handleNoteOn(message.getNoteNumber(), message.getVelocity());
    }
    else if (message.isPitchWheel())
    {
        handlePitchWheel(message.getChannel(), message.getPitchWheelValue());
    }
}

void MackieControlSurface::handleNoteOn(int note, int velocity)
{
    if (velocity == 0) return; // Note off

    switch (note)
    {
    case CMD_PLAY:
        audioEngine.setPlaying(true);
        break;
    case CMD_STOP:
        audioEngine.setPlaying(false);
        break;
    case CMD_RECORD:
        audioEngine.setRecording(true);
        break;
    default:
        break;
    }
}

void MackieControlSurface::handlePitchWheel(int channel, int value)
{
    // MCU uses pitch wheel messages (channels 1-8) for 14-bit high res fader movements
    int trackIndex = channel - 1;
    if (trackIndex >= 0 && trackIndex < 8)
    {
        float normalizedGain = value / 16383.0f;
        // Map to decibels and send to audio engine
        audioEngine.setTrackVolume(trackIndex, juce::Decibels::gainToDecibels(normalizedGain, -100.0f));
    }
}
