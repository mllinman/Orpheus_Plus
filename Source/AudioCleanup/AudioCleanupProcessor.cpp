#include "AudioCleanupProcessor.h"

AudioCleanupProcessor::AudioCleanupProcessor()
    : AudioProcessor(BusesProperties()
          .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{}

AudioCleanupProcessor::~AudioCleanupProcessor() {}

void AudioCleanupProcessor::prepareToPlay(double sampleRate, int blockSize)
{
    currentSampleRate = sampleRate;
    fftSize = 2048;

    fft = std::make_unique<juce::dsp::FFT>((int)std::log2(fftSize));
    fftBuffer.assign(fftSize * 2, 0.0f);

    // Hann window
    windowFunc.resize(fftSize);
    for (int i = 0; i < fftSize; ++i)
        windowFunc[i] = 0.5f - 0.5f * std::cos(2.0f * juce::MathConstants<float>::pi * i / fftSize);

    noiseProfile.assign(fftSize / 2 + 1, 0.0f);

    // Build notch filters for hum removal
    humFiltersL.clear();
    humFiltersR.clear();
    for (int h = 1; h <= humHarmonics; ++h)
    {
        float freq  = humFreq * h;
        auto  coeff = juce::dsp::IIR::Coefficients<float>::makeNotch(sampleRate, freq, 10.0f);
        humFiltersL.emplace_back();
        humFiltersR.emplace_back();
        *humFiltersL.back().coefficients  = *coeff;
        *humFiltersR.back().coefficients  = *coeff;
    }
}

void AudioCleanupProcessor::releaseResources()
{
    humFiltersL.clear();
    humFiltersR.clear();
}

void AudioCleanupProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                          juce::MidiBuffer&)
{
    if (dcEnabled)      processDCOffset(buffer);
    if (noiseEnabled)   processNoiseReduction(buffer);
    if (deClickEnabled) processDeClick(buffer);
    if (deEsserEnabled) processDeEsser(buffer);
    if (humEnabled)     processHumRemoval(buffer);
}

//──────────────────────────────────────────────────────────────────────────────
void AudioCleanupProcessor::processDCOffset(juce::AudioBuffer<float>& buffer)
{
    // High-pass filter at ~5 Hz to remove DC
    const float coeff = 1.0f - (2.0f * juce::MathConstants<float>::pi * 5.0f / (float)currentSampleRate);

    if (buffer.getNumChannels() > 0)
    {
        auto* L = buffer.getWritePointer(0);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            dcHistL = coeff * dcHistL + L[i];
            L[i] -= dcHistL * (1.0f - coeff);
        }
    }
    if (buffer.getNumChannels() > 1)
    {
        auto* R = buffer.getWritePointer(1);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            dcHistR = coeff * dcHistR + R[i];
            R[i] -= dcHistR * (1.0f - coeff);
        }
    }
}

void AudioCleanupProcessor::processNoiseReduction(juce::AudioBuffer<float>& buffer)
{
    if (!hasNoiseProfile) return;

    // Spectral subtraction / Wiener filter per channel
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        int n = buffer.getNumSamples();

        // Process in FFT frames
        for (int pos = 0; pos + fftSize <= n; pos += fftSize / 2)
        {
            // Fill FFT buffer with windowed input
            for (int i = 0; i < fftSize; ++i)
            {
                fftBuffer[i * 2]     = (pos + i < n) ? data[pos + i] * windowFunc[i] : 0.0f;
                fftBuffer[i * 2 + 1] = 0.0f;
            }

            fft->performRealOnlyForwardTransform(fftBuffer.data());

            // Apply Wiener filter
            for (int bin = 0; bin < fftSize / 2 + 1; ++bin)
            {
                float re  = fftBuffer[bin * 2];
                float im  = fftBuffer[bin * 2 + 1];
                float mag = std::sqrt(re * re + im * im);
                float noiseFloor = noiseProfile[bin] * (1.0f + noiseAmount * 3.0f);
                float gain = (mag > noiseFloor)
                    ? (mag - noiseFloor * noiseAmount) / mag
                    : 0.0f;
                fftBuffer[bin * 2]     = re * gain;
                fftBuffer[bin * 2 + 1] = im * gain;
            }

            fft->performRealOnlyInverseTransform(fftBuffer.data());

            // Overlap-add back
            for (int i = 0; i < fftSize && pos + i < n; ++i)
                data[pos + i] = fftBuffer[i * 2] * windowFunc[i] / fftSize;
        }
    }
}

void AudioCleanupProcessor::captureNoiseProfile()
{
    // TODO: capture next audio block as noise profile
    hasNoiseProfile = false;
}

void AudioCleanupProcessor::setNoiseReductionAmount(float amount)
{
    noiseAmount = juce::jlimit(0.0f, 1.0f, amount);
}

