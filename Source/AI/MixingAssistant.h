#pragma once
#include <JuceHeader.h>
#include "TrackElementClassifier.h"
#include <vector>
#include <cmath>
#include <array>

/**
 * AI-powered mixing assistant — enhanced with instrument-aware intelligence.
 *
 * Phase 1 (original): Analyzes spectral content and provides heuristic-based suggestions
 *                      for EQ, compression, and balance.
 *
 * Phase 2 (upgraded):  Uses TrackElementClassifier for instrument detection, then generates
 *                       instrument-specific EQ/compression/pan/gain suggestions using
 *                       FFT-based spectral centroid (replacing zero-crossing proxy) and
 *                       multi-band energy analysis.
 */
class MixingAssistant
{
public:
    struct TrackAnalysis
    {
        juce::String trackName;
        float        peakLevel    = 0.0f;
        float        rmsLevel     = 0.0f;
        float        spectralCentroid = 0.0f;
        float        crestFactor  = 0.0f;   // peak / rms

        // ── New: Instrument classification fields ──
        TrackElementClassifier::InstrumentType instrumentType
            = TrackElementClassifier::InstrumentType::Other;
        float        classificationConfidence = 0.0f;
        juce::String instrumentSubType;

        // ── New: Advanced spectral features ──
        float harmonicToNoiseRatio = 0.0f;   // dB
        float transientDensity     = 0.0f;   // Onsets per second
        float lowBandEnergy        = 0.0f;   // Proportion 0-1
        float midBandEnergy        = 0.0f;
        float highBandEnergy       = 0.0f;

        // Suggestions
        juce::String eqSuggestion;
        juce::String compSuggestion;
        juce::String panSuggestion;
        float        suggestedGain = 0.0f;  // dB adjustment
        float        suggestedPan  = 0.0f;  // -1.0 to 1.0
    };

    MixingAssistant() = default;

    /** Analyze a single track's audio buffer and produce instrument-aware suggestions. */
    TrackAnalysis analyzeTrack(const juce::String& name,
                               const juce::AudioBuffer<float>& buffer,
                               double sampleRate)
    {
        TrackAnalysis result;
        result.trackName = name;

        if (buffer.getNumSamples() == 0) return result;

        int numSamples = buffer.getNumSamples();
        auto* data = buffer.getReadPointer(0);

        // Peak and RMS
        float peak = 0.0f, rmsSum = 0.0f;
        for (int i = 0; i < numSamples; ++i)
        {
            float s = std::abs(data[i]);
            peak = juce::jmax(peak, s);
            rmsSum += data[i] * data[i];
        }
        result.peakLevel = peak;
        result.rmsLevel  = std::sqrt(rmsSum / (float)numSamples);
        result.crestFactor = (result.rmsLevel > 0.0001f)
            ? result.peakLevel / result.rmsLevel : 0.0f;

        // ── NEW: Use TrackElementClassifier for instrument detection + spectral features ──
        auto classification = classifier.classify(name, buffer, sampleRate);

        result.spectralCentroid       = classification.spectralCentroid;
        result.instrumentType         = classification.type;
        result.classificationConfidence = classification.confidence;
        result.instrumentSubType      = classification.subType;
        result.harmonicToNoiseRatio   = classification.harmonicToNoiseRatio;
        result.transientDensity       = classification.transientDensity;
        result.lowBandEnergy          = classification.lowBandEnergy;
        result.midBandEnergy          = classification.midBandEnergy;
        result.highBandEnergy         = classification.highBandEnergy;

        // ── Generate instrument-aware suggestions ──
        generateInstrumentAwareSuggestions(result);

        return result;
    }

    /** Analyze all tracks and produce a full mix report. */
    std::vector<TrackAnalysis> analyzeMix(
        const std::vector<std::pair<juce::String, juce::AudioBuffer<float>>>& tracks,
        double sampleRate)
    {
        std::vector<TrackAnalysis> results;
        for (auto& [name, buffer] : tracks)
            results.push_back(analyzeTrack(name, buffer, sampleRate));
        return results;
    }

private:
    TrackElementClassifier classifier;

