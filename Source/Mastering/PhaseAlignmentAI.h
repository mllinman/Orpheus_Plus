#pragma once
#include <JuceHeader.h>
#include <vector>

// Represents an offline/online phase alignment tool
class PhaseAlignmentAI
{
public:
    PhaseAlignmentAI();
    ~PhaseAlignmentAI() = default;

    // Aligns a target buffer to a reference buffer by finding the maximum cross-correlation
    // Returns the calculated sample offset (positive means target is delayed relative to reference)
    // Applies the offset to the target buffer in-place.
    static int alignBufferToReference(juce::AudioBuffer<float>& target, 
                                      const juce::AudioBuffer<float>& reference, 
                                      int maxOffsetSamples);

    // Global phase alignment for a multi-track setup. 
    // Assumes track 0 is the reference.
    static void alignAllTracks(std::vector<juce::AudioBuffer<float>*>& tracks, 
                               int maxOffsetSamples);
};