void AudioCleanupProcessor::setNoiseGateThreshold(float threshDB)
{
    noiseGateThresh = threshDB;
}

void AudioCleanupProcessor::processDeClick(juce::AudioBuffer<float>& buffer)
{
    // Click/crackle detection using derivative threshold
    const float clickThresh = 0.3f;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        int n = buffer.getNumSamples();

        float prev = 0.0f;
        for (int i = 0; i < n; ++i)
        {
            float delta = data[i] - prev;
            if (std::abs(delta) > clickThresh)
            {
                // Interpolate over the click
                int windowSize = juce::jmin(8, n - i);
                for (int w = 0; w < windowSize; ++w)
                    data[i + w] = prev + (data[juce::jmin(i + windowSize, n - 1)] - prev)
                                  * w / windowSize;
            }
            prev = data[i];
        }
    }
}

void AudioCleanupProcessor::processDeEsser(juce::AudioBuffer<float>& buffer)
{
    // Sidechain-style de-esser: detect sibilance, apply gain reduction
    const float attackCoeff  = std::exp(-1.0f / (0.001f * (float)currentSampleRate));
    const float releaseCoeff = std::exp(-1.0f / (0.05f  * (float)currentSampleRate));
    const float threshLin    = juce::Decibels::decibelsToGain(deEsserThresh);
    const float rangeLin     = juce::Decibels::decibelsToGain(deEsserRange);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        auto& env  = (ch == 0) ? deEsserEnvL : deEsserEnvR;
        int n = buffer.getNumSamples();

        // Simple bandpass around deEsserFreq to detect sibilance
        float fc   = deEsserFreq / (float)currentSampleRate;
        float bpOut = 0.0f;

        for (int i = 0; i < n; ++i)
        {
            // 1-pole bandpass approximation
            bpOut = data[i] * 0.3f + bpOut * 0.7f; // very simplified

            float mag = std::abs(bpOut);
            if (mag > env) env = attackCoeff  * env + (1.0f - attackCoeff)  * mag;
            else           env = releaseCoeff * env + (1.0f - releaseCoeff) * mag;

            float gain = (env > threshLin)
                ? 1.0f - (1.0f - rangeLin) * juce::jlimit(0.0f, 1.0f,
                          (env - threshLin) / threshLin)
                : 1.0f;

            data[i] *= gain;
        }
    }
}

void AudioCleanupProcessor::processHumRemoval(juce::AudioBuffer<float>& buffer)
{
    juce::dsp::AudioBlock<float> block(buffer);

    for (int h = 0; h < (int)humFiltersL.size() && h < (int)humFiltersR.size(); ++h)
    {
        if (buffer.getNumChannels() > 0)
        {
            auto ch0 = block.getSingleChannelBlock(0);
            juce::dsp::ProcessContextReplacing<float> ctx(ch0);
            humFiltersL[h].process(ctx);
        }
        if (buffer.getNumChannels() > 1)
        {
            auto ch1 = block.getSingleChannelBlock(1);
            juce::dsp::ProcessContextReplacing<float> ctx(ch1);
            humFiltersR[h].process(ctx);
        }
    }
}

bool AudioCleanupProcessor::processFileWithRNNoise(const juce::File& inputFile,
                                                    const juce::File& outputFile,
                                                    float amount)
{
    //─────────────────────────────────────────────────────────────────────────
    // RNNoise (https://gitlab.xiph.org/xiph/rnnoise)
    // For best results, compile RNNoise as a C++ library and link directly.
    // Quick approach: use Python wrapper
    //   pip install rnnoise-python
    //─────────────────────────────────────────────────────────────────────────
    juce::String script =
        "import soundfile as sf, numpy as np\n"
        "try:\n"
        "    import rnnoise\n"
        "    denoiser = rnnoise.RNNoise()\n"
        "    audio, sr = sf.read('" + inputFile.getFullPathName() + "')\n"
        "    if audio.ndim == 1: audio = audio[:, np.newaxis]\n"
        "    out = np.zeros_like(audio)\n"
        "    for ch in range(audio.shape[1]):\n"
        "        out[:, ch] = denoiser.process_frames(audio[:, ch], sr) * "
        + juce::String(amount, 2) + " + audio[:, ch] * (1 - " + juce::String(amount, 2) + ")\n"
        "    sf.write('" + outputFile.getFullPathName() + "', out, sr)\n"
        "except Exception as e:\n"
        "    import shutil; shutil.copy('" + inputFile.getFullPathName() + "', '"
        + outputFile.getFullPathName() + "')\n";

    auto scriptFile = juce::File::createTempFile(".py");
    scriptFile.replaceWithText(script);

    juce::ChildProcess proc;
    proc.start("python3 " + scriptFile.getFullPathName());
    bool ok = proc.waitForProcessToFinish(60000) && proc.getExitCode() == 0;

    scriptFile.deleteFile();
    return ok;
}
