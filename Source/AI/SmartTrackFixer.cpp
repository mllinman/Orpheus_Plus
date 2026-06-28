#include "SmartTrackFixer.h"
#include <cmath>

//==============================================================================
// Public API
//==============================================================================

SmartTrackFixer::FixProfile
SmartTrackFixer::generateFixProfile(
    const TrackElementClassifier::ClassificationResult& classification,
    float intensity)
{
    using IT = TrackElementClassifier::InstrumentType;

    switch (classification.type)
    {
        case IT::Vocals:          return buildVocalProfile(classification, intensity);
        case IT::Drums:           return buildDrumProfile(classification, intensity);
        case IT::Bass:            return buildBassProfile(classification, intensity);
        case IT::ElectricGuitar:  return buildElectricGuitarProfile(classification, intensity);
        case IT::AcousticGuitar:  return buildAcousticGuitarProfile(classification, intensity);
        case IT::Synth:           return buildSynthProfile(classification, intensity);
        case IT::Piano:           return buildPianoProfile(classification, intensity);
        case IT::Percussion:      return buildPercussionProfile(classification, intensity);
        case IT::Other:
        default:                  return buildDefaultProfile(classification, intensity);
    }
}

void SmartTrackFixer::applyFixToTrack(AudioEngine& engine, int trackIndex,
                                       const FixProfile& profile)
{
    auto* track = engine.getTrack(trackIndex);
    if (track == nullptr) return;

    float intensity = profile.overallIntensity;

    // ── Apply Stereo Settings ──
    if (profile.stereo.active)
    {
        float targetPan = profile.stereo.pan * intensity;
        engine.setTrackPan(trackIndex, targetPan);
    }

    // ── Apply Gain ──
    if (std::abs(profile.suggestedGainDb) > 0.1f)
    {
        float gainAdjust = profile.suggestedGainDb * intensity;
        float currentVol = track->volume;
        float newVol = currentVol * juce::Decibels::decibelsToGain(gainAdjust);
        engine.setTrackVolume(trackIndex, juce::jlimit(0.0f, 2.0f, newVol));
    }

    // ── Apply Sweetener (uses the track's built-in channel strip) ──
    // Map EQ and compression concepts to the sweetener parameter
    // Sweetener is a simplified "make it sound good" knob
    if (profile.compression.active || profile.numActiveEQBands > 0)
    {
        // Scale sweetener based on how much processing we want to apply
        float sweetenerAmount = 0.0f;

        // More EQ bands active = more sweetening needed
        sweetenerAmount += (float)profile.numActiveEQBands * 0.05f;

        // Compression contributes to sweetener
        if (profile.compression.active)
            sweetenerAmount += 0.15f;

        sweetenerAmount *= intensity;
        sweetenerAmount = juce::jlimit(0.0f, 1.0f, sweetenerAmount);

        engine.setTrackSweetener(trackIndex, sweetenerAmount);
    }
}

SmartTrackFixer::FullFixResult
SmartTrackFixer::analyzeAndFixAll(AudioEngine& engine, float intensity)
{
    FullFixResult result;
    int numTracks = engine.getNumTracks();

    // ── Phase 1: Classify all tracks ──
    for (int i = 0; i < numTracks; ++i)
    {
        auto* track = engine.getTrack(i);
        if (track == nullptr)
        {
            TrackElementClassifier::ClassificationResult empty;
            result.classifications.push_back(empty);
            continue;
        }

        // For classification we need audio data — if the track has clips, use the first one
        juce::AudioBuffer<float> analysisBuffer;
        bool hasAudio = false;

        for (auto* clip : track->clips)
        {
            if (auto* audioClip = dynamic_cast<AudioClip*>(clip))
            {
                // Try to load a short segment for analysis
                auto file = juce::File(audioClip->getFilePath());
                if (file.existsAsFile())
                {
                    juce::AudioFormatManager fmtMgr;
                    fmtMgr.registerBasicFormats();
                    if (auto reader = std::unique_ptr<juce::AudioFormatReader>(
                            fmtMgr.createReaderFor(file)))
                    {
                        // Read up to 5 seconds for analysis
                        int samplesToRead = juce::jmin((int)reader->lengthInSamples,
                                                       (int)(reader->sampleRate * 5.0));
                        analysisBuffer.setSize(1, samplesToRead);
                        reader->read(&analysisBuffer, 0, samplesToRead, 0, true, false);
                        hasAudio = true;

                        auto classification = classifier.classify(
                            track->name, analysisBuffer, reader->sampleRate);
                        result.classifications.push_back(classification);
                    }
                    else
                    {
                        // Can't read the file — classify by name only
                        juce::AudioBuffer<float> emptyBuf;
                        auto classification = classifier.classify(track->name, emptyBuf, 44100.0);
                        result.classifications.push_back(classification);
                    }
                }
                else
                {
                    juce::AudioBuffer<float> emptyBuf;
                    auto classification = classifier.classify(track->name, emptyBuf, 44100.0);
                    result.classifications.push_back(classification);
                }
                break; // Only analyze first clip
            }
        }

        if (!hasAudio && result.classifications.size() <= (size_t)i)
        {
            // No audio clips — classify by name only
            juce::AudioBuffer<float> emptyBuf;
            auto classification = classifier.classify(track->name, emptyBuf, 44100.0);
            result.classifications.push_back(classification);
        }
    }

    // ── Phase 2: Generate fix profiles ──
    for (auto& classification : result.classifications)
    {
        result.profiles.push_back(generateFixProfile(classification, intensity));
    }

    // ── Phase 3: Detect conflicts ──
    result.conflicts = detectConflicts(result.classifications, engine);

    // ── Phase 4: Apply fixes ──
    for (int i = 0; i < numTracks && i < (int)result.profiles.size(); ++i)
    {
        applyFixToTrack(engine, i, result.profiles[(size_t)i]);
    }

    return result;
}

