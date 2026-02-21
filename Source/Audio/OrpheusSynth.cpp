#include "OrpheusSynth.h"

OrpheusVoice::OrpheusVoice()
{
    adsrParams.attack = 0.01f;
    adsrParams.decay = 0.1f;
    adsrParams.sustain = 0.7f;
    adsrParams.release = 0.2f;
    adsr.setParameters(adsrParams);
}

bool OrpheusVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<OrpheusSound*>(sound) != nullptr;
}

void OrpheusVoice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int /*currentPitchWheelPosition*/)
{
    level = velocity * 0.25;
    frequency = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
    phase = 0.0;
    phaseIncrement = (frequency * 2.0 * juce::MathConstants<double>::pi) / getSampleRate();
    
    adsr.noteOn();
}

void OrpheusVoice::stopNote(float /*velocity*/, bool allowTailOff)
{
    if (allowTailOff)
    {
        adsr.noteOff();
    }
    else
    {
        adsr.reset();
        clearCurrentNote();
    }
}

void OrpheusVoice::pitchWheelMoved(int) {}
void OrpheusVoice::controllerMoved(int, int) {}

void OrpheusVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (!adsr.isActive()) return;

    for (int i = 0; i < numSamples; ++i)
    {
        double sample = 0.0;
        
        switch (waveform)
        {
            case 0: // Sine
                sample = std::sin(phase);
                break;
            case 1: // Saw
                sample = (phase / juce::MathConstants<double>::pi) - 1.0;
                break;
            case 2: // Square
                sample = (phase < juce::MathConstants<double>::pi) ? 1.0 : -1.0;
                break;
            case 3: // Noise
                sample = (juce::Random::getSystemRandom().nextFloat() * 2.0f) - 1.0f;
                break;
        }
        
        float adsrVal = adsr.getNextSample();
        float finalSample = (float)(sample * level * adsrVal);
        
        for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
            outputBuffer.addSample(ch, startSample + i, finalSample);
            
        phase = std::fmod(phase + phaseIncrement, 2.0 * juce::MathConstants<double>::pi);
        
        if (!adsr.isActive())
        {
            clearCurrentNote();
            break;
        }
    }
}

OrpheusSynth::OrpheusSynth()
{
    for (int i = 0; i < 16; ++i)
        addVoice(new OrpheusVoice());
        
    addSound(new OrpheusSound());
}

void OrpheusSynth::setup(double sampleRate)
{
    setCurrentPlaybackSampleRate(sampleRate);
}
