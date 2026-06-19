#pragma once
#include <JuceHeader.h>
#include "../PitchCorrection/VocalSuiteProcessor.h"

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

    std::atomic<double> currentPlayhead { 0.0 };

public:
    std::function<void(juce::AudioBuffer<float>&, double)> renderAudioCallback;
    void setPlayhead(double pos) { currentPlayhead.store(pos); }

    // Smoothed values for ramp
    juce::LinearSmoothedValue<float> smoothVolume { 1.0f };
    juce::LinearSmoothedValue<float> smoothPan    { 0.0f };

    // Insert FX chain
    juce::OwnedArray<juce::AudioProcessor> insertFX;
    std::unique_ptr<VocalSuiteProcessor> vocalSuite;

public:
    void addInsertFX(std::unique_ptr<juce::AudioProcessor> p) 
    {
        p->prepareToPlay(getSampleRate(), getBlockSize());
        insertFX.add(p.release());
    }
    
    void clearInsertFX()
    {
        insertFX.clear();
    }
    
    juce::OwnedArray<juce::AudioProcessor>& getInsertFX() { return insertFX; }

    void setSweetener(float amount) { sweetenerAmount.store(amount); updateSweetener(); }
    float getSweetener() const { return sweetenerAmount.load(); }

    float getPeakL() const { return peakL.load(); }
    float getPeakR() const { return peakR.load(); }

private:
    void updateSweetener();

    std::atomic<float> sweetenerAmount { 0.0f };

    juce::dsp::Compressor<float> compressor;
    using Filter = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>;
    Filter highShelf;
    Filter lowShelf;

    std::atomic<float> peakL { 0.0f };
    std::atomic<float> peakR { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackProcessor)
};
