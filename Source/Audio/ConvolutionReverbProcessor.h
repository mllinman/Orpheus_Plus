#pragma once
#include <JuceHeader.h>

/**
 * A simple wrapper around juce::dsp::Convolution for use in the Orpheus engine.
 */
class ConvolutionReverbProcessor : public juce::AudioProcessor
{
public:
    ConvolutionReverbProcessor();
    ~ConvolutionReverbProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    // Convolution specific
    void loadImpulseResponse(const juce::File& file);
    void setDryWet(float wetAmount); // 0.0 to 1.0

    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    const juce::String getName() const override { return "Convolution Reverb"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

private:
    juce::dsp::Convolution convolution;
    float dryWet { 0.3f };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConvolutionReverbProcessor)
};
