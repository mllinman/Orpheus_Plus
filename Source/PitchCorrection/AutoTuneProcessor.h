#pragma once
#include <JuceHeader.h>

//==============================================================================
// Real-time auto-tune / pitch correction processor
// Uses YIN pitch detection + phase vocoder pitch shifting
// For production quality, integrate Rubber Band Library
//
class AutoTuneProcessor : public juce::AudioProcessor
{
public:
    AutoTuneProcessor();
    ~AutoTuneProcessor() override;

    //── AudioProcessor interface ─────────────────────────────────────────────
    const juce::String getName() const override { return "AutoTune"; }
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
    void processBlock(juce::AudioBuffer<float>& buffer,
                      juce::MidiBuffer&) override;

    //── Parameters ──────────────────────────────────────────────────────────
    void setEnabled(bool e)           { enabled = e; }
    void setSpeed(float s)            { speed = juce::jlimit(0.0f, 1.0f, s); } // 0=natural, 1=T-Pain
    void setKey(int semitone)         { key = semitone; }  // 0=C, 1=C# etc.
    void setScale(int scaleType)      { scale = scaleType; } // 0=chromatic, 1=major, 2=minor
    void setRobotVoiceAmount(float r) { robotAmount = r; }   // 0-1
    void setFormantShift(float f)     { formantShift = f; }  // in semitones

    float getDetectedPitch() const { return detectedPitch.load(); }
    float getCorrectedPitch() const { return correctedPitch.load(); }

private:
    float detectPitchYIN(const float* buffer, int numSamples, double sampleRate);
    float findClosestScalePitch(float detectedHz);
    void shiftPitch(juce::AudioBuffer<float>& buffer, float semitones);

    // Scale definitions (semitones within octave)
    static constexpr int CHROMATIC[12] = {0,1,2,3,4,5,6,7,8,9,10,11};
    static constexpr int MAJOR[7]      = {0,2,4,5,7,9,11};
    static constexpr int MINOR[7]      = {0,2,3,5,7,8,10};

    bool  enabled      = false;
    float speed        = 0.5f;
    int   key          = 0;
    int   scale        = 1;     // major
    float robotAmount  = 0.0f;
    float formantShift = 0.0f;

    std::atomic<float> detectedPitch  { 0.0f };
    std::atomic<float> correctedPitch { 0.0f };

    double currentSampleRate = 44100.0;
    int    currentBlockSize  = 512;

    // Phase vocoder state
    std::vector<float> inputBuffer;
    std::vector<float> outputBuffer;
    std::vector<float> phaseAccumulator;
    std::vector<float> lastPhase;
    std::vector<float> outputAccumulator;
    int overlapFactor = 4;
    int fftSize       = 2048;
    std::unique_ptr<juce::dsp::FFT> fft;

    float pitchSmoothed = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutoTuneProcessor)
};
