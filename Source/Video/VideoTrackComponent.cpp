#include "VideoTrackComponent.h"

VideoTrackComponent::VideoTrackComponent(TransportController& transport)
    : transportController(transport)
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

void VideoTrackComponent::timerCallback()
{
    if (!isVideoLoaded) return;

    // If we had a real juce::VideoComponent, we would sync it here:
    // double audioTime = transport.getCurrentTimeSeconds(); // NOT IN JUCE 8 TRANSPORT
    double audioTime = 0.0;
    // videoComp.setPlayPosition(audioTime);
}
