#include "AutoMixer.h"

void AutoMixer::analyzeSession(AudioEngine* engine)
{
    calculatedTargets.clear();

    if (engine == nullptr)
        return;

    int numTracks = engine->getNumTracks();

    // MOCK: Iterate over tracks, extract feature metadata (RMS, crest factor, spectral centroid)
    // and compute a pink noise balance (-3dB/Octave) offset.
    // In a full implementation, we would queue an offline render pass of the clips or read their cached metadata.

    for (int i = 0; i < numTracks; ++i)
    {
        TargetLevels target;
        target.trackIndex = i;
        
        // Mock targets: 
        // We simulate that the AI wants to bring everything to -12dB FS RMS on average.
        // We'll apply random pan values to simulate a widened mix.
        target.targetVolumeDb = -6.0f; // -6dB is a common safe starting point
        
        // Random spread for panning based on index
        target.targetPan = (i % 2 == 0) ? -0.2f : 0.2f; 
        if (i == 0) target.targetPan = 0.0f; // Track 0 (usually kick/bass) stays centered

        calculatedTargets.push_back(target);
    }
}

void AutoMixer::applyBalancing(AudioEngine* engine)
{
    if (engine == nullptr)
        return;

    for (const auto& target : calculatedTargets)
    {
        if (auto* track = engine->getTrack(target.trackIndex))
        {
            // Convert target dB to linear gain
            float linearGain = juce::Decibels::decibelsToGain(target.targetVolumeDb);
            engine->setTrackVolume(target.trackIndex, linearGain);
            engine->setTrackPan(target.trackIndex, target.targetPan);
        }
    }
}
