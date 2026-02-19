#pragma once
#include "Clip.h"

class AudioClip : public Clip
{
public:
    AudioClip(const juce::File& f, double start);
    ~AudioClip() override;

    void paint(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Rectangle<int> clipArea) override;
    
    void setThumbnailCache(juce::AudioThumbnailCache& cache, juce::AudioFormatManager& formatManager);

    juce::File sourceFile;
    std::unique_ptr<juce::AudioThumbnail> thumbnail;
    
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioClip)
};
