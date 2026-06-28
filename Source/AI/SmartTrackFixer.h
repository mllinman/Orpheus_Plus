#pragma once
#include <JuceHeader.h>
#include "TrackElementClassifier.h"
#include "../Audio/AudioEngine.h"
#include <vector>
#include <map>

/**
 * SmartTrackFixer
 *
 * Applies instrument-specific correction profiles to tracks based on their
 * classified instrument type. Each instrument gets tailored EQ, compression,
 * stereo placement, and transient shaping to make the track sit perfectly
 * in a professional mix.
 *
 * Also performs cross-track frequency conflict analysis to identify masking
 * issues and automatically apply spectral carving suggestions.
 */
class SmartTrackFixer
{
public:
    //──────────────────────────────────────────────────────────────────────
    // Fix Profile — all the corrections for a single track
    //──────────────────────────────────────────────────────────────────────
    struct EQBand
    {
        enum class Type { HighPass, LowShelf, Parametric, HighShelf, LowPass };
        Type   type       = Type::Parametric;
        float  frequency  = 1000.0f;   // Hz
        float  gainDb     = 0.0f;      // dB
        float  q          = 1.0f;
        bool   active     = false;
    };

    struct CompressionSettings
    {
        float ratio     = 2.0f;
        float threshDb  = -18.0f;
        float attackMs  = 10.0f;
        float releaseMs = 100.0f;
        float kneeDb    = 6.0f;
        float makeupDb  = 0.0f;
        bool  active    = false;
    };

    struct TransientSettings
    {
        float attackAmount  = 0.0f;   // -100 to +100 (negative = soften, positive = sharpen)
        float sustainAmount = 0.0f;   // -100 to +100
        bool  active        = false;
    };

    struct StereoSettings
    {
        float width      = 1.0f;      // 0.0 = mono, 1.0 = normal, 2.0 = wide
        float pan         = 0.0f;      // -1.0 to +1.0
        float monoBelow   = 0.0f;      // Hz — make mono below this freq (e.g. 200 Hz for bass)
        bool  active      = false;
    };

    struct FixProfile
    {
        TrackElementClassifier::InstrumentType instrumentType
            = TrackElementClassifier::InstrumentType::Other;
        juce::String instrumentLabel;

        // Up to 6 EQ bands
        static constexpr int MAX_EQ_BANDS = 6;
        std::array<EQBand, MAX_EQ_BANDS> eqBands;
        int numActiveEQBands = 0;

        CompressionSettings compression;
        TransientSettings   transients;
        StereoSettings      stereo;

        float suggestedGainDb = 0.0f;   // Final gain adjustment
        float overallIntensity = 1.0f;  // 0.0 = bypass all, 1.0 = full correction

        juce::String summary;           // Human-readable description of changes
    };

    //──────────────────────────────────────────────────────────────────────
    // Frequency Conflict (masking between tracks)
    //──────────────────────────────────────────────────────────────────────
    struct FrequencyConflict
    {
        int trackIndexA = -1;
        int trackIndexB = -1;
        juce::String trackNameA;
        juce::String trackNameB;
        TrackElementClassifier::InstrumentType typeA
            = TrackElementClassifier::InstrumentType::Other;
        TrackElementClassifier::InstrumentType typeB
            = TrackElementClassifier::InstrumentType::Other;
        float conflictFreqHz   = 0.0f;   // Center of conflict zone
        float conflictBandWidth = 0.0f;  // Hz
        float severity         = 0.0f;   // 0.0–1.0
        juce::String suggestion;         // e.g. "Cut 200–400 Hz on Bass to clear room for Kick"
    };

    //──────────────────────────────────────────────────────────────────────
    // API
    //──────────────────────────────────────────────────────────────────────
    SmartTrackFixer() = default;
    ~SmartTrackFixer() = default;

    /**
     * Generate a FixProfile for a single track based on its classification.
     * Does NOT apply the fix — call applyFixToTrack() for that.
     */
    FixProfile generateFixProfile(
        const TrackElementClassifier::ClassificationResult& classification,
        float intensity = 1.0f);

    /**
     * Apply a FixProfile to a track in the AudioEngine.
     * This sets EQ (via insert FX), volume, pan, and sweetener on the track.
     */
    void applyFixToTrack(AudioEngine& engine, int trackIndex,
                         const FixProfile& profile);

    /**
     * Analyze all tracks for frequency masking conflicts.
     * Requires the classification results for each track (same order as engine tracks).
     */
    std::vector<FrequencyConflict> detectConflicts(
        const std::vector<TrackElementClassifier::ClassificationResult>& classifications,
        AudioEngine& engine);

    /**
     * Full pipeline: classify all tracks → generate profiles → apply fixes.
     * Returns the generated profiles and any detected conflicts.
     */
    struct FullFixResult
    {
        std::vector<TrackElementClassifier::ClassificationResult> classifications;
        std::vector<FixProfile> profiles;
        std::vector<FrequencyConflict> conflicts;
    };

    FullFixResult analyzeAndFixAll(AudioEngine& engine, float intensity = 1.0f);

private:
    //── Profile generators per instrument type ──────────────────────────
    FixProfile buildVocalProfile(const TrackElementClassifier::ClassificationResult& c, float intensity);
    FixProfile buildDrumProfile(const TrackElementClassifier::ClassificationResult& c, float intensity);
    FixProfile buildBassProfile(const TrackElementClassifier::ClassificationResult& c, float intensity);
    FixProfile buildElectricGuitarProfile(const TrackElementClassifier::ClassificationResult& c, float intensity);
    FixProfile buildAcousticGuitarProfile(const TrackElementClassifier::ClassificationResult& c, float intensity);
    FixProfile buildSynthProfile(const TrackElementClassifier::ClassificationResult& c, float intensity);
    FixProfile buildPianoProfile(const TrackElementClassifier::ClassificationResult& c, float intensity);
    FixProfile buildPercussionProfile(const TrackElementClassifier::ClassificationResult& c, float intensity);
    FixProfile buildDefaultProfile(const TrackElementClassifier::ClassificationResult& c, float intensity);

    //── Conflict detection helpers ──────────────────────────────────────
    struct InstrumentFreqRange
    {
        float primaryLow  = 0.0f;
        float primaryHigh = 0.0f;
    };
    InstrumentFreqRange getFrequencyRange(TrackElementClassifier::InstrumentType t);

    TrackElementClassifier classifier;
};
