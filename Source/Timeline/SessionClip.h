#pragma once
#include <JuceHeader.h>
#include "../Timeline/Clip.h"

// Non-linear session view clip encapsulation
class SessionClip
{
public:
    SessionClip() = default;
    ~SessionClip() = default;

    enum class LaunchQuantization {
        None, Bar, Half, Quarter, Eighth, Sixteenth
    };

    enum class LaunchMode {
        Trigger, Gate, Toggle, Repeat
    };

    std::unique_ptr<Clip> internalClip; // The underlying Audio/Midi Clip
    
    LaunchQuantization quantize { LaunchQuantization::Bar };
    LaunchMode launchMode { LaunchMode::Trigger };
    
    juce::Colour colour { juce::Colours::grey };
    juce::String name { "Clip" };
    
    bool isPlaying { false };
    bool isQueued { false };
    double playheadPosition { 0.0 };

    void trigger() {
        isQueued = true;
    }
    
    void stop() {
        isPlaying = false;
        isQueued = false;
        playheadPosition = 0.0;
    }
};
