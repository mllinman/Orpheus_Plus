#include "AudioClip.h"
#include "../Audio/TransientDetector.h"

AudioClip::AudioClip(const juce::File& f, double start)
    : Clip(Type::Audio), sourceFile(f)
{
    startTime = start;
    name = f.getFileNameWithoutExtension();
    stretcher.reset(44100.0, 2); // Default, will be updated on load
}

AudioClip::~AudioClip()
{
}

std::unique_ptr<Clip> AudioClip::clone() const
{
    auto copy = std::make_unique<AudioClip>(sourceFile, startTime);
    copy->duration = duration;
    copy->offset = offset;
    copy->name = name;
    copy->colour = colour;
    copy->fadeIn = fadeIn;
    copy->fadeOut = fadeOut;
    copy->gain = gain;
    copy->muted = muted;
    copy->loopEnabled = loopEnabled;
    copy->pitchShift = pitchShift;

    if (isLoaded)
    {
        copy->audioData = audioData; // juce::AudioBuffer copies its data underneath or shares? Actually juce::AudioBuffer operator= makes a deep copy or we can just deep copy.
        // Wait, juce::AudioBuffer copy constructor makes a deep copy.
        copy->sampleRate = sampleRate;
        copy->isLoaded = true;
    }

    return copy;
}

void AudioClip::setThumbnailCache(juce::AudioThumbnailCache& cache, juce::AudioFormatManager& formatManager)
{
    thumbnail = std::make_unique<juce::AudioThumbnail>(512, formatManager, cache);
    thumbnail->setSource(new juce::FileInputSource(sourceFile));
    
    // Estimate duration using reader usually, but thumbnail also approximates it.
    // Ideally we should open a reader to get exact duration, but for now we rely on thumbnail or external setter.
}

void AudioClip::loadAudioData(juce::AudioFormatManager& formatManager)
{
    if (isLoaded) return;
    
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(sourceFile));
    if (reader)
    {
        sampleRate = reader->sampleRate;
        auto lengthInSamples = (int)reader->lengthInSamples;
        audioData.setSize(reader->numChannels, lengthInSamples);
        reader->read(&audioData, 0, lengthInSamples, 0, true, true);
        
        duration = lengthInSamples / sampleRate;
        isLoaded = true;
        stretcher.reset(sampleRate, reader->numChannels);

        detectTransients();


        // Try to detect BPM from filename (e.g. "loop_120bpm.wav")
        auto filename = sourceFile.getFileNameWithoutExtension().toLowerCase();
        int bpmIdx = filename.indexOf("bpm");
        if (bpmIdx > 0)
        {
            int start = bpmIdx - 1;
            while (start >= 0 && juce::CharacterFunctions::isDigit(filename[start]))
                --start;
            
            auto bpmStr = filename.substring(start + 1, bpmIdx);
            if (bpmStr.isNotEmpty())
                sourceBpm = bpmStr.getDoubleValue();
        }
    }
}

