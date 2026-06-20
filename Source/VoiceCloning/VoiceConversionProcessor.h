#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <vector>
#include <memory>

// Define this if compiling with ONNX Runtime available
// #define USE_ONNX_RUNTIME 1

#if USE_ONNX_RUNTIME
#include <onnxruntime_cxx_api.h>
#endif

class VoiceConversionProcessor : public juce::AudioProcessor
{
public:
    VoiceConversionProcessor();
    ~VoiceConversionProcessor() override;

    // AudioProcessor overrides
    const juce::String getName() const override { return "VoiceConversion"; }
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
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    // Timbre Control Parameters
    void setEnabled(bool e) { enabled.store(e); }
    void setModelPath(const juce::String& path);
    void setSpeakerEmbedding(const std::vector<float>& embedding);
    void setPitchShift(float semitones) { pitchShift = semitones; }
    void setTimbreMix(float mix) { timbreMix = juce::jlimit(0.0f, 1.0f, mix); }

    // Neural Restorer Parameters
    void setHumanizeAmount(float amt) { humanizeAmount.store(juce::jlimit(0.0f, 1.0f, amt)); }
    void setPreserveVibrato(bool preserve) { preserveVibrato.store(preserve); }
    void setScaleLock(int rootNote, int scaleType); // For F0 smoothing

private:
    float detectPitchYIN(const float* samples, int numSamples, double sr);
    float smoothF0Contour(float currentF0, float targetF0);

    std::atomic<bool> enabled { false };
    float pitchShift = 0.0f;
    float timbreMix = 1.0f; // 0 = original, 1 = fully cloned
    
    std::atomic<float> humanizeAmount { 0.5f };
    std::atomic<bool> preserveVibrato { true };

    juce::dsp::IIR::Filter<float> highPassFilter; // Extractor for unvoiced breath/noise


    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;

    std::vector<float> speakerEmbeddingData;
    juce::String currentModelPath;

#if USE_ONNX_RUNTIME
    std::unique_ptr<Ort::Env> ortEnv;
    std::unique_ptr<Ort::Session> ortSession;
    Ort::MemoryInfo memoryInfo {nullptr};
#endif

    // Feature extraction buffers (Pitch, PPG)
    std::vector<float> inputBuffer;
    std::vector<float> outputBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoiceConversionProcessor)
};