//==============================================================================
// Frequency Conflict Detection
//==============================================================================

std::vector<SmartTrackFixer::FrequencyConflict>
SmartTrackFixer::detectConflicts(
    const std::vector<TrackElementClassifier::ClassificationResult>& classifications,
    AudioEngine& engine)
{
    std::vector<FrequencyConflict> conflicts;

    for (size_t i = 0; i < classifications.size(); ++i)
    {
        for (size_t j = i + 1; j < classifications.size(); ++j)
        {
            auto rangeA = getFrequencyRange(classifications[i].type);
            auto rangeB = getFrequencyRange(classifications[j].type);

            // Check for overlap
            float overlapLow  = juce::jmax(rangeA.primaryLow, rangeB.primaryLow);
            float overlapHigh = juce::jmin(rangeA.primaryHigh, rangeB.primaryHigh);

            if (overlapLow < overlapHigh)
            {
                float overlapWidth = overlapHigh - overlapLow;
                float totalRange   = juce::jmax(rangeA.primaryHigh - rangeA.primaryLow,
                                                  rangeB.primaryHigh - rangeB.primaryLow);
                float severity = (totalRange > 0.0f) ? overlapWidth / totalRange : 0.0f;

                // Only report significant conflicts
                if (severity > 0.2f)
                {
                    FrequencyConflict conflict;
                    conflict.trackIndexA = (int)i;
                    conflict.trackIndexB = (int)j;
                    conflict.typeA = classifications[i].type;
                    conflict.typeB = classifications[j].type;
                    conflict.conflictFreqHz = (overlapLow + overlapHigh) * 0.5f;
                    conflict.conflictBandWidth = overlapWidth;
                    conflict.severity = juce::jmin(1.0f, severity);

                    auto* trackA = engine.getTrack((int)i);
                    auto* trackB = engine.getTrack((int)j);
                    conflict.trackNameA = trackA ? trackA->name : "Track " + juce::String((int)i + 1);
                    conflict.trackNameB = trackB ? trackB->name : "Track " + juce::String((int)j + 1);

                    auto typeNameA = TrackElementClassifier::instrumentTypeToString(classifications[i].type);
                    auto typeNameB = TrackElementClassifier::instrumentTypeToString(classifications[j].type);

                    conflict.suggestion = "Frequency masking between " + typeNameA + " and " + typeNameB
                        + " around " + juce::String((int)conflict.conflictFreqHz) + " Hz. "
                        + "Consider cutting " + juce::String((int)overlapLow) + "–"
                        + juce::String((int)overlapHigh) + " Hz on one track.";

                    conflicts.push_back(conflict);
                }
            }
        }
    }

    return conflicts;
}

