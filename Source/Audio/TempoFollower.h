#pragma once
#include <JuceHeader.h>
#include <vector>
#include <atomic>

/**
 * Onset-detection based tempo estimator.
 * Feeds on audio input and tracks inter-onset intervals
 * to estimate BPM using an exponential moving average.
 */
class TempoFollower
{
public:
    TempoFollower() = default;

    void prepare(double sampleRate, int blockSize);
    void processBlock(const float* data, int numSamples);

    double getEstimatedBPM() const { return estimatedBPM.load(); }
    bool   isConfident()     const { return confidence.load() > 0.5f; }
    float  getConfidence()   const { return confidence.load(); }

    void setEnabled(bool e) { enabled.store(e); }
    bool isEnabled()  const { return enabled.load(); }

    void reset();

private:
    void detectOnset(float energy);

    double sampleRate_ = 44100.0;
    int    blockSize_  = 512;
    int    sampleCounter_ = 0;

    // Energy envelope
    float  prevEnergy_    = 0.0f;
    float  energyThreshold_ = 0.02f;

    // Onset timing
    std::vector<double> onsetTimesSeconds_;
    static constexpr int MAX_ONSETS = 64;

    // Output
    std::atomic<double> estimatedBPM { 120.0 };
    std::atomic<float>  confidence   { 0.0f };
    std::atomic<bool>   enabled      { false };

    // Cooldown to avoid double-triggers
    int cooldownSamples_ = 0;
    int cooldownMax_     = 4410; // ~100ms at 44.1k
};
