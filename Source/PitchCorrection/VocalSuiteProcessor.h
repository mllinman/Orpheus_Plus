#pragma once
#include <JuceHeader.h>

//==============================================================================
// Vocal Suite Processor
// Comprehensive vocal processing engine featuring:
// - Real-time Pitch Correction (YIN + Phase Vocoder)
// - Formant Shifting (Spectral envelope scaling)
// - Vocal Doubler / Harmonizer
// - Real-time automation targets
//==============================================================================
class VocalSuiteProcessor : public juce::AudioProcessor
{
public:
    VocalSuiteProcessor();
    ~VocalSuiteProcessor() override;

    //── AudioProcessor interface ─────────────────────────────────────────────
    const juce::String getName() const override { return "VocalSuite"; }
    bool acceptsMidi()  const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int    getNumPrograms() override    { return 1; }
    int    getCurrentProgram() override { return 0; }
    void   setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }

    void prepareToPlay(double sampleRate, int blockSize) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override;

    //── Parameters ──────────────────────────────────────────────────────────
    void setEnabled(bool e)           { enabled = e; }
    void setRetuneSpeed(float s)      { retuneSpeed = juce::jlimit(0.0f, 1.0f, s); } // 0=Natural, 1=Robotic
    void setKey(int semitone)         { key = semitone; }
    void setScale(int scaleType)      { scale = scaleType; }
    void setFormantShift(float f)     { formantShift = juce::jlimit(-12.0f, 12.0f, f); } // semitones
    void setDoublerAmount(float d)    { doublerAmount = juce::jlimit(0.0f, 1.0f, d); }
    void setHarmonyInterval(int h)    { harmonyInterval = h; } // e.g. +3, -5

    float getDetectedPitch() const { return detectedPitch.load(); }
    float getCorrectedPitch() const { return correctedPitch.load(); }

private:
    float detectPitchYIN(const float* buffer, int numSamples, double sampleRate);
    float findClosestScalePitch(float detectedHz);
    
    // Advanced DSP Core
    void processPhaseVocoder(juce::AudioBuffer<float>& buffer, float pitchShiftSemitones, float formantShiftSemitones);
    void applyDoubler(juce::AudioBuffer<float>& buffer);
    void applyHarmony(juce::AudioBuffer<float>& buffer);

    static constexpr int CHROMATIC[12] = {0,1,2,3,4,5,6,7,8,9,10,11};
    static constexpr int MAJOR[7]      = {0,2,4,5,7,9,11};
    static constexpr int MINOR[7]      = {0,2,3,5,7,8,10};

    // State
    bool  enabled         = false;
    float retuneSpeed     = 0.5f;
    int   key             = 0;
    int   scale           = 1;
    float formantShift    = 0.0f;
    float doublerAmount   = 0.0f;
    int   harmonyInterval = 0;

    std::atomic<float> detectedPitch  { 0.0f };
    std::atomic<float> correctedPitch { 0.0f };

    double currentSampleRate = 44100.0;
    int    currentBlockSize  = 512;

    // Phase vocoder structures
    std::vector<float> inputBuffer;
    std::vector<float> outputBuffer;
    std::vector<float> phaseAccumulator;
    std::vector<float> lastPhase;
    std::vector<float> outputAccumulator;
    int overlapFactor = 4;
    int fftSize       = 2048;
    std::unique_ptr<juce::dsp::FFT> fft;

    // Delay lines for Doubler/Harmonizer
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayLineL { 44100 };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayLineR { 44100 };
    juce::LinearSmoothedValue<float> smoothedLfoPhase { 0.0f };

    float pitchSmoothed = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VocalSuiteProcessor)
};
