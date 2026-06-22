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

    //── Original Parameters (0.0 to 1.0) ─────────────────────────────────────
    void setWarmth(float amount)   { warmth = amount; }
    void setFlutter(float amount)  { flutter = amount; }
    void setNoiseFloor(float amount) { noiseFloor = amount; }
    void setDeChatter(float amount)  { deChatter = amount; }

    //── New Parameters (0.0 to 1.0) ──────────────────────────────────────────
    void setMicroTiming(float amount)      { microTiming = amount; }
    void setStereoWidth(float amount)       { stereoWidth = amount; }
    void setHarmonicExciter(float amount)   { harmonicExciter = amount; }
    void setDynamicBreathing(float amount)  { dynamicBreathing = amount; }

    //── Preset application ───────────────────────────────────────────────────
    enum class Preset { Subtle, WarmAnalog, LiveFeel, FullTreatment };
    void applyPreset(Preset preset);

private:
    //── Original DSP stages ──────────────────────────────────────────────────
    void applySaturation(juce::AudioBuffer<float>& buffer);
    void applyWowAndFlutter(juce::AudioBuffer<float>& buffer);
    void applyNoiseInjection(juce::AudioBuffer<float>& buffer);
    void applyDeChatter(juce::AudioBuffer<float>& buffer);

    //── New DSP stages ───────────────────────────────────────────────────────
    void applyMicroTimingJitter(juce::AudioBuffer<float>& buffer);
    void applyStereoDecorrelation(juce::AudioBuffer<float>& buffer);
    void applyHarmonicExcitement(juce::AudioBuffer<float>& buffer);
    void applyDynamicBreathing(juce::AudioBuffer<float>& buffer);

    //── Original parameters ──────────────────────────────────────────────────
    float warmth      { 0.5f };
    float flutter     { 0.3f };
    float noiseFloor  { 0.2f };
    float deChatter   { 0.5f };

    //── New parameters ───────────────────────────────────────────────────────
    float microTiming      { 0.3f };
    float stereoWidth      { 0.3f };
    float harmonicExciter  { 0.2f };
    float dynamicBreathing { 0.2f };

    double currentSampleRate { 44100.0 };

    //── Original DSP State ───────────────────────────────────────────────────
    juce::dsp::DelayLine<float> delayLine { 44100 }; // 1 sec max
    float lfoPhase { 0.0f };
    juce::Random randomGenerator;

    //── Micro-timing jitter state ────────────────────────────────────────────
    // Per-channel interpolated delay lines for sub-sample jitter
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> jitterDelayL { 512 };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> jitterDelayR { 512 };
    float jitterTargetL { 0.0f };
    float jitterTargetR { 0.0f };
    float jitterCurrentL { 0.0f };
    float jitterCurrentR { 0.0f };
    int   jitterUpdateCounter { 0 };
    int   jitterUpdateInterval { 4410 }; // ~100ms at 44.1kHz

    //── Stereo decorrelation state ───────────────────────────────────────────
    // Schroeder allpass chain coefficients and state
    static constexpr int kNumAllpass = 4;
    struct AllpassState {
        float buffer { 0.0f };
    };
    AllpassState allpassL[kNumAllpass];
    AllpassState allpassR[kNumAllpass];
    static constexpr float allpassCoeffs[kNumAllpass] = { 0.3f, -0.4f, 0.5f, -0.35f };
    static constexpr int allpassDelays[kNumAllpass] = { 13, 17, 23, 31 }; // prime delay taps
    // Small circular buffers for allpass delays
    std::array<std::vector<float>, kNumAllpass> allpassBufL;
    std::array<std::vector<float>, kNumAllpass> allpassBufR;
    std::array<int, kNumAllpass> allpassIdxL {};
    std::array<int, kNumAllpass> allpassIdxR {};

    //── Harmonic exciter state ───────────────────────────────────────────────
    // DC blocker for post-exciter (removes DC offset from waveshaping)
    float dcBlockerX1L { 0.0f }, dcBlockerY1L { 0.0f };
    float dcBlockerX1R { 0.0f }, dcBlockerY1R { 0.0f };

    //── Dynamic breathing state ──────────────────────────────────────────────
    // Smooth random gain modulation using summed slow LFOs
    float breathPhase1 { 0.0f };
    float breathPhase2 { 0.0f };
    float breathPhase3 { 0.0f };

    //── Pink noise generator (Voss-McCartney) ────────────────────────────────
    float pinkNoiseValue { 0.0f };
    static constexpr int kPinkNoiseOctaves = 8;
    float pinkRows[kPinkNoiseOctaves] {};
    int   pinkRunningSum { 0 };
    int   pinkIndex { 0 };
    float generatePinkNoiseSample();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AIHumanizerProcessor)
};
