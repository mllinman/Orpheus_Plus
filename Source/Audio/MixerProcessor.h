#pragma once
#include <JuceHeader.h>

class MasteringModule;

class MixerProcessor : public juce::AudioProcessor
{
public:
    MixerProcessor();
    ~MixerProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }

    const juce::String getName() const override { return "Master Mixer"; }
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

    void setMasterVolume(float vol);
    float getMasterVolume() const { return masterVolume.load(); }

    // Metering
    float getRMSLeft() const  { return rmsLeft.load(); }
    float getRMSRight() const { return rmsRight.load(); }

    void setMasteringModule(MasteringModule* module) { masteringModule = module; }

private:
    MasteringModule* masteringModule = nullptr;
    std::atomic<float> masterVolume { 1.0f };
    juce::LinearSmoothedValue<float> smoothVolume { 1.0f };

    std::atomic<float> rmsLeft { 0.0f };
    std::atomic<float> rmsRight { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerProcessor)
};
