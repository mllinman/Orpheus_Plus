#include "MidiLearnManager.h"
#include "AudioEngine.h"

MidiLearnManager::MidiLearnManager(AudioEngine& engine) : audioEngine(engine) {}

void MidiLearnManager::setLearnMode(bool active, ParameterTarget target)
{
    learnMode = active;
    waitingTarget = target;
}

void MidiLearnManager::handleIncomingMidi(const juce::MidiMessage& message)
{
    if (message.isController())
    {
        int ccNum = message.getControllerNumber();
        float val = message.getControllerValue() / 127.0f;

        if (learnMode)
        {
            bindCC(ccNum, waitingTarget);
            setLearnMode(false);
            return;
        }

        auto it = ccToTarget.find(ccNum);
        if (it != ccToTarget.end())
        {
            auto& target = it->second;
            if (target.type == ParameterTarget::Type::TrackVolume)
            {
                audioEngine.setTrackVolume(target.trackIndex, val * 1.5f); 
            }
            else if (target.type == ParameterTarget::Type::TrackPan)
            {
                audioEngine.setTrackPan(target.trackIndex, val * 2.0f - 1.0f);
            }
            else if (target.type == ParameterTarget::Type::TrackSweet)
            {
                audioEngine.setTrackSweetener(target.trackIndex, val);
            }
            else if (target.type == ParameterTarget::Type::MasterVolume)
            {
                audioEngine.setMasterVolume(val);
            }
        }
    }
}

void MidiLearnManager::bindCC(int ccNumber, ParameterTarget target)
{
    ccToTarget[ccNumber] = target;
}

void MidiLearnManager::unbindCC(int ccNumber)
{
    ccToTarget.erase(ccNumber);
}
