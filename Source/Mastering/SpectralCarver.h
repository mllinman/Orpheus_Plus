#pragma once
#include <JuceHeader.h>
#include <vector>

class SpectralCarver : public juce::AudioProcessor
{
public:
    SpectralCarver();
    ~SpectralCarver() override = default;

    const juce::String getName() const override { return "SpectralCarver"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    // The mask input (e.g. Vocals) that will drive the ducking
    void processSidechainMask(const juce::AudioBuffer<float>& maskBuffer);

    void setCarveAmount(float amount) { carveAmount.store(amount); }

private:
    std::atomic<float> carveAmount { 0.5f };
    
    // In a full implementation, we use juce::dsp::FFT to analyze both signals,
    // subtract the mask bins from the target bins, and run an inverse FFT.
    juce::dsp::FFT forwardFFT { 11 }; // 2048 points
    juce::dsp::FFT inverseFFT { 11 };
    
    std::vector<float> sidechainMagnitudes;
};
