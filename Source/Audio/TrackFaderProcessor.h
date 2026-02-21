#pragma once
#include <JuceHeader.h>

/**
    Handles volume, pan, sweetener (EQ/Comp strip), and metering for a track.
    This acts as the final node for a track in the AudioProcessorGraph.
*/
class TrackFaderProcessor : public juce::AudioProcessor
{
public:
    TrackFaderProcessor();
    ~TrackFaderProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }

    const juce::String getName() const override { return "Track Fader"; }
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

    // Controls
    void setVolume(float gain);
    void setPan(float pan);
    void setMute(bool shouldMute);
    void setSweetener(float amount);

    float getVolume() const { return currentVolume.load(); }
    float getPan() const    { return currentPan.load(); }
    float getSweetener() const { return sweetenerAmount.load(); }

    float getPeakL() const { return peakL.load(); }
    float getPeakR() const { return peakR.load(); }

private:
    void updateSweetener();

    std::atomic<float> currentVolume { 1.0f };
    std::atomic<float> currentPan    { 0.0f };
    std::atomic<bool>  muted        { false };
    std::atomic<float> sweetenerAmount { 0.0f };

    juce::LinearSmoothedValue<float> smoothVolume { 1.0f };
    juce::LinearSmoothedValue<float> smoothPan    { 0.0f };

    // Sweetener DSP
    juce::dsp::Compressor<float> compressor;
    using Filter = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>;
    Filter highShelf;
    Filter lowShelf;

    // Metering
    std::atomic<float> peakL { 0.0f };
    std::atomic<float> peakR { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackFaderProcessor)
};
