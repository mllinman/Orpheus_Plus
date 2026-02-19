#pragma once
#include <JuceHeader.h>

class Clip
{
public:
    enum class Type { Audio, Midi };

    Clip(Type t) : type(t) {}
    virtual ~Clip() = default;

    Type getType() const { return type; }
    
    // Common properties
    double startTime = 0.0;    // seconds
    double duration  = 4.0;    // seconds
    double offset    = 0.0;    // start offset within source
    juce::String name;
    juce::Colour colour;
    bool selected = false;

    virtual void paint(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Rectangle<int> clipArea) = 0;

private:
    Type type;
};
