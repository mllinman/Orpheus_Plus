#pragma once
#include <JuceHeader.h>
#include "../Timeline/MidiClip.h"

class ScoreViewComponent : public juce::Component
{
public:
    ScoreViewComponent();
    ~ScoreViewComponent() override;

    void setMidiClip(MidiClip* clip);
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void drawStaff(juce::Graphics& g, int yCenter);
    void drawNote(juce::Graphics& g, int pitch, int staffY, int xPos);

    MidiClip* activeClip = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScoreViewComponent)
};