    void generateInstrumentAwareSuggestions(TrackAnalysis& t)
    {
        using IT = TrackElementClassifier::InstrumentType;

        // ── EQ Suggestions — instrument-specific ──
        switch (t.instrumentType)
        {
            case IT::Vocals:
                t.eqSuggestion = "Vocals: HPF at 80 Hz. Cut 250 Hz for clarity. "
                                 "Boost 3–5 kHz for presence. Shelf +1.5 dB at 10 kHz for air. "
                                 "Gentle cut at 7 kHz to control sibilance.";
                break;

            case IT::Drums:
                t.eqSuggestion = "Drums: HPF at 30 Hz. Boost sub at 60 Hz for weight. "
                                 "Cut mud at 400 Hz. Boost 5 kHz for snap. "
                                 "Shelf at 10 kHz for cymbal shimmer.";
                break;

            case IT::Bass:
                t.eqSuggestion = "Bass: HPF at 30 Hz. Warm shelf boost at 200 Hz. "
                                 "Cut mid honk at 650 Hz. LPF at 8 kHz to remove noise. "
                                 "Keep mono below 200 Hz.";
                break;

            case IT::ElectricGuitar:
                t.eqSuggestion = "Electric Guitar: HPF at 100 Hz to clear sub for bass/kick. "
                                 "Cut mud at 300 Hz. Boost presence at 3 kHz. "
                                 "Shelf at 8 kHz for sparkle.";
                break;

            case IT::AcousticGuitar:
                t.eqSuggestion = "Acoustic Guitar: HPF at 80 Hz. Cut body buildup at 250 Hz. "
                                 "Boost clarity at 3 kHz. High shelf +2 dB at 12 kHz for air/string detail.";
                break;

            case IT::Synth:
                t.eqSuggestion = "Synth: HPF at 60 Hz. Cut 200 Hz to avoid vocal masking. "
                                 "Boost 4 kHz for presence. Tame harshness above 8 kHz.";
                break;

            case IT::Piano:
                t.eqSuggestion = "Piano: HPF at 50 Hz. Cut body at 300 Hz for a leaner sound. "
                                 "Boost 3 kHz for note clarity. High shelf at 10 kHz for brilliance.";
                break;

            case IT::Percussion:
                t.eqSuggestion = "Percussion: HPF at 100 Hz. Boost snap at 2 kHz. "
                                 "High shelf +1.5 dB at 8 kHz for definition.";
                break;

            default:
                // Fallback to spectral-centroid-based generic suggestions
                if (t.spectralCentroid < 500.0f)
                    t.eqSuggestion = "Bass-heavy. Consider high-pass filter at 40-80 Hz and slight boost around 2-4 kHz for clarity.";
                else if (t.spectralCentroid < 2000.0f)
                    t.eqSuggestion = "Mid-range dominant. Consider cutting 300-500 Hz to reduce muddiness.";
                else if (t.spectralCentroid < 6000.0f)
                    t.eqSuggestion = "Well-balanced presence. Minor shelving at 8 kHz can add air.";
                else
                    t.eqSuggestion = "Bright/harsh. Consider gentle cut around 3-5 kHz to tame sibilance.";
                break;
        }

        // ── Compression Suggestions — instrument-aware ──
        switch (t.instrumentType)
        {
            case IT::Vocals:
                t.compSuggestion = "Vocals: 3:1 ratio, threshold -18 dB, attack 8 ms, release 80 ms. "
                                   "Consider serial compression with a second lighter stage.";
                break;

            case IT::Drums:
                t.compSuggestion = "Drums: 4:1 ratio, threshold -15 dB, attack 5 ms, release 60 ms. "
                                   "Add parallel compression for punch without squashing transients.";
                break;

            case IT::Bass:
                t.compSuggestion = "Bass: 4:1 ratio, threshold -16 dB, attack 5 ms, release 50 ms. "
                                   "Keep it tight and consistent. Consider a limiter for peaks.";
                break;

            case IT::ElectricGuitar:
            case IT::AcousticGuitar:
                if (t.crestFactor > 8.0f)
                    t.compSuggestion = "Guitar: Very dynamic — apply 3:1 at -14 dB. "
                                       "Use moderate attack (10 ms) to preserve pick transients.";
                else
                    t.compSuggestion = "Guitar: Light compression 2:1 at -20 dB. "
                                       "Already fairly even — preserve natural dynamics.";
                break;

            case IT::Synth:
                t.compSuggestion = "Synth: 3:1 ratio, threshold -16 dB. "
                                   "Longer release (100 ms) for smooth sustain.";
                break;

            case IT::Piano:
                t.compSuggestion = "Piano: Gentle 2:1, threshold -22 dB, slow attack (15 ms). "
                                   "Preserve hammer dynamics while evening out the sustain.";
                break;

            default:
                if (t.crestFactor > 10.0f)
                    t.compSuggestion = "Very dynamic. Apply gentle compression (ratio 2:1, threshold -18 dB) to even out levels.";
                else if (t.crestFactor > 4.0f)
                    t.compSuggestion = "Moderate dynamics. Optional light compression (ratio 3:1, threshold -12 dB).";
                else
                    t.compSuggestion = "Already compressed. Avoid additional compression to preserve transients.";
                break;
        }

        // ── Pan Suggestions — instrument-specific stereo placement ──
        switch (t.instrumentType)
        {
            case IT::Vocals:
                t.panSuggestion = "Vocals: Keep centered (0%). Center panning ensures vocal clarity and focus.";
                t.suggestedPan = 0.0f;
                break;

            case IT::Drums:
                t.panSuggestion = "Drums: Keep overheads/room slightly wide. Kick and snare centered.";
                t.suggestedPan = 0.0f;
                break;

            case IT::Bass:
                t.panSuggestion = "Bass: Always centered. Mono below 200 Hz for solid low end.";
                t.suggestedPan = 0.0f;
                break;

            case IT::ElectricGuitar:
                t.panSuggestion = "Electric Guitar: Pan 30–50% left or right. "
                                  "Double-tracked guitars go hard L/R.";
                t.suggestedPan = 0.4f; // Default right; could alternate
                break;

            case IT::AcousticGuitar:
                t.panSuggestion = "Acoustic Guitar: Pan 25% to one side to open the center for vocals.";
                t.suggestedPan = -0.25f;
                break;

            case IT::Synth:
                t.panSuggestion = "Synth: Pan 20–40% for pads. Leads can stay closer to center.";
                t.suggestedPan = 0.25f;
                break;

            case IT::Piano:
                t.panSuggestion = "Piano: Slight pan 15% for stereo interest while keeping body centered.";
                t.suggestedPan = 0.15f;
                break;

            case IT::Percussion:
                t.panSuggestion = "Percussion: Pan 30% for rhythmic variety in the stereo field.";
                t.suggestedPan = 0.3f;
                break;

            default:
                t.panSuggestion = "No specific pan recommendation.";
                t.suggestedPan = 0.0f;
                break;
        }

        // ── Gain Staging — instrument-aware target RMS levels ──
        float targetRMSdB = -18.0f; // Default K-14
        switch (t.instrumentType)
        {
            case IT::Vocals:          targetRMSdB = -14.0f; break;
            case IT::Drums:           targetRMSdB = -16.0f; break;
            case IT::Bass:            targetRMSdB = -16.0f; break;
            case IT::ElectricGuitar:  targetRMSdB = -18.0f; break;
            case IT::AcousticGuitar:  targetRMSdB = -18.0f; break;
            case IT::Synth:           targetRMSdB = -20.0f; break;
            case IT::Piano:           targetRMSdB = -18.0f; break;
            case IT::Percussion:      targetRMSdB = -20.0f; break;
            default:                  targetRMSdB = -18.0f; break;
        }

        float currentRMSdB = (t.rmsLevel > 0.0001f) ? 20.0f * std::log10(t.rmsLevel) : -70.0f;
        t.suggestedGain = targetRMSdB - currentRMSdB;
        t.suggestedGain = juce::jlimit(-12.0f, 12.0f, t.suggestedGain);
    }
};
