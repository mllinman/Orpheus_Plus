#pragma once
#include <JuceHeader.h>
#include <map>

struct ParameterTarget
{
    enum class Type {
        TrackVolume,
        TrackPan,
        TrackSweet,
        MasterVolume
    };

    Type type;
    int trackIndex = -1; // -1 for Master

    bool operator<(const ParameterTarget& other) const {
        if (type != other.type) return (int)type < (int)other.type;
        return trackIndex < other.trackIndex;
    }
};

class AudioEngine;

class MidiLearnManager
{
public:
    MidiLearnManager(AudioEngine& engine);

    void setLearnMode(bool active, ParameterTarget target = {});
    bool isLearnModeActive() const { return learnMode; }

    void handleIncomingMidi(const juce::MidiMessage& message);
    
    void bindCC(int ccNumber, ParameterTarget target);
    void unbindCC(int ccNumber);

private:
    AudioEngine& audioEngine;
    bool learnMode = false;
    ParameterTarget waitingTarget;

    std::map<int, ParameterTarget> ccToTarget;
};
