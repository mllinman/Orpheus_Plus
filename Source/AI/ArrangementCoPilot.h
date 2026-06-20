#pragma once
#include <JuceHeader.h>
#include <vector>

class ArrangementCoPilot
{
public:
    ArrangementCoPilot() = default;
    ~ArrangementCoPilot() = default;

    struct Suggestion {
        double timeSeconds;
        juce::String text;
        juce::String type; // "Riser", "Drop", "Fill"
    };

    // Feeds an entire mixdown track or stem into the analyzer to find energy curves
    void analyzeEnergyCurve(const float* audioData, int numSamples, double sampleRate);

    // Returns structural suggestions based on the analyzed energy drops/spikes
    std::vector<Suggestion> getSuggestions() const { return suggestions; }

private:
    std::vector<Suggestion> suggestions;
    std::vector<float> energyEnvelope;
};