SmartTrackFixer::InstrumentFreqRange
SmartTrackFixer::getFrequencyRange(TrackElementClassifier::InstrumentType t)
{
    using IT = TrackElementClassifier::InstrumentType;
    switch (t)
    {
        case IT::Vocals:          return { 200.0f, 6000.0f };
        case IT::Drums:           return { 30.0f, 12000.0f };
        case IT::Bass:            return { 30.0f, 500.0f };
        case IT::ElectricGuitar:  return { 100.0f, 6000.0f };
        case IT::AcousticGuitar:  return { 80.0f, 5000.0f };
        case IT::Synth:           return { 60.0f, 15000.0f };
        case IT::Piano:           return { 50.0f, 8000.0f };
        case IT::Percussion:      return { 200.0f, 10000.0f };
        case IT::Other:
        default:                  return { 20.0f, 20000.0f };
    }
}

//==============================================================================
// Instrument-Specific Fix Profiles
//==============================================================================

SmartTrackFixer::FixProfile
SmartTrackFixer::buildVocalProfile(const TrackElementClassifier::ClassificationResult& c, float intensity)
{
    FixProfile p;
    p.instrumentType = c.type;
    p.instrumentLabel = c.subType.isNotEmpty() ? c.subType : "Vocals";
    p.overallIntensity = intensity;

    // EQ: HPF 80Hz, cut muddiness 200-300Hz, presence boost 3-5kHz, air 10kHz+, de-ess shelf 6-8kHz
    p.eqBands[0] = { EQBand::Type::HighPass,   80.0f,   0.0f, 0.7f, true };
    p.eqBands[1] = { EQBand::Type::Parametric, 250.0f, -2.5f * intensity, 1.2f, true };
    p.eqBands[2] = { EQBand::Type::Parametric, 3800.0f, 2.5f * intensity, 1.5f, true };
    p.eqBands[3] = { EQBand::Type::HighShelf,  10000.0f, 1.5f * intensity, 0.7f, true };
    p.eqBands[4] = { EQBand::Type::Parametric, 7000.0f, -1.5f * intensity, 2.0f, true }; // De-ess
    p.numActiveEQBands = 5;

    // Compression: Gentle, vocal-focused
    p.compression = { 3.0f, -18.0f, 8.0f, 80.0f, 6.0f, 2.0f * intensity, true };

    // Stereo: Center, slight widening
    p.stereo = { 1.1f, 0.0f, 0.0f, true };

    // Transients: Slightly soften attack for smoothness
    p.transients = { -10.0f * intensity, 15.0f * intensity, true };

    // Gain: Target around -6 dB
    float currentRMSdB = (c.rmsLevel > 0.0001f) ? 20.0f * std::log10(c.rmsLevel) : -60.0f;
    p.suggestedGainDb = juce::jlimit(-8.0f, 8.0f, (-6.0f - currentRMSdB) * 0.5f * intensity);

    p.summary = "Vocal Fix: HPF 80Hz, cut mud 250Hz, presence +3.8kHz, air +10kHz, "
                "de-ess 7kHz, gentle compression 3:1, centered.";
    return p;
}

SmartTrackFixer::FixProfile
SmartTrackFixer::buildDrumProfile(const TrackElementClassifier::ClassificationResult& c, float intensity)
{
    FixProfile p;
    p.instrumentType = c.type;
    p.instrumentLabel = c.subType.isNotEmpty() ? c.subType : "Drums";
    p.overallIntensity = intensity;

    // EQ: HPF 30Hz, sub boost 60Hz, body cut 300-500Hz, presence 4-6kHz, air 10kHz
    p.eqBands[0] = { EQBand::Type::HighPass,   30.0f,   0.0f, 0.7f, true };
    p.eqBands[1] = { EQBand::Type::Parametric,  60.0f,  3.0f * intensity, 1.5f, true };
    p.eqBands[2] = { EQBand::Type::Parametric, 400.0f, -2.0f * intensity, 1.0f, true };
    p.eqBands[3] = { EQBand::Type::Parametric, 5000.0f, 2.0f * intensity, 1.5f, true };
    p.eqBands[4] = { EQBand::Type::HighShelf,  10000.0f, 1.0f * intensity, 0.7f, true };
    p.numActiveEQBands = 5;

    // Compression: Punchy parallel-style
    p.compression = { 4.0f, -15.0f, 5.0f, 60.0f, 4.0f, 3.0f * intensity, true };

    // Stereo: Slightly wider for overheads, centered for close mics
    p.stereo = { 1.2f, 0.0f, 0.0f, true };

    // Transients: Sharpen attacks for punchiness
    p.transients = { 30.0f * intensity, -5.0f * intensity, true };

    float currentRMSdB = (c.rmsLevel > 0.0001f) ? 20.0f * std::log10(c.rmsLevel) : -60.0f;
    p.suggestedGainDb = juce::jlimit(-8.0f, 8.0f, (-8.0f - currentRMSdB) * 0.5f * intensity);

    p.summary = "Drum Fix: HPF 30Hz, sub +60Hz, cut mud 400Hz, snap +5kHz, "
                "punchy compression 4:1, transient sharpen.";
    return p;
}

