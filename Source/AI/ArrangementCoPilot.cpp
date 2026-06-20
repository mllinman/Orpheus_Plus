#include "ArrangementCoPilot.h"
#include <cmath>

void ArrangementCoPilot::analyzeEnergyCurve(const float* audioData, int numSamples, double sampleRate)
{
    energyEnvelope.clear();
    suggestions.clear();

    // Very basic offline RMS windowing to build an energy envelope
    int windowSize = static_cast<int>(sampleRate * 0.5); // 500ms windows
    if (windowSize == 0 || numSamples == 0) return;

    for (int i = 0; i < numSamples; i += windowSize) {
        int end = juce::jmin(i + windowSize, numSamples);
        float sumSq = 0.0f;
        for (int j = i; j < end; ++j) {
            sumSq += audioData[j] * audioData[j];
        }
        float rms = std::sqrt(sumSq / (end - i));
        energyEnvelope.push_back(rms);
    }

    // Identify significant drops (potential sections for risers or fills)
    for (size_t i = 1; i < energyEnvelope.size(); ++i) {
        float delta = energyEnvelope[i] - energyEnvelope[i-1];
        if (delta > 0.3f) {
            suggestions.push_back({ (i * windowSize) / sampleRate, "Huge energy spike detected. Suggest: Add Impact/Crash", "Impact" });
            suggestions.push_back({ ((i-2) * windowSize) / sampleRate, "Energy building. Suggest: Add Riser", "Riser" });
        } else if (delta < -0.3f) {
            suggestions.push_back({ (i * windowSize) / sampleRate, "Energy drop. Suggest: Drum Fill or Sub Drop", "Fill" });
        }
    }
}
