#pragma once
#include <JuceHeader.h>

/**
 * A simple polyphonic synthesizer voice for Orpheus Plus.
 */
class OrpheusVoice : public juce::SynthesiserVoice
{
public:
    OrpheusVoice();
    
    bool canPlaySound(juce::SynthesiserSound* sound) override;
    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void pitchWheelMoved(int newPitchWheelValue) override;
    void controllerMoved(int controllerNumber, int newControllerValue) override;
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

private:
    double level { 0.0 };
    double frequency { 0.0 };
    double phase { 0.0 };
    double phaseIncrement { 0.0 };
    
    juce::ADSR adsr;
    juce::ADSR::Parameters adsrParams;
    
    // Waveform type (0: Sine, 1: Saw, 2: Square, 3: Noise)
    int waveform { 1 };
};

/**
 * Standard JUCE SynthesiserSound implementation.
 */
class OrpheusSound : public juce::SynthesiserSound
{
public:
    OrpheusSound() {}
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

/**
 * The main synthesizer class.
 */
class OrpheusSynth : public juce::Synthesiser
{
public:
    OrpheusSynth();
    void setup(double sampleRate);
};
