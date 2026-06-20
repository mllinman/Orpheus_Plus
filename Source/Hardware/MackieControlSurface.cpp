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
    else if (message.isController())
    {
        handleControlChange(message.getControllerNumber(), message.getControllerValue());
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
    case CMD_BANK_LEFT:
        if (currentBank > 0) currentBank--;
        break;
    case CMD_BANK_RIGHT:
        currentBank++;
        break;
    default:
        // Check Mute (16-23)
        if (note >= 16 && note <= 23) {
            int trackIdx = (note - 16) + (currentBank * 8);
            // Toggle mute
            bool isMuted = audioEngine.getTrack(trackIdx)->isMuted();
            audioEngine.getTrack(trackIdx)->setMute(!isMuted);
            updateMute(trackIdx, !isMuted);
        }
        // Check Solo (8-15)
        else if (note >= 8 && note <= 15) {
            int trackIdx = (note - 8) + (currentBank * 8);
            bool isSolo = audioEngine.getTrack(trackIdx)->isSolo();
            audioEngine.getTrack(trackIdx)->setSolo(!isSolo);
            updateSolo(trackIdx, !isSolo);
        }
        break;
    }
}

void MackieControlSurface::handlePitchWheel(int channel, int value)
{
    // MCU uses pitch wheel messages (channels 1-8) for 14-bit high res fader movements
    int trackIndex = (channel - 1) + (currentBank * 8);
    if (trackIndex >= 0)
    {
        float normalizedGain = value / 16383.0f;
        // Map to decibels and send to audio engine
        audioEngine.setTrackVolume(trackIndex, juce::Decibels::gainToDecibels(normalizedGain, -100.0f));
    }
}

void MackieControlSurface::handleControlChange(int controller, int value)
{
    // Pan is usually CC 16-23 on V-Pots
    if (controller >= 16 && controller <= 23)
    {
        int trackIndex = (controller - 16) + (currentBank * 8);
        // value is typically endless encoder (1-63 right, 65-127 left)
        // Convert to absolute pan value -1.0 to 1.0
        float currentPan = audioEngine.getTrack(trackIndex)->getPan();
        float delta = 0.0f;
        if (value >= 1 && value <= 63) delta = value * 0.05f;
        else if (value >= 65 && value <= 127) delta = -(value - 64) * 0.05f;
        
        float newPan = juce::jlimit(-1.0f, 1.0f, currentPan + delta);
        audioEngine.getTrack(trackIndex)->setPan(newPan);
    }
}

void MackieControlSurface::updateFader(int trackIndex, float volumeDB)
{
    if (midiOutput == nullptr) return;
    int faderIndex = trackIndex - (currentBank * 8);
    if (faderIndex >= 0 && faderIndex < 8)
    {
        float normalizedGain = juce::Decibels::decibelsToGain(volumeDB, -100.0f);
        int value = (int)(normalizedGain * 16383.0f);
        value = juce::jlimit(0, 16383, value);
        midiOutput->sendMessageNow(juce::MidiMessage::pitchWheel(faderIndex + 1, value));
    }
}

void MackieControlSurface::updatePan(int trackIndex, float pan)
{
    if (midiOutput == nullptr) return;
    int encoderIndex = trackIndex - (currentBank * 8);
    if (encoderIndex >= 0 && encoderIndex < 8)
    {
        // For MCU LED rings, typically CC 48-55
        int ledValue = (int)((pan + 1.0f) * 0.5f * 11.0f) + 1; // 1 to 11
        midiOutput->sendMessageNow(juce::MidiMessage::controllerEvent(1, 48 + encoderIndex, ledValue));
    }
}

void MackieControlSurface::updateMute(int trackIndex, bool isMute)
{
    if (midiOutput == nullptr) return;
    int btnIndex = trackIndex - (currentBank * 8);
    if (btnIndex >= 0 && btnIndex < 8)
    {
        midiOutput->sendMessageNow(juce::MidiMessage::noteOn(1, 16 + btnIndex, isMute ? 127.0f : 0.0f));
    }
}

void MackieControlSurface::updateSolo(int trackIndex, bool isSolo)
{
    if (midiOutput == nullptr) return;
    int btnIndex = trackIndex - (currentBank * 8);
    if (btnIndex >= 0 && btnIndex < 8)
    {
        midiOutput->sendMessageNow(juce::MidiMessage::noteOn(1, 8 + btnIndex, isSolo ? 127.0f : 0.0f));
    }
}
