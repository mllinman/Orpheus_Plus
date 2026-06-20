#pragma once
#include <JuceHeader.h>

/**
 * A basic spatial reverb that applies decorrelated reverberation
 * to a 12-channel surround/spatial audio stream.
 */
class SpatialReverbProcessor : public juce::AudioProcessor
{
public:
    SpatialReverbProcessor();
    ~SpatialReverbProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }

    const juce::String getName() const override { return "Spatial Reverb"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

    void setRoomSize(float size);
    void setDamping(float damping);
    void setWetLevel(float wet);
    void setDryLevel(float dry);

private:
    std::array<juce::dsp::Reverb, 12> reverbs;
    juce::Reverb::Parameters reverbParams;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpatialReverbProcessor)
};
