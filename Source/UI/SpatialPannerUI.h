#pragma once
#include <JuceHeader.h>
#include "../Audio/AudioEngine.h"

class SpatialPannerUI : public juce::Component, public juce::Timer
{
public:
    SpatialPannerUI(AudioEngine& engineToUse, int trackIdx);
    ~SpatialPannerUI() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;

    void setTrackIndex(int trackIdx) { trackIndex = trackIdx; }

private:
    void updateSpatialPositionFromMouse(const juce::MouseEvent& e);

    AudioEngine& engine;
    int trackIndex;

    juce::ToggleButton enableButton { "Spatial Enabled" };
    juce::Slider elevationSlider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
    juce::Label elevationLabel { "", "Elevation" };

    float currentAzimuth { 0.0f };
    float currentDistance { 1.0f };
    float currentElevation { 0.0f };
    bool isSpatialEnabled { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpatialPannerUI)
};
