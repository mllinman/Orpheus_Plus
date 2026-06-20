#pragma once
#include "Clip.h"
#include "../Audio/TimeStretcher.h"

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
    float  pitchShift { 0.0f }; // In semitones
    bool   isLoaded { false };
    bool   loopEnabled { true };
    
    // Quick fades (seconds)
    double fadeInLength { 0.01 };
    double fadeOutLength { 0.01 };
    
    // Transients
    std::vector<double> transientHitpoints;
    void detectTransients();

    
    // Warping and Time-Stretching
    enum class WarpMode { Complex, Beats, Texture, Repitch, Off };
    WarpMode warpMode { WarpMode::Complex };

    struct WarpMarker {
        double sourceTime;   // Where in the original file this marker is
        double timelineTime; // Where on the timeline this marker is pinned
    };
    std::vector<WarpMarker> warpMarkers;

    TimeStretcher stretcher;
    
    float getStretchFactor(double sessionBpm) const { 
        return (float)(sessionBpm / sourceBpm); 
    }
    
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioClip)
};