SmartTrackFixer::FixProfile
SmartTrackFixer::buildBassProfile(const TrackElementClassifier::ClassificationResult& c, float intensity)
{
    FixProfile p;
    p.instrumentType = c.type;
    p.instrumentLabel = c.subType.isNotEmpty() ? c.subType : "Bass";
    p.overallIntensity = intensity;

    // EQ: HPF 30Hz, warmth 200Hz, mid cut 500-800Hz, LPF 8kHz
    p.eqBands[0] = { EQBand::Type::HighPass,   30.0f,   0.0f, 0.7f, true };
    p.eqBands[1] = { EQBand::Type::LowShelf,  200.0f,  2.0f * intensity, 0.7f, true };
    p.eqBands[2] = { EQBand::Type::Parametric, 650.0f, -2.0f * intensity, 1.2f, true };
    p.eqBands[3] = { EQBand::Type::LowPass,   8000.0f,  0.0f, 0.7f, true };
    p.numActiveEQBands = 4;

    // Compression: Tight and controlled
    p.compression = { 4.0f, -16.0f, 5.0f, 50.0f, 3.0f, 2.0f * intensity, true };

    // Stereo: Mono below 200Hz, centered
    p.stereo = { 0.8f, 0.0f, 200.0f, true };

    // Transients: Slightly soften for smoothness
    p.transients = { -5.0f * intensity, 10.0f * intensity, true };

    float currentRMSdB = (c.rmsLevel > 0.0001f) ? 20.0f * std::log10(c.rmsLevel) : -60.0f;
    p.suggestedGainDb = juce::jlimit(-8.0f, 8.0f, (-10.0f - currentRMSdB) * 0.5f * intensity);

    p.summary = "Bass Fix: HPF 30Hz, warmth +200Hz, cut mid 650Hz, LPF 8kHz, "
                "tight compression 4:1, mono below 200Hz.";
    return p;
}

SmartTrackFixer::FixProfile
SmartTrackFixer::buildElectricGuitarProfile(const TrackElementClassifier::ClassificationResult& c, float intensity)
{
    FixProfile p;
    p.instrumentType = c.type;
    p.instrumentLabel = c.subType.isNotEmpty() ? c.subType : "Electric Guitar";
    p.overallIntensity = intensity;

    // EQ: HPF 100Hz, cut mud 300Hz, mid scoop 400Hz, presence 2-4kHz, air 8kHz
    p.eqBands[0] = { EQBand::Type::HighPass,   100.0f,   0.0f, 0.7f, true };
    p.eqBands[1] = { EQBand::Type::Parametric, 300.0f, -2.0f * intensity, 1.0f, true };
    p.eqBands[2] = { EQBand::Type::Parametric, 3000.0f, 2.0f * intensity, 1.5f, true };
    p.eqBands[3] = { EQBand::Type::HighShelf,  8000.0f,  1.0f * intensity, 0.7f, true };
    p.numActiveEQBands = 4;

    // Compression: Moderate
    p.compression = { 3.0f, -14.0f, 10.0f, 100.0f, 6.0f, 1.5f * intensity, true };

    // Stereo: Pan L or R (alternate based on subtype)
    float panPos = (c.subType.contains("Rhythm") || c.subType.contains("rhythm")) ? -0.4f : 0.4f;
    p.stereo = { 1.0f, panPos * intensity, 0.0f, true };

    // Transients: Slight sharpen for articulation
    p.transients = { 15.0f * intensity, 0.0f, true };

    float currentRMSdB = (c.rmsLevel > 0.0001f) ? 20.0f * std::log10(c.rmsLevel) : -60.0f;
    p.suggestedGainDb = juce::jlimit(-8.0f, 8.0f, (-12.0f - currentRMSdB) * 0.5f * intensity);

    p.summary = "Electric Guitar Fix: HPF 100Hz, cut mud 300Hz, presence +3kHz, "
                "air +8kHz, moderate compression, panned " + juce::String(panPos > 0 ? "right" : "left") + ".";
    return p;
}

