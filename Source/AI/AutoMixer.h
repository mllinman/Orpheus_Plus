#pragma once
#include <JuceHeader.h>
#include "../Audio/AudioEngine.h"
#include "TrackElementClassifier.h"

/**
 * AutoMixer — enhanced with instrument-aware intelligence.
 *
 * Uses TrackElementClassifier to detect each track's instrument type,
 * then applies professionally-calibrated target volumes and stereo
 * positions based on the detected role in the mix.
 */
class AutoMixer
{
public:
    AutoMixer() = default;
    ~AutoMixer() = default;

    /**
     * Analyzes all tracks in the AudioEngine to calculate ideal fader/pan values.
     * Now uses instrument classification for intelligent targeting instead of
     * flat mock values.
     */
    void analyzeSession(AudioEngine* engine);

    /**
     * Applies the calculated balancing values to the TrackFaderProcessors.
     */
    void applyBalancing(AudioEngine* engine);

    /**
     * Returns the classification results from the last analyzeSession() call.
     * Useful for UI display.
     */
    const std::vector<TrackElementClassifier::ClassificationResult>& getLastClassifications() const
    {
        return lastClassifications;
    }

    struct TargetLevels {
        int   trackIndex;
        float targetVolumeDb;
        float targetPan;
        TrackElementClassifier::InstrumentType instrumentType
            = TrackElementClassifier::InstrumentType::Other;
        juce::String instrumentLabel;
    };

    const std::vector<TargetLevels>& getCalculatedTargets() const { return calculatedTargets; }

private:
    std::vector<TargetLevels> calculatedTargets;
    std::vector<TrackElementClassifier::ClassificationResult> lastClassifications;
    TrackElementClassifier classifier;

    // Instrument-aware target level and pan lookup
    float getInstrumentTargetDb(TrackElementClassifier::InstrumentType type);
    float getInstrumentTargetPan(TrackElementClassifier::InstrumentType type, int trackIndex);
};
