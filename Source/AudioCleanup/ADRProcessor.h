#pragma once
#include <JuceHeader.h>
#include <vector>

class ADRProcessor
{
public:
    ADRProcessor();
    ~ADRProcessor();

    // Analyze the source location audio to capture its room impulse/EQ fingerprint
    void analyzeLocationAudio(const juce::AudioBuffer<float>& locationAudio);

    // Apply dialogue leveling (compression/normalization) and the extracted Match EQ
    void processADRClip(juce::AudioBuffer<float>& adrAudio);

private:
    float targetRMSLevel = 0.1f;
    std::vector<float> matchEQCurve;
    bool hasLocationFingerprint = false;

    // Simple multi-band dynamic processor mock
    void applyDialogueLeveling(juce::AudioBuffer<float>& buffer);
    
    // Spectral match EQ mock
    void applyMatchEQ(juce::AudioBuffer<float>& buffer);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ADRProcessor)
};
