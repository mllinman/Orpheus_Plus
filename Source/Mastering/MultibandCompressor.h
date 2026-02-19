#pragma once
#include <JuceHeader.h>

class MultibandCompressor
{
public:
    MultibandCompressor();
    ~MultibandCompressor();

    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();
    // Simplified process for stereo buffer
    void process(juce::AudioBuffer<float>& buffer);

    // Crossovers
    void setLowMidCrossover(float freq);
    void setMidHighCrossover(float freq);

    // Per-band control
    void setThreshold(int band, float thresholdDB);
    void setRatio(int band, float ratio);
    void setAttack(int band, float attackMs);
    void setRelease(int band, float releaseMs);
    void setMakeupGain(int band, float gainDB);

private:
   static constexpr int NUM_BANDS = 3;

    using Filter = juce::dsp::LinkwitzRileyFilter<float>;
    
    // Crossover filters
    Filter filterLP1, filterHP1; // Split Low / (Mid+High)
    Filter filterLP2, filterHP2; // Split Mid / High

    std::array<juce::dsp::Compressor<float>, NUM_BANDS> compressors;
    std::array<float, NUM_BANDS> makeupGains;

    // Minimal buffers for splitting
    juce::AudioBuffer<float> bufferMob; // For mid+high
    juce::AudioBuffer<float> bufferHigh; 
    juce::AudioBuffer<float> bufferMid;
    juce::AudioBuffer<float> bufferLow;

    double currentSampleRate = 44100.0;
};