void AudioClip::paint(juce::Graphics& g, juce::Rectangle<float> clipBounds, juce::Rectangle<int> clipArea)
{
    // Background
    g.setColour(selected ? colour.brighter(0.3f) : colour.withBrightness(0.5f));
    g.fillRoundedRectangle(clipBounds, 3.0f);

    // Title
    g.setColour(juce::Colours::white.withAlpha(0.9f));
    g.setFont(juce::FontOptions(10.0f));
    g.drawText(name, clipBounds.toNearestInt().reduced(4, 2),
               juce::Justification::topLeft, true);

    // Waveform
    if (thumbnail)
    {
        g.setColour(juce::Colours::white.withAlpha(0.6f));
        auto waveArea = clipBounds.reduced(0, 14);
        
        // Draw the visible portion
        double sourceLen = thumbnail->getTotalLength();
        if (sourceLen <= 0.0) sourceLen = duration;
        
        if (loopEnabled && sourceLen > 0.0 && duration > sourceLen)
        {
            double currentT = 0.0;
            while (currentT < duration)
            {
                double segmentLen = juce::jmin(sourceLen, duration - currentT);
                
                float pixelX = clipBounds.getX() + (float)(currentT / duration * clipBounds.getWidth());
                float pixelW = (float)(segmentLen / duration * clipBounds.getWidth());
                
                auto segmentRect = juce::Rectangle<int>((int)pixelX, (int)waveArea.getY(), (int)pixelW, (int)waveArea.getHeight());
                thumbnail->drawChannel(g, segmentRect, 0.0, segmentLen, 0, 1.0f);
                
                if (currentT > 0.0)
                {
                    g.setColour(juce::Colours::white.withAlpha(0.3f));
                    g.drawVerticalLine((int)pixelX, waveArea.getY(), waveArea.getBottom());
                    g.setColour(juce::Colours::white.withAlpha(0.6f));
                }
                
                currentT += sourceLen;
            }
        }
        else
        {
            thumbnail->drawChannel(g, waveArea.toNearestInt(), 0.0, duration, 0, 1.0f);
        }
    }

    // Warp Markers
    if (warpMode != WarpMode::Off && !warpMarkers.empty())
    {
        g.setColour(juce::Colours::yellow.withAlpha(0.8f));
        for (const auto& marker : warpMarkers)
        {
            float markerX = clipBounds.getX() + (float)((marker.timelineTime / duration) * clipBounds.getWidth());
            g.drawVerticalLine((int)markerX, clipBounds.getY(), clipBounds.getBottom());
            
            // Draw a tiny triangle at the top
            juce::Path p;
            p.addTriangle(markerX - 4.0f, clipBounds.getY(), 
                          markerX + 4.0f, clipBounds.getY(), 
                          markerX, clipBounds.getY() + 6.0f);
            g.fillPath(p);
        }
    }

    // Transients
    if (!transientHitpoints.empty())
    {
        g.setColour(juce::Colours::cyan.withAlpha(0.3f));
        for (double t : transientHitpoints)
        {
            if (t > duration) break;
            float px = clipBounds.getX() + (float)(t / duration * clipBounds.getWidth());
            g.drawVerticalLine((int)px, clipBounds.getY(), clipBounds.getBottom());
        }
    }

    // Border

    g.setColour(selected ? juce::Colours::white : colour.brighter(0.2f));
    g.drawRoundedRectangle(clipBounds, 3.0f, 1.0f);
    
    // Fades
    if (fadeIn > 0)
    {
        float fadeW = (float)(fadeIn * clipBounds.getWidth() / duration);
        if (fadeW > 0)
        {
            g.setColour(juce::Colours::black.withAlpha(0.3f));
            juce::Path p;
            p.startNewSubPath(clipBounds.getX(), clipBounds.getY());
            p.lineTo(clipBounds.getX() + fadeW, clipBounds.getY());
            p.lineTo(clipBounds.getX(), clipBounds.getBottom());
            p.closeSubPath();
            g.fillPath(p);
        }
    }
    
    if (fadeOut > 0)
    {
        float fadeW = (float)(fadeOut * clipBounds.getWidth() / duration);
        if (fadeW > 0)
        {
            g.setColour(juce::Colours::black.withAlpha(0.3f));
            juce::Path p;
            p.startNewSubPath(clipBounds.getRight(), clipBounds.getY());
            p.lineTo(clipBounds.getRight() - fadeW, clipBounds.getY());
            p.lineTo(clipBounds.getRight(), clipBounds.getBottom());
            p.closeSubPath();
            g.fillPath(p);
        }
    }
}

void AudioClip::detectTransients()
{
    if (!isLoaded || sampleRate <= 0.0) return;
    transientHitpoints = TransientDetector::detectTransients(audioData, sampleRate, 0.4f, 0.05f);
}
