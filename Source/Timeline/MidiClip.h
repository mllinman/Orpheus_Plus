#pragma once
#include "Clip.h"

class MidiClip : public Clip
{
public:
    MidiClip(double start, double duration);
    ~MidiClip() override;

    void paint(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Rectangle<int> clipArea) override;

    juce::MidiMessageSequence midiData;
    
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiClip)
};
