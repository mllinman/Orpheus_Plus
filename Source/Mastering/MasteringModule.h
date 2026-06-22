#pragma once
#include <JuceHeader.h>
#include "../Audio/AudioEngine.h"
#include "MultibandCompressor.h"
#include "LinearPhaseEQ.h"
#include "LUFSMeter.h"
#include "../Audio/ConvolutionReverbProcessor.h"
#include "LatentMatchEQ.h"
#include <functional>

//==============================================================================
// Linear Phase EQ Band
struct EQBand
{
    enum class Type { LowShelf, Peak, HighShelf, LowPass, HighPass };
    Type   type      = Type::Peak;
    double frequency = 1000.0;
    double gain      = 0.0;    // dB
    double q         = 0.707;
    bool   enabled   = true;
};

//==============================================================================
class MasteringModule : public juce::Component
{
public:
    explicit MasteringModule(AudioEngine& engine);
    ~MasteringModule() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    // Process a buffer through the mastering chain
    void prepare(const juce::dsp::ProcessSpec& spec);
    void processBlock(juce::AudioBuffer<float>& buffer, double sampleRate);

    // Chain enable/disable
    void setMidSideEnabled(bool e)       { midSideEnabled = e; updateChain(); }
    void setEQEnabled(bool e)            { eqEnabled = e; updateChain(); }
    void setMultibandCompEnabled(bool e) { mbCompEnabled = e; updateChain(); }
    void setSaturationEnabled(bool e)    { satEnabled = e; updateChain(); }
    void setReverbEnabled(bool e)        { reverbEnabled = e; updateChain(); }
    void setLimiterEnabled(bool e)       { limiterEnabled = e; updateChain(); }
    void setAutoLUFSEnabled(bool e)      { autoLufsEnabled = e; updateChain(); }
    void setDynamicEQEnabled(bool e)     { dynamicEqEnabled = e; updateChain(); }
    void setMatchEQEnabled(bool e)       { matchEqEnabled = e; updateChain(); }
    void setTargetLUFS(float target)     { targetLUFS = target; }

    void analyzeTrack();
    void finishAnalysis();
    void loadMatchEQReference();
    void loadReferenceFile(const juce::File& file) { latentMatchEQ.analyzeReferenceTrack(file); }
    
    std::function<void()> onPhaseAlign;

    // EQ
    void setEQBand(int band, double freq, double gainDB, double q, EQBand::Type type);

    // Multiband compressor
    void setMBThreshold(int band, float threshDB);
    void setMBRatio(int band, float ratio);
    void setMBAttack(int band, float ms);
    void setMBRelease(int band, float ms);

    // Limiter
    void setLimiterCeiling(float ceilingDB) { limiterCeiling = ceilingDB; }
    void setLimiterRelease(float ms)        { limiterRelease = ms; }

    void forceLufsTarget(float targetLufs, float targetTruePeak);
    void disableForceLufsTarget();

    // Metering
    float getLUFS()       const { return currentLUFS.load(); }
    float getTruePeak()   const { return truePeak.load(); }
    float getCorrelation() const { return correlation.load(); }

    void loadReverbIR(const juce::File& file) { reverbProcessor.loadImpulseResponse(file); }
    void setReverbDryWet(float wet)           { reverbProcessor.setDryWet(wet); }

private:
    void updateChain();
    void processEQ(juce::AudioBuffer<float>& buffer);
    void processMidSide(juce::AudioBuffer<float>& buffer, bool encode);
    void processMultibandComp(juce::AudioBuffer<float>& buffer);
    void processSaturation(juce::AudioBuffer<float>& buffer);
    void processReverb(juce::AudioBuffer<float>& buffer);
    void processLimiter(juce::AudioBuffer<float>& buffer);
    void updateMeters(const juce::AudioBuffer<float>& buffer);

    void buildUI();

    AudioEngine& audioEngine;

    // Chain flags
    bool msToggleEnabled = false; // Internal flag rename or keep? The header had midSideEnabled
    bool midSideEnabled = false;
    bool eqEnabled      = true;
    bool mbCompEnabled  = true;
    bool satEnabled     = false;
    bool reverbEnabled  = false;
    bool limiterEnabled = true;
    bool autoLufsEnabled = false;
    bool dynamicEqEnabled = false;
    bool matchEqEnabled   = false;
    float targetLUFS      = -14.0f;
    float autoGainDb      = 0.0f;

    ConvolutionReverbProcessor reverbProcessor;
    LatentMatchEQ latentMatchEQ;

    // EQ
    static constexpr int NUM_EQ_BANDS = 8;
    std::array<EQBand, NUM_EQ_BANDS> eqBands;
    LinearPhaseEQ linearPhaseEQ;

    // Limiter state
    float limiterCeiling = -0.1f; // dBFS
    float limiterRelease = 50.0f; // ms
    float limiterEnvL = 0.0f, limiterEnvR = 0.0f;

    // Multiband comp state
    // Multiband comp
    MultibandCompressor multibandComp;
    bool mbCompInitialized = false;

    // Metering
    std::atomic<float> currentLUFS  { -70.0f };
    std::atomic<float> truePeak     { -70.0f };
    std::atomic<float> correlation  {  1.0f  };
    LUFSMeter lufsMeter;

    double currentSampleRate = 44100.0;

    // Analysis
    std::atomic<bool> isAnalyzing { false };
    int framesAnalyzed = 0;
    float peakAccumulator = 0.0f;

    // UI Controls
    juce::ToggleButton eqToggle      { "EQ" };
    juce::ToggleButton compToggle    { "COMP" };
    juce::ToggleButton msToggle      { "M/S" };
    juce::ToggleButton satToggle     { "SAT" };
    juce::ToggleButton reverbToggle  { "REVERB" };
    juce::ToggleButton limiterToggle { "LIMIT" };
    juce::ToggleButton autoLufsToggle { "AUTO-LUFS" };
    juce::ToggleButton dynEqToggle    { "DYN-EQ" };
    juce::ToggleButton matchEqToggle  { "MATCH-EQ" };
    juce::TextButton   analyzeButton  { "Analyze Track" };
    juce::TextButton   loadMatchEQBtn { "Load Reference" };
    juce::TextButton   phaseAlignBtn  { "Phase Align All Tracks" };

    juce::Slider lufsSlider;  // display only
    juce::Label  lufsLabel;
    juce::Label  truePeakLabel;
    juce::Label  correlationLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasteringModule)
};
