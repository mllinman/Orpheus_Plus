#include "VoiceConversionProcessor.h"
#include <cmath>

VoiceConversionProcessor::VoiceConversionProcessor()
{
#if USE_ONNX_RUNTIME
    ortEnv = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "VoiceCloning");
    memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
#endif
}

VoiceConversionProcessor::~VoiceConversionProcessor()
{
}

void VoiceConversionProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;

    inputBuffer.assign(samplesPerBlock, 0.0f);
    outputBuffer.assign(samplesPerBlock, 0.0f);
}

void VoiceConversionProcessor::releaseResources()
{
    inputBuffer.clear();
    outputBuffer.clear();
}

void VoiceConversionProcessor::setModelPath(const juce::String& path)
{
    currentModelPath = path;

#if USE_ONNX_RUNTIME
    if (path.isNotEmpty() && juce::File(path).existsAsFile())
    {
        try {
            Ort::SessionOptions sessionOptions;
            sessionOptions.SetIntraOpNumThreads(2);
            sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

            #if defined(_WIN32)
            std::wstring wpath = path.toWideCharPointer();
            ortSession = std::make_unique<Ort::Session>(*ortEnv, wpath.c_str(), sessionOptions);
            #else
            ortSession = std::make_unique<Ort::Session>(*ortEnv, path.toUTF8(), sessionOptions);
            #endif
            
            juce::Logger::writeToLog("VoiceConversionProcessor: ONNX Model loaded successfully.");
        } catch (const Ort::Exception& e) {
            juce::Logger::writeToLog("VoiceConversionProcessor: Failed to load ONNX model. " + juce::String(e.what()));
        }
    }
#else
    juce::Logger::writeToLog("VoiceConversionProcessor: ONNX Runtime disabled. Timbre transfer will be simulated.");
#endif
}

void VoiceConversionProcessor::setSpeakerEmbedding(const std::vector<float>& embedding)
{
    speakerEmbeddingData = embedding;
    juce::Logger::writeToLog("VoiceConversionProcessor: Speaker embedding updated (size: " + juce::String(embedding.size()) + ")");
}

void VoiceConversionProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    if (!enabled.load())
        return;

    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();
    
    // Copy input to mono buffer for feature extraction
    const float* inL = buffer.getReadPointer(0);
    for (int i = 0; i < numSamples; ++i) {
        inputBuffer[i] = inL[i];
        if (numChannels > 1) {
            inputBuffer[i] = (inputBuffer[i] + buffer.getReadPointer(1)[i]) * 0.5f;
        }
        outputBuffer[i] = inputBuffer[i]; // Default dry
    }

    // 1. Pitch Extraction (YIN)
    float detectedPitch = detectPitchYIN(inputBuffer.data(), numSamples, currentSampleRate);
    float targetPitch = detectedPitch * std::pow(2.0f, pitchShift / 12.0f);

#if USE_ONNX_RUNTIME
    // If ONNX is available and session is loaded, run real-time inference
    if (ortSession && speakerEmbeddingData.size() > 0)
    {
        // Convert input buffer, pitch, and embedding into Ort::Value tensors
        // In a real RVC model:
        // - content tensor: shape [1, seq_len, 256] (requires HuBERT extraction first)
        // - pitch tensor: shape [1, seq_len]
        // - speaker tensor: shape [1, emb_size]
        
        // *Placeholder for ONNX inference execution*
        // std::vector<const char*> inputNames = {"audio", "pitch", "speaker"};
        // std::vector<Ort::Value> inputTensors;
        // ...
        // auto outputTensors = ortSession->Run(Ort::RunOptions{nullptr}, inputNames.data(), inputTensors.data(), inputNames.size(), outputNames.data(), outputNames.size());
        
        // *Simulate output*
        for (int i = 0; i < numSamples; ++i) {
            outputBuffer[i] = inputBuffer[i]; // Normally read from outputTensors[0]
        }
    }
#else
    // If ONNX is not built in, perform a lightweight simulation (pitch shift + slight resonant filtering) to demonstrate the signal path works
    if (timbreMix > 0.01f) {
        // Very basic mock timbre effect (clipping/filtering)
        for (int i = 0; i < numSamples; ++i) {
            float s = inputBuffer[i];
            s = std::tanh(s * 1.5f); // "Changed" timbre
            outputBuffer[i] = s;
        }
    }
#endif

    // Wet/Dry Mix back to output
    for (int ch = 0; ch < numChannels; ++ch) {
        auto* out = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i) {
            out[i] = (inputBuffer[i] * (1.0f - timbreMix)) + (outputBuffer[i] * timbreMix);
        }
    }
}

float VoiceConversionProcessor::detectPitchYIN(const float* samples, int numSamples, double sr)
{
    int halfSize = numSamples / 2;
    if (halfSize <= 0) return 0.0f;
    std::vector<float> difference(halfSize, 0.0f);

    for (int tau = 1; tau < halfSize; ++tau) {
        for (int i = 0; i < halfSize; ++i) {
            float d = samples[i] - samples[i + tau];
            difference[tau] += d * d;
        }
    }

    std::vector<float> cmndf(halfSize, 0.0f);
    cmndf[0] = 1.0f;
    float runningSum = 0.0f;

    for (int tau = 1; tau < halfSize; ++tau) {
        runningSum += difference[tau];
        cmndf[tau] = difference[tau] * tau / (runningSum + 1e-9f);
    }

    int bestTau = -1;
    float threshold = 0.15f;
    for (int tau = 2; tau < halfSize; ++tau) {
        if (cmndf[tau] < threshold) {
            while (tau + 1 < halfSize && cmndf[tau + 1] < cmndf[tau]) {
                tau++;
            }
            bestTau = tau;
            break;
        }
    }

    if (bestTau > 0) return (float)sr / (float)bestTau;
    return 0.0f;
}
