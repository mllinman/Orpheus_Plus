#pragma once
#include <JuceHeader.h>
#include <vector>
#include <cmath>

/**
 * Modulation sources: LFO and Envelope Follower.
 * Used to modulate any mapped parameter at audio rate.
 */

// ─── LFO Source ──────────────────────────────────────────────────────────────

class LFOSource
{
public:
    enum class Waveform { Sine, Square, Triangle, Saw };

    LFOSource() = default;

    void prepare(double sampleRate) { sampleRate_ = sampleRate; }

    void setRate(float hz)        { rate_ = hz; }
    void setDepth(float d)        { depth_ = d; }
    void setWaveform(Waveform w)  { waveform_ = w; }

    float getRate()  const { return rate_; }
    float getDepth() const { return depth_; }

    /** Returns modulation value in [-depth, +depth] and advances phase. */
    float tick()
    {
        float value = 0.0f;
        switch (waveform_)
        {
            case Waveform::Sine:
                value = std::sin(phase_ * juce::MathConstants<float>::twoPi);
                break;
            case Waveform::Square:
                value = phase_ < 0.5f ? 1.0f : -1.0f;
                break;
            case Waveform::Triangle:
                value = 4.0f * std::abs(phase_ - 0.5f) - 1.0f;
                break;
            case Waveform::Saw:
                value = 2.0f * phase_ - 1.0f;
                break;
        }

        phase_ += rate_ / (float)sampleRate_;
        if (phase_ >= 1.0f) phase_ -= 1.0f;

        return value * depth_;
    }

    void reset() { phase_ = 0.0f; }

private:
    double sampleRate_ = 44100.0;
    float  rate_       = 1.0f;   // Hz
    float  depth_      = 0.5f;   // 0..1
    float  phase_      = 0.0f;
    Waveform waveform_ = Waveform::Sine;
};

// ─── Envelope Follower Source ────────────────────────────────────────────────

class EnvelopeFollowerSource
{
public:
    EnvelopeFollowerSource() = default;

    void prepare(double sampleRate)
    {
        sampleRate_ = sampleRate;
        updateCoefficients();
    }

    void setAttack(float ms)
    {
        attackMs_ = ms;
        updateCoefficients();
    }

    void setRelease(float ms)
    {
        releaseMs_ = ms;
        updateCoefficients();
    }

    void setSensitivity(float s) { sensitivity_ = s; }

    /** Feed an audio sample and get the envelope value. */
    float processSample(float input)
    {
        float absInput = std::abs(input) * sensitivity_;
        if (absInput > envelope_)
            envelope_ = attackCoeff_ * (envelope_ - absInput) + absInput;
        else
            envelope_ = releaseCoeff_ * (envelope_ - absInput) + absInput;

        return juce::jlimit(0.0f, 1.0f, envelope_);
    }

    void reset() { envelope_ = 0.0f; }

private:
    void updateCoefficients()
    {
        if (sampleRate_ > 0)
        {
            attackCoeff_  = std::exp(-1.0f / (float)(attackMs_  * 0.001f * sampleRate_));
            releaseCoeff_ = std::exp(-1.0f / (float)(releaseMs_ * 0.001f * sampleRate_));
        }
    }

    double sampleRate_    = 44100.0;
    float  attackMs_      = 5.0f;
    float  releaseMs_     = 50.0f;
    float  sensitivity_   = 1.0f;
    float  envelope_      = 0.0f;
    float  attackCoeff_   = 0.0f;
    float  releaseCoeff_  = 0.0f;
};

// ─── Modulation Mapping ─────────────────────────────────────────────────────

struct ModulationMapping
{
    int          sourceId;       // Index into a managed array of sources
    juce::String targetParam;   // e.g., "vol", "pan", "cutoff"
    int          trackIndex;    // Which track this applies to (-1 = master)
    float        depth;         // Modulation depth multiplier
};