SmartTrackFixer::FixProfile
SmartTrackFixer::buildAcousticGuitarProfile(const TrackElementClassifier::ClassificationResult& c, float intensity)
{
    FixProfile p;
    p.instrumentType = c.type;
    p.instrumentLabel = c.subType.isNotEmpty() ? c.subType : "Acoustic Guitar";
    p.overallIntensity = intensity;

    // EQ: HPF 80Hz, cut body/mud 250Hz, clarity 3kHz, air 12kHz
    p.eqBands[0] = { EQBand::Type::HighPass,   80.0f,   0.0f, 0.7f, true };
    p.eqBands[1] = { EQBand::Type::Parametric, 250.0f, -2.0f * intensity, 1.2f, true };
    p.eqBands[2] = { EQBand::Type::Parametric, 3000.0f, 1.5f * intensity, 1.5f, true };
    p.eqBands[3] = { EQBand::Type::HighShelf,  12000.0f, 2.0f * intensity, 0.7f, true };
    p.numActiveEQBands = 4;

    // Compression: Light, preserve dynamics
    p.compression = { 2.0f, -20.0f, 15.0f, 120.0f, 8.0f, 1.0f * intensity, true };

    // Stereo: Moderate pan offset
    p.stereo = { 1.1f, -0.25f * intensity, 0.0f, true };

    // Transients: Gentle sharpen for pick attack
    p.transients = { 10.0f * intensity, 5.0f * intensity, true };

    float currentRMSdB = (c.rmsLevel > 0.0001f) ? 20.0f * std::log10(c.rmsLevel) : -60.0f;
    p.suggestedGainDb = juce::jlimit(-8.0f, 8.0f, (-12.0f - currentRMSdB) * 0.5f * intensity);

    p.summary = "Acoustic Guitar Fix: HPF 80Hz, cut body 250Hz, clarity +3kHz, "
                "air +12kHz, light compression 2:1, panned left.";
    return p;
}

SmartTrackFixer::FixProfile
SmartTrackFixer::buildSynthProfile(const TrackElementClassifier::ClassificationResult& c, float intensity)
{
    FixProfile p;
    p.instrumentType = c.type;
    p.instrumentLabel = c.subType.isNotEmpty() ? c.subType : "Synth";
    p.overallIntensity = intensity;

    // EQ: HPF 60Hz, cut below-vocal-range 200Hz, slight presence 4kHz, roll off harshness 8kHz
    p.eqBands[0] = { EQBand::Type::HighPass,   60.0f,   0.0f, 0.7f, true };
    p.eqBands[1] = { EQBand::Type::Parametric, 200.0f, -1.5f * intensity, 1.0f, true };
    p.eqBands[2] = { EQBand::Type::Parametric, 4000.0f, 1.5f * intensity, 1.5f, true };
    p.eqBands[3] = { EQBand::Type::HighShelf,  8000.0f, -1.0f * intensity, 0.7f, true };
    p.numActiveEQBands = 4;

    // Compression: Moderate
    p.compression = { 3.0f, -16.0f, 12.0f, 100.0f, 6.0f, 1.5f * intensity, true };

    // Stereo: Wide, panned slightly
    float panPos = (c.subType.contains("Pad")) ? 0.15f : -0.3f;
    p.stereo = { 1.4f, panPos * intensity, 0.0f, true };

    // Transients: Soften for pads, sharpen for leads
    float attackAmt = (c.subType.contains("Pad")) ? -15.0f : 10.0f;
    p.transients = { attackAmt * intensity, 0.0f, true };

    float currentRMSdB = (c.rmsLevel > 0.0001f) ? 20.0f * std::log10(c.rmsLevel) : -60.0f;
    p.suggestedGainDb = juce::jlimit(-8.0f, 8.0f, (-14.0f - currentRMSdB) * 0.5f * intensity);

    p.summary = "Synth Fix: HPF 60Hz, spectral carve 200Hz, presence +4kHz, "
                "tame highs 8kHz, moderate compression, stereo wide.";
    return p;
}

