#pragma once
#include <JuceHeader.h>

class AIHumanizerProcessor : public juce::AudioProcessor
{
public:
    AIHumanizerProcessor();
    ~AIHumanizerProcessor() override;

    const juce::String getName() const override { return "AI Humanizer"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
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
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    // Process an entire file offline (Option B)
    void processFileOffline(const juce::File& inputFile, const juce::File& outputFile, class AppState* appState, class AudioEngine* engine);

    // Parameters (0.0 to 1.0)
    void setWarmth(float amount) { warmth = amount; }
    void setFlutter(float amount) { flutter = amount; }
    void setNoiseFloor(float amount) { noiseFloor = amount; }
    void setDeChatter(float amount) { deChatter = amount; }

private:
    void applySaturation(juce::AudioBuffer<float>& buffer);
    void applyWowAndFlutter(juce::AudioBuffer<float>& buffer);
    void applyNoiseInjection(juce::AudioBuffer<float>& buffer);
    void applyDeChatter(juce::AudioBuffer<float>& buffer);

    float warmth { 0.5f };
    float flutter { 0.3f };
    float noiseFloor { 0.2f };
    float deChatter { 0.5f };

    double currentSampleRate { 44100.0 };

    // DSP State
    juce::dsp::DelayLine<float> delayLine { 44100 }; // 1 sec max
    float lfoPhase { 0.0f };
    juce::Random randomGenerator;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AIHumanizerProcessor)
};
