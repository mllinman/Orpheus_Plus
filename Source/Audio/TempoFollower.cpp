#include "TempoFollower.h"
#include <numeric>
#include <algorithm>
#include <cmath>

void TempoFollower::prepare(double sampleRate, int blockSize)
{
    sampleRate_ = sampleRate;
    blockSize_  = blockSize;
    cooldownMax_ = (int)(0.1 * sampleRate); // 100ms minimum between onsets
    reset();
}

void TempoFollower::reset()
{
    onsetTimesSeconds_.clear();
    prevEnergy_ = 0.0f;
    cooldownSamples_ = 0;
    sampleCounter_ = 0;
    estimatedBPM.store(120.0);
    confidence.store(0.0f);
}

void TempoFollower::processBlock(const float* data, int numSamples)
{
    if (!enabled.load()) return;

    // Compute RMS energy of the block
    float energy = 0.0f;
    for (int i = 0; i < numSamples; ++i)
        energy += data[i] * data[i];
    energy = std::sqrt(energy / (float)numSamples);

    detectOnset(energy);

    prevEnergy_ = energy;
    sampleCounter_ += numSamples;
}

void TempoFollower::detectOnset(float energy)
{
    if (cooldownSamples_ > 0)
    {
        cooldownSamples_ -= blockSize_;
        return;
    }

    // Simple onset: energy rises sharply above threshold
    float diff = energy - prevEnergy_;
    if (diff > energyThreshold_ && energy > 0.01f)
    {
        double currentTimeSeconds = (double)sampleCounter_ / sampleRate_;
        onsetTimesSeconds_.push_back(currentTimeSeconds);

        // Keep only recent onsets
        if ((int)onsetTimesSeconds_.size() > MAX_ONSETS)
            onsetTimesSeconds_.erase(onsetTimesSeconds_.begin());

        cooldownSamples_ = cooldownMax_;

        // Estimate BPM from inter-onset intervals
        if (onsetTimesSeconds_.size() >= 4)
        {
            std::vector<double> intervals;
            for (size_t i = 1; i < onsetTimesSeconds_.size(); ++i)
            {
                double interval = onsetTimesSeconds_[i] - onsetTimesSeconds_[i - 1];
                if (interval > 0.15 && interval < 2.0) // 30-400 BPM range
                    intervals.push_back(interval);
            }

            if (intervals.size() >= 3)
            {
                // Median interval for robustness
                std::sort(intervals.begin(), intervals.end());
                double medianInterval = intervals[intervals.size() / 2];
                double bpm = 60.0 / medianInterval;

                // Clamp to reasonable range
                if (bpm >= 30.0 && bpm <= 300.0)
                {
                    // Exponential moving average
                    double prev = estimatedBPM.load();
                    double alpha = 0.3;
                    double smoothed = alpha * bpm + (1.0 - alpha) * prev;
                    estimatedBPM.store(smoothed);

                    // Confidence based on consistency
                    double variance = 0.0;
                    for (double iv : intervals)
                    {
                        double diff = iv - medianInterval;
                        variance += diff * diff;
                    }
                    variance /= (double)intervals.size();
                    float conf = (float)std::max(0.0, 1.0 - variance * 10.0);
                    confidence.store(conf);
                }
            }
        }
    }
}
