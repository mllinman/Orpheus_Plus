#pragma once
#include <JuceHeader.h>
#include "../Audio/AudioEngine.h"

class AutoMixer
{
public:
    AutoMixer() = default;
    ~AutoMixer() = default;

    /**
     * Analyzes all tracks in the AudioEngine to calculate ideal fader/pan values.
     * Generates a "Pink Noise" (-3dB/octave) balance by assessing spectral centroids and RMS levels.
     */
    void analyzeSession(AudioEngine* engine);

    /**
     * Applies the calculated balancing values to the TrackFaderProcessors.
     */
    void applyBalancing(AudioEngine* engine);

private:
    struct TargetLevels {
        int trackIndex;
        float targetVolumeDb;
        float targetPan;
    };

    std::vector<TargetLevels> calculatedTargets;
};
