#pragma once
#include <JuceHeader.h>

class TrackProcessor : public juce::AudioProcessor
{
public:
    TrackProcessor();
    ~TrackProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }

    const juce::String getName() const override { return "Track Processor"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

    // Track controls
    void setVolume(float gain);
    void setPan(float pan);
    void setMute(bool shouldMute);
    void setSolo(bool shouldSolo);
    
    float getVolume() const { return currentVolume.load(); }
    float getPan() const    { return currentPan.load(); }

private:
    std::atomic<float> currentVolume { 1.0f };
    std::atomic<float> currentPan    { 0.0f };
    std::atomic<bool>  muted        { false };
    std::atomic<bool>  soloed       { false };

    juce::dsp::Gain<float> gain;
    juce::dsp::Panner<float> panner;
    
    // Smoothed values for ramp
    juce::LinearSmoothedValue<float> smoothVolume { 1.0f };
    juce::LinearSmoothedValue<float> smoothPan    { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackProcessor)
};
