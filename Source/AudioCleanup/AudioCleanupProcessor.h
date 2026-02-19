#pragma once
#include <JuceHeader.h>

//==============================================================================
class AudioCleanupProcessor : public juce::AudioProcessor
{
public:
    AudioCleanupProcessor();
    ~AudioCleanupProcessor() override;

    const juce::String getName() const override { return "Audio Cleanup"; }
    bool  acceptsMidi()  const override { return false; }
    bool  producesMidi() const override { return false; }
    bool  isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int  getNumPrograms()  override { return 1; }
    int  getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }

    void prepareToPlay(double sampleRate, int blockSize) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override;

    //── Enable/Disable modules ───────────────────────────────────────────────
    void setNoiseReductionEnabled(bool e)  { noiseEnabled = e; }
    void setDeClickEnabled(bool e)         { deClickEnabled = e; }
    void setDeEsserEnabled(bool e)         { deEsserEnabled = e; }
    void setHumRemovalEnabled(bool e)      { humEnabled = e; }
    void setDCOffsetEnabled(bool e)        { dcEnabled = e; }

    //── Noise reduction ─────────────────────────────────────────────────────
    void captureNoiseProfile();   // Learn noise floor from current audio
    void setNoiseReductionAmount(float amount); // 0-1
    void setNoiseGateThreshold(float threshDB);

    //── De-esser ─────────────────────────────────────────────────────────────
    void setDeEsserFrequency(float hz)    { deEsserFreq = hz; }
    void setDeEsserThreshold(float threshDB) { deEsserThresh = threshDB; }
    void setDeEsserRange(float rangeDB)   { deEsserRange = rangeDB; }

    //── Hum removal ──────────────────────────────────────────────────────────
    void setHumFrequency(float hz) { humFreq = hz; } // 50 or 60 Hz
    void setHumHarmonics(int n)    { humHarmonics = n; }

    // Offline denoise using RNNoise (better quality)
    static bool processFileWithRNNoise(const juce::File& inputFile,
                                       const juce::File& outputFile,
                                       float amount = 1.0f);

private:
    void processNoiseReduction(juce::AudioBuffer<float>& buffer);
    void processDeClick(juce::AudioBuffer<float>& buffer);
    void processDeEsser(juce::AudioBuffer<float>& buffer);
    void processHumRemoval(juce::AudioBuffer<float>& buffer);
    void processDCOffset(juce::AudioBuffer<float>& buffer);

    // Spectral noise reduction
    void updateNoiseMask(const juce::AudioBuffer<float>& buffer);
    void applyWienerFilter(std::vector<std::complex<float>>& spectrum);

    bool noiseEnabled   = false;
    bool deClickEnabled = false;
    bool deEsserEnabled = false;
    bool humEnabled     = false;
    bool dcEnabled      = true;

    float noiseAmount     = 0.6f;
    float noiseGateThresh = -60.0f;

    float deEsserFreq   = 7500.0f;
    float deEsserThresh = -20.0f;
    float deEsserRange  = -12.0f;

    float humFreq      = 50.0f;
    int   humHarmonics = 3;

    double currentSampleRate = 44100.0;
    int    fftSize           = 2048;

    // Noise profile (frequency domain)
    std::vector<float> noiseProfile;
    bool               hasNoiseProfile = false;

    // FFT
    std::unique_ptr<juce::dsp::FFT> fft;
    std::vector<float> fftBuffer;
    std::vector<float> windowFunc;

    // DC offset history
    float dcHistL = 0.0f, dcHistR = 0.0f;

    // De-esser state
    struct BiquadState { float z1 = 0, z2 = 0; };
    BiquadState deEsserFilterL, deEsserFilterR;
    float deEsserEnvL = 0.0f, deEsserEnvR = 0.0f;

    // Notch filters for hum (one per harmonic, per channel)
    std::vector<juce::dsp::IIR::Filter<float>> humFiltersL, humFiltersR;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioCleanupProcessor)
};
