#pragma once
#include <JuceHeader.h>
#include "../Timeline/TransportController.h"

class VideoTrackComponent : public juce::Component, public juce::Timer
{
public:
    VideoTrackComponent(TransportController& transport);
    ~VideoTrackComponent() override;

    void loadVideo(const juce::File& videoFile);
    void resized() override;
    void timerCallback() override;

private:
    TransportController& transportController;
    std::unique_ptr<juce::Component> videoComponent;
    bool isVideoLoaded = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VideoTrackComponent)
};
