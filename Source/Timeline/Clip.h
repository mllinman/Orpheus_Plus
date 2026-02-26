#pragma once
#include <JuceHeader.h>

class Clip
{
public:
    enum class Type { Audio, Midi };

    Clip(Type t) : clipType(t) {}
    virtual ~Clip() = default;

    Type getType() const { return clipType; }
    
    // Common properties
    double startTime = 0.0;    // seconds
    double duration  = 4.0;    // seconds
    double offset    = 0.0;    // start offset within source
    juce::String name;
    juce::Colour colour;
    bool selected = false;

    // Manipulation properties
    enum class FadeCurve { Linear, Exponential, S_Curve };
    double    fadeIn    = 0.0; // seconds
    double    fadeOut   = 0.0; // seconds
    FadeCurve fadeInCurve  = FadeCurve::Linear;
    FadeCurve fadeOutCurve = FadeCurve::Linear;
    double    gain      = 1.0; // linear gain
    bool      muted     = false;

    virtual void paint(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Rectangle<int> clipArea) = 0;
    virtual std::unique_ptr<Clip> clone() const = 0;

private:
    Type clipType;
};
