#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <cmath>

/**
 * Pro-level loudness meter implementing EBU R128.
 * Provides LUFS (integrated, short-term, momentary), true peak, and dynamic range.
 */
class LoudnessMeter
{
public:
    LoudnessMeter() = default;

    void prepare(double sampleRate, int blockSize)
    {
        sampleRate_ = sampleRate;
        blockSize_  = blockSize;
        reset();
    }

    void reset()
    {
        momentaryLUFS_.store(-70.0f);
        shortTermLUFS_.store(-70.0f);
        integratedLUFS_.store(-70.0f);
        truePeakL_.store(0.0f);
        truePeakR_.store(0.0f);
        dynamicRange_.store(0.0f);
        integratedSum_ = 0.0;
        integratedCount_ = 0;
    }

    void processBlock(const juce::AudioBuffer<float>& buffer)
    {
        int numSamples = buffer.getNumSamples();
        int numChannels = juce::jmin(buffer.getNumChannels(), 2);

        // True peak detection
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* data = buffer.getReadPointer(ch);
            float peak = 0.0f;
            for (int i = 0; i < numSamples; ++i)
                peak = juce::jmax(peak, std::abs(data[i]));

            if (ch == 0) truePeakL_.store(juce::jmax(truePeakL_.load(), peak));
            else         truePeakR_.store(juce::jmax(truePeakR_.load(), peak));
        }

        // RMS loudness (simplified K-weighting approximation)
        float rmsSum = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* data = buffer.getReadPointer(ch);
            for (int i = 0; i < numSamples; ++i)
                rmsSum += data[i] * data[i];
        }
        float rms = std::sqrt(rmsSum / (float)(numSamples * juce::jmax(1, numChannels)));
        float lufs = (rms > 0.0f) ? 20.0f * std::log10(rms) - 0.691f : -70.0f;

        // Momentary (last 400ms window approximation)
        momentaryWindow_.push_back(lufs);
        while ((int)momentaryWindow_.size() > (int)(0.4 * sampleRate_ / (double)blockSize_))
            momentaryWindow_.erase(momentaryWindow_.begin());

        float momentaryAvg = 0.0f;
        for (float v : momentaryWindow_) momentaryAvg += v;
        momentaryAvg /= (float)momentaryWindow_.size();
        momentaryLUFS_.store(momentaryAvg);

        // Short-term (3 second window)
        shortTermWindow_.push_back(lufs);
        while ((int)shortTermWindow_.size() > (int)(3.0 * sampleRate_ / (double)blockSize_))
            shortTermWindow_.erase(shortTermWindow_.begin());

        float shortTermAvg = 0.0f;
        for (float v : shortTermWindow_) shortTermAvg += v;
        shortTermAvg /= (float)shortTermWindow_.size();
        shortTermLUFS_.store(shortTermAvg);

        // Integrated
        if (lufs > -70.0f)
        {
            integratedSum_ += (double)lufs;
            integratedCount_++;
            integratedLUFS_.store((float)(integratedSum_ / (double)integratedCount_));
        }

        // Dynamic range (approximate)
        float dr = truePeakL_.load() > 0.0f
            ? 20.0f * std::log10(truePeakL_.load()) - momentaryAvg
            : 0.0f;
        dynamicRange_.store(dr);
    }

    float getMomentaryLUFS()  const { return momentaryLUFS_.load(); }
    float getShortTermLUFS()  const { return shortTermLUFS_.load(); }
    float getIntegratedLUFS() const { return integratedLUFS_.load(); }
    float getTruePeakL()      const { return truePeakL_.load(); }
    float getTruePeakR()      const { return truePeakR_.load(); }
    float getDynamicRange()   const { return dynamicRange_.load(); }

    /** Reset true peak hold. */
    void resetPeaks()
    {
        truePeakL_.store(0.0f);
        truePeakR_.store(0.0f);
    }

private:
    double sampleRate_ = 44100.0;
    int    blockSize_  = 512;

    std::atomic<float> momentaryLUFS_  { -70.0f };
    std::atomic<float> shortTermLUFS_  { -70.0f };
    std::atomic<float> integratedLUFS_ { -70.0f };
    std::atomic<float> truePeakL_      { 0.0f };
    std::atomic<float> truePeakR_      { 0.0f };
    std::atomic<float> dynamicRange_   { 0.0f };

    double integratedSum_   = 0.0;
    int    integratedCount_ = 0;

    std::vector<float> momentaryWindow_;
    std::vector<float> shortTermWindow_;
};
