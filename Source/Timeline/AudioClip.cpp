#include "AudioClip.h"

AudioClip::AudioClip(const juce::File& f, double start)
    : Clip(Type::Audio), sourceFile(f)
{
    startTime = start;
    name = f.getFileNameWithoutExtension();
}

AudioClip::~AudioClip()
{
}

void AudioClip::setThumbnailCache(juce::AudioThumbnailCache& cache, juce::AudioFormatManager& formatManager)
{
    thumbnail = std::make_unique<juce::AudioThumbnail>(512, formatManager, cache);
    thumbnail->setSource(new juce::FileInputSource(sourceFile));
    
    // Estimate duration using reader usually, but thumbnail also approximates it.
    // Ideally we should open a reader to get exact duration, but for now we rely on thumbnail or external setter.
}

void AudioClip::paint(juce::Graphics& g, juce::Rectangle<float> clipBounds, juce::Rectangle<int> clipArea)
{
    // Background
    g.setColour(selected ? colour.brighter(0.3f) : colour.withBrightness(0.5f));
    g.fillRoundedRectangle(clipBounds, 3.0f);

    // Title
    g.setColour(juce::Colours::white.withAlpha(0.9f));
    g.setFont(juce::Font(10.0f));
    g.drawText(name, clipBounds.toNearestInt().reduced(4, 2),
               juce::Justification::topLeft, true);

    // Waveform
    if (thumbnail)
    {
        g.setColour(juce::Colours::white.withAlpha(0.6f));
        auto waveArea = clipBounds.reduced(0, 14);
        
        // Draw the visible portion
        // We need to map time to x
        // For simplicity, just draw the whole clip's thumbnail in the bounds
        thumbnail->drawChannel(g, waveArea.toNearestInt(), 0.0, thumbnail->getTotalLength(), 0, 1.0f);
    }

    // Border
    g.setColour(selected ? juce::Colours::white : colour.brighter(0.2f));
    g.drawRoundedRectangle(clipBounds, 3.0f, 1.0f);
}
