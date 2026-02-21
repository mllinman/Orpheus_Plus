#pragma once
#include "Clip.h"

class AudioClip : public Clip
{
public:
    AudioClip(const juce::File& f, double start);
    ~AudioClip() override;

    void paint(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Rectangle<int> clipArea) override;
    
    void setThumbnailCache(juce::AudioThumbnailCache& cache, juce::AudioFormatManager& formatManager);
    std::unique_ptr<Clip> clone() const override;

    juce::File sourceFile;
    std::unique_ptr<juce::AudioThumbnail> thumbnail;

    // Playback data
    void loadAudioData(juce::AudioFormatManager& formatManager);
    juce::AudioBuffer<float> audioData;
    double sampleRate { 0.0 };
    double sourceBpm { 120.0 };
    bool isLoaded { false };
    bool loopEnabled { true };
    
    float getStretchFactor(double sessionBpm) const { 
        return (float)(sessionBpm / sourceBpm); 
    }
    
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioClip)
};
