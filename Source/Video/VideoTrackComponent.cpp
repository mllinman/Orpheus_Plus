#include "VideoTrackComponent.h"

VideoTrackComponent::VideoTrackComponent(TransportController& transport)
    : transportController(transport)
{
    videoComponent = std::make_unique<juce::VideoComponent>();
    addAndMakeVisible(videoComponent.get());
    startTimerHz(30); // 30 FPS sync update
}

VideoTrackComponent::~VideoTrackComponent()
{
    stopTimer();
}

void VideoTrackComponent::loadVideo(const juce::File& videoFile)
{
    if (videoComponent->load(videoFile))
    {
        isVideoLoaded = true;
    }
}

void VideoTrackComponent::resized()
{
    if (videoComponent)
        videoComponent->setBounds(getLocalBounds());
}

void VideoTrackComponent::timerCallback()
{
    if (!isVideoLoaded) return;

    double currentTime = transportController.getCurrentTimeSeconds();
    
    // Scrub video to audio transport time if it drifted or scrubbed
    double videoTime = videoComponent->getVideoPosition();
    if (std::abs(videoTime - currentTime) > 0.1)
    {
        videoComponent->setPlayPosition(currentTime);
    }
    
    // Play state sync
    if (transportController.isPlaying() && !videoComponent->isPlaying())
    {
        videoComponent->play();
    }
    else if (!transportController.isPlaying() && videoComponent->isPlaying())
    {
        videoComponent->stop();
    }
}
