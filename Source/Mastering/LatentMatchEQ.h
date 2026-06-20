#pragma once
#include <JuceHeader.h>
#include <vector>

class LatentMatchEQ : public juce::AudioProcessor
{
public:
    LatentMatchEQ();
    ~LatentMatchEQ() override = default;

    // Standard AudioProcessor Overrides
    const juce::String getName() const override { return "LatentMatchEQ"; }
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

    // Learns the acoustic fingerprint of a reference file (Latent Space Embedding)
    void analyzeReferenceTrack(const juce::File& audioFile);

    // Amount to apply the matched EQ curve (0.0 to 1.0)
    void setMatchAmount(float amount) { matchAmount.store(amount); }

private:
    std::atomic<float> matchAmount { 1.0f };
    juce::dsp::Convolution convolutionFilter; // We apply the learned EQ via an FIR IR
    
    bool hasAnalyzedReference = false;
    double currentSampleRate = 44100.0;
};
