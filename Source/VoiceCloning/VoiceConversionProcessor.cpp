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

    // Setup high-pass filter for humanize (unvoiced noise extraction)
    // Cutoff around 6kHz to isolate breaths and lip noise
    highPassFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 6000.0f);
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
    
    // Smooth F0 if preserving vibrato
    float smoothedPitch = detectedPitch;
    if (preserveVibrato.load() && detectedPitch > 0.0f) {
        // Find nearest chromatic note
        float midiNote = 69.0f + 12.0f * std::log2(detectedPitch / 440.0f);
        float perfectMidiNote = std::round(midiNote);
        float perfectPitch = 440.0f * std::pow(2.0f, (perfectMidiNote - 69.0f) / 12.0f);
        
        // Retain the deviation (vibrato) but center it on the perfect pitch
        smoothedPitch = smoothF0Contour(detectedPitch, perfectPitch);
    }
    
    float targetPitch = smoothedPitch * std::pow(2.0f, pitchShift / 12.0f);

#if USE_ONNX_RUNTIME
    // If ONNX is available and session is loaded, run real-time inference
    if (ortSession && speakerEmbeddingData.size() > 0)
    {
        try {
            // A typical RVC or Voice Conversion model might take:
            // 1. Audio/Hubert PPGs (we'll just use raw audio for generic ONNX testing here)
            // 2. F0 (Pitch)
            // 3. Speaker Embedding
            
            auto allocatorInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

            // Shape: [1, seq_len]
            std::vector<int64_t> audioShape = {1, numSamples};
            Ort::Value audioTensor = Ort::Value::CreateTensor<float>(
                allocatorInfo, inputBuffer.data(), inputBuffer.size(), audioShape.data(), audioShape.size());

            // Shape: [1, seq_len] -> We duplicate pitch across the block for simplicity
            std::vector<float> pitchBuffer(numSamples, targetPitch);
            std::vector<int64_t> pitchShape = {1, numSamples};
            Ort::Value pitchTensor = Ort::Value::CreateTensor<float>(
                allocatorInfo, pitchBuffer.data(), pitchBuffer.size(), pitchShape.data(), pitchShape.size());

            // Shape: [1, emb_size]
            std::vector<int64_t> spkShape = {1, (int64_t)speakerEmbeddingData.size()};
            Ort::Value spkTensor = Ort::Value::CreateTensor<float>(
                allocatorInfo, speakerEmbeddingData.data(), speakerEmbeddingData.size(), spkShape.data(), spkShape.size());

            std::vector<const char*> inputNames = {"audio", "pitch", "speaker"};
            std::vector<Ort::Value> inputTensors;
            inputTensors.push_back(std::move(audioTensor));
            inputTensors.push_back(std::move(pitchTensor));
            inputTensors.push_back(std::move(spkTensor));

            std::vector<const char*> outputNames = {"audio_out"};

            auto outputTensors = ortSession->Run(
                Ort::RunOptions{nullptr}, 
                inputNames.data(), 
                inputTensors.data(), 
                inputNames.size(), 
                outputNames.data(), 
                outputNames.size()
            );

            // Extract output
            if (outputTensors.size() > 0 && outputTensors[0].IsTensor()) {
                float* outData = outputTensors[0].GetTensorMutableData<float>();
                auto typeInfo = outputTensors[0].GetTensorTypeAndShapeInfo();
                size_t outLen = typeInfo.GetElementCount();
                
                size_t limit = juce::jmin((size_t)numSamples, outLen);
                for (size_t i = 0; i < limit; ++i) {
                    outputBuffer[i] = outData[i];
                }
            }
        } catch (const Ort::Exception& e) {
            // Inference failed, pass dry signal to avoid stutter
            juce::Logger::writeToLog("ONNX Inference Error: " + juce::String(e.what()));
            for (int i = 0; i < numSamples; ++i) {
                outputBuffer[i] = inputBuffer[i];
            }
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

    // Wet/Dry Mix back to output and Humanize
    float humanize = humanizeAmount.load();
    bool keepTiming = preserveTiming.load();
    
    for (int ch = 0; ch < numChannels; ++ch) {
        auto* out = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i) {
            float original = inputBuffer[i];
            float neural = outputBuffer[i];
            
            // Extract unvoiced noise from original
            float unvoicedNoise = highPassFilter.processSample(original);
            
            // If preserveTiming is engaged, we emphasize original consonants 
            // by injecting more of the unvoiced noise and dynamically docking the neural signal 
            // during transient attacks to prevent smearing.
            float consonantInjection = 0.0f;
            if (keepTiming && std::abs(unvoicedNoise) > 0.1f) {
                consonantInjection = unvoicedNoise * 1.5f;
                neural *= 0.5f; // Duck neural slightly during hard consonants
            }
            
            // Mix Neural and Humanize Noise
            float perfectVocal = (original * (1.0f - timbreMix)) + (neural * timbreMix);
            out[i] = perfectVocal + (unvoicedNoise * humanize) + consonantInjection;
        }
    }
}

float VoiceConversionProcessor::smoothF0Contour(float currentF0, float targetF0)
{
    // A simplified micro-pitch preserver.
    // In reality, this would require analyzing a window to find the mean F0 of the current note,
    // and shifting the entire contour so the mean aligns with targetF0.
    // For now, we gently interpolate towards the target to smooth out robotic snapping.
    return currentF0 * 0.2f + targetF0 * 0.8f;
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
