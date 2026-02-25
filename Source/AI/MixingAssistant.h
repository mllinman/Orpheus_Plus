#pragma once
#include <JuceHeader.h>
#include <vector>
#include <cmath>

/**
 * AI-powered mixing assistant.
 * Analyzes spectral content and provides heuristic-based suggestions
 * for EQ, compression, and balance.
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

        // Suggestions
        juce::String eqSuggestion;
        juce::String compSuggestion;
        juce::String panSuggestion;
        float        suggestedGain = 0.0f;  // dB adjustment
    };

    MixingAssistant() = default;

    /** Analyze a single track's audio buffer and produce suggestions. */
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

        // Spectral centroid (simplified via zero-crossing rate as proxy)
        int zeroCrossings = 0;
        for (int i = 1; i < numSamples; ++i)
        {
            if ((data[i] >= 0.0f) != (data[i - 1] >= 0.0f))
                zeroCrossings++;
        }
        result.spectralCentroid = (float)zeroCrossings / ((float)numSamples / (float)sampleRate) * 0.5f;

        // Generate suggestions
        generateSuggestions(result);

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
    void generateSuggestions(TrackAnalysis& t)
    {
        // EQ suggestions based on spectral centroid
        if (t.spectralCentroid < 500.0f)
            t.eqSuggestion = "Bass-heavy. Consider high-pass filter at 40-80 Hz and slight boost around 2-4 kHz for clarity.";
        else if (t.spectralCentroid < 2000.0f)
            t.eqSuggestion = "Mid-range dominant. Consider cutting 300-500 Hz to reduce muddiness.";
        else if (t.spectralCentroid < 6000.0f)
            t.eqSuggestion = "Well-balanced presence. Minor shelving at 8 kHz can add air.";
        else
            t.eqSuggestion = "Bright/harsh. Consider gentle cut around 3-5 kHz to tame sibilance.";

        // Compression suggestions based on crest factor
        if (t.crestFactor > 10.0f)
            t.compSuggestion = "Very dynamic. Apply gentle compression (ratio 2:1, threshold -18 dB) to even out levels.";
        else if (t.crestFactor > 4.0f)
            t.compSuggestion = "Moderate dynamics. Optional light compression (ratio 3:1, threshold -12 dB).";
        else
            t.compSuggestion = "Already compressed. Avoid additional compression to preserve transients.";

        // Gain staging
        float idealRMSdB = -18.0f; // K-14 reference
        float currentRMSdB = (t.rmsLevel > 0.0001f) ? 20.0f * std::log10(t.rmsLevel) : -70.0f;
        t.suggestedGain = idealRMSdB - currentRMSdB;
        t.suggestedGain = juce::jlimit(-12.0f, 12.0f, t.suggestedGain);
    }
};