SmartTrackFixer::FixProfile
SmartTrackFixer::buildPianoProfile(const TrackElementClassifier::ClassificationResult& c, float intensity)
{
    FixProfile p;
    p.instrumentType = c.type;
    p.instrumentLabel = c.subType.isNotEmpty() ? c.subType : "Piano";
    p.overallIntensity = intensity;

    // EQ: HPF 50Hz, body cut 300Hz, clarity 3kHz, air 10kHz
    p.eqBands[0] = { EQBand::Type::HighPass,   50.0f,   0.0f, 0.7f, true };
    p.eqBands[1] = { EQBand::Type::Parametric, 300.0f, -1.5f * intensity, 1.0f, true };
    p.eqBands[2] = { EQBand::Type::Parametric, 3000.0f, 2.0f * intensity, 1.5f, true };
    p.eqBands[3] = { EQBand::Type::HighShelf,  10000.0f, 1.5f * intensity, 0.7f, true };
    p.numActiveEQBands = 4;

    // Compression: Gentle, preserve dynamics
    p.compression = { 2.0f, -22.0f, 15.0f, 150.0f, 8.0f, 1.0f * intensity, true };

    // Stereo: Moderate width, slight pan
    p.stereo = { 1.2f, 0.15f * intensity, 0.0f, true };

    // Transients: Preserve natural hammer attack
    p.transients = { 5.0f * intensity, 10.0f * intensity, true };

    float currentRMSdB = (c.rmsLevel > 0.0001f) ? 20.0f * std::log10(c.rmsLevel) : -60.0f;
    p.suggestedGainDb = juce::jlimit(-8.0f, 8.0f, (-12.0f - currentRMSdB) * 0.5f * intensity);

    p.summary = "Piano Fix: HPF 50Hz, cut body 300Hz, clarity +3kHz, air +10kHz, "
                "gentle compression 2:1, moderate stereo width.";
    return p;
}

SmartTrackFixer::FixProfile
SmartTrackFixer::buildPercussionProfile(const TrackElementClassifier::ClassificationResult& c, float intensity)
{
    FixProfile p;
    p.instrumentType = c.type;
    p.instrumentLabel = c.subType.isNotEmpty() ? c.subType : "Percussion";
    p.overallIntensity = intensity;

    // EQ: HPF 100Hz, snap 2kHz, air 8kHz
    p.eqBands[0] = { EQBand::Type::HighPass,   100.0f,  0.0f, 0.7f, true };
    p.eqBands[1] = { EQBand::Type::Parametric, 2000.0f, 2.0f * intensity, 1.5f, true };
    p.eqBands[2] = { EQBand::Type::HighShelf,  8000.0f, 1.5f * intensity, 0.7f, true };
    p.numActiveEQBands = 3;

    // Compression: Medium, preserve groove
    p.compression = { 3.0f, -16.0f, 8.0f, 80.0f, 5.0f, 1.5f * intensity, true };

    // Stereo: Slightly wider, varied pan
    p.stereo = { 1.3f, 0.3f * intensity, 0.0f, true };

    // Transients: Sharpen for definition
    p.transients = { 20.0f * intensity, -5.0f * intensity, true };

    float currentRMSdB = (c.rmsLevel > 0.0001f) ? 20.0f * std::log10(c.rmsLevel) : -60.0f;
    p.suggestedGainDb = juce::jlimit(-8.0f, 8.0f, (-14.0f - currentRMSdB) * 0.5f * intensity);

    p.summary = "Percussion Fix: HPF 100Hz, snap +2kHz, air +8kHz, "
                "medium compression, transient sharpen, panned right.";
    return p;
}

SmartTrackFixer::FixProfile
SmartTrackFixer::buildDefaultProfile(const TrackElementClassifier::ClassificationResult& c, float intensity)
{
    FixProfile p;
    p.instrumentType = c.type;
    p.instrumentLabel = "Other";
    p.overallIntensity = intensity;

    // Generic: HPF 40Hz, slight clarity boost 3kHz
    p.eqBands[0] = { EQBand::Type::HighPass,   40.0f,  0.0f, 0.7f, true };
    p.eqBands[1] = { EQBand::Type::Parametric, 3000.0f, 1.0f * intensity, 1.5f, true };
    p.numActiveEQBands = 2;

    // Light compression
    p.compression = { 2.0f, -20.0f, 12.0f, 120.0f, 8.0f, 1.0f * intensity, true };

    p.stereo = { 1.0f, 0.0f, 0.0f, false };
    p.transients = { 0.0f, 0.0f, false };

    p.suggestedGainDb = 0.0f;
    p.summary = "Generic Fix: HPF 40Hz, slight clarity boost, light compression.";
    return p;
}
