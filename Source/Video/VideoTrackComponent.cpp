#include "VideoTrackComponent.h"

VideoTrackComponent::VideoTrackComponent(AudioEngine& engine, TransportController& transport)
    : audioEngine(engine), transportController(transport)
{
    videoComponent = std::make_unique<juce::Component>();
    addAndMakeVisible(videoComponent.get());
    startTimerHz(30); // 30 FPS sync update
}

VideoTrackComponent::~VideoTrackComponent()
{
    stopTimer();
}

void VideoTrackComponent::loadVideo(const juce::File& videoFile)
{
    juce::ignoreUnused(videoFile);
    // videoComponent->load(videoFile) removed in JUCE 8
    isVideoLoaded = true;
}

void VideoTrackComponent::resized()
{
    if (videoComponent)
        videoComponent->setBounds(getLocalBounds());
}

void VideoTrackComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
    
    if (isVideoLoaded)
    {
        // Draw Faux Video Frame Placeholder
        g.setColour(juce::Colours::darkgrey);
        g.drawRect(getLocalBounds(), 2);
        
        // Draw SMPTE Timecode overlay
        double audioTime = audioEngine.getPlayheadPosition();
        int hours = (int)(audioTime / 3600.0);
        int mins = (int)(audioTime / 60.0) % 60;
        int secs = (int)audioTime % 60;
        int frames = (int)((audioTime - std::floor(audioTime)) * 30.0); // Assuming 30fps
        
        juce::String smpte = juce::String::formatted("%02d:%02d:%02d:%02d", hours, mins, secs, frames);
        
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(24.0f, juce::Font::bold));
        g.drawText("SMPTE: " + smpte, getLocalBounds().reduced(20).removeFromBottom(40), juce::Justification::bottomRight, false);
        g.drawText("Video Placeholder Active", getLocalBounds(), juce::Justification::centred, false);
    }
    else
    {
        g.setColour(juce::Colours::white);
        g.setFont(16.0f);
        g.drawText("No Video Loaded", getLocalBounds(), juce::Justification::centred, false);
    }
}

void VideoTrackComponent::timerCallback()
{
    if (!isVideoLoaded) return;

    // Trigger repaint to update the SMPTE timecode
    repaint();
}
