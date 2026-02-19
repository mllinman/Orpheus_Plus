#include "AutoTuneProcessor.h"

constexpr int AutoTuneProcessor::CHROMATIC[12];
constexpr int AutoTuneProcessor::MAJOR[7];
constexpr int AutoTuneProcessor::MINOR[7];

AutoTuneProcessor::AutoTuneProcessor()
    : AudioProcessor(BusesProperties()
          .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{}

AutoTuneProcessor::~AutoTuneProcessor() {}

void AutoTuneProcessor::prepareToPlay(double sampleRate, int blockSize)
{
    currentSampleRate = sampleRate;
    currentBlockSize  = blockSize;

    fftSize = 2048;
    while (fftSize < blockSize * overlapFactor)
        fftSize *= 2;

    fft = std::make_unique<juce::dsp::FFT>((int)std::log2(fftSize));

    inputBuffer.assign(fftSize, 0.0f);
    outputBuffer.assign(fftSize * 2, 0.0f);
    phaseAccumulator.assign(fftSize / 2 + 1, 0.0f);
    lastPhase.assign(fftSize / 2 + 1, 0.0f);
    outputAccumulator.assign(fftSize * 2, 0.0f);
    pitchSmoothed = 0.0f;
}

void AutoTuneProcessor::releaseResources()
{
    inputBuffer.clear();
    outputBuffer.clear();
}

void AutoTuneProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                      juce::MidiBuffer&)
{
    if (!enabled) return;

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // Process mono (left channel for detection, apply to all channels)
    if (numChannels == 0) return;

    auto* data = buffer.getReadPointer(0);

    // 1. Detect pitch using YIN algorithm
    float rawPitch = detectPitchYIN(data, numSamples, currentSampleRate);

    // 2. Smooth pitch detection
    if (rawPitch > 50.0f && rawPitch < 2000.0f)
    {
        float alpha = 0.3f * speed + 0.1f;
        pitchSmoothed = alpha * rawPitch + (1.0f - alpha) * pitchSmoothed;
        detectedPitch.store(pitchSmoothed);
    }

    // 3. Find target pitch on scale
    float targetPitch = findClosestScalePitch(pitchSmoothed);
    correctedPitch.store(targetPitch);

    // 4. Calculate semitones to shift
    if (pitchSmoothed > 0.0f && targetPitch > 0.0f)
    {
        float semitones = 12.0f * std::log2f(targetPitch / pitchSmoothed);

        // Blend based on speed (0 = subtle correction, 1 = hard snap)
        float correction = semitones * (speed * 0.9f + 0.1f);

        if (std::abs(correction) > 0.01f)
            shiftPitch(buffer, correction);
    }

    // 5. Robot voice effect (optional)
    if (robotAmount > 0.01f)
    {
        // Vocoder-style hard pitch snap - flatten phase in FFT
        // Simplified: this adds characteristic robotic quality
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* w = buffer.getWritePointer(ch);
            for (int i = 0; i < numSamples; ++i)
            {
                // Hard-clip to create more harmonic content
                float r = w[i] * (1.0f + robotAmount * 2.0f);
                w[i] = std::tanh(r) * (1.0f - robotAmount * 0.5f) + w[i] * robotAmount * 0.5f;
            }
        }
    }
}

float AutoTuneProcessor::detectPitchYIN(const float* samples, int numSamples, double sr)
{
    // YIN pitch detection algorithm (de Cheveigné & Kawahara, 2002)
    const int maxLag = numSamples / 2;
    std::vector<float> d(maxLag, 0.0f);
    std::vector<float> d_prime(maxLag, 0.0f);

    // Step 1: Difference function
    for (int tau = 1; tau < maxLag; ++tau)
    {
        float sum = 0.0f;
        for (int j = 0; j < maxLag; ++j)
        {
            float diff = samples[j] - samples[j + tau];
            sum += diff * diff;
        }
        d[tau] = sum;
    }

    // Step 2: Cumulative mean normalized difference
    d_prime[0] = 1.0f;
    float runningSum = 0.0f;
    for (int tau = 1; tau < maxLag; ++tau)
    {
        runningSum += d[tau];
        d_prime[tau] = d[tau] * tau / runningSum;
    }

    // Step 3: Find first dip below threshold
    const float threshold = 0.15f;
    int tauEstimate = -1;

    for (int tau = 2; tau < maxLag; ++tau)
    {
        if (d_prime[tau] < threshold)
        {
            while (tau + 1 < maxLag && d_prime[tau + 1] < d_prime[tau])
                ++tau;
            tauEstimate = tau;
            break;
        }
    }

    if (tauEstimate < 2) return 0.0f;

    // Step 4: Parabolic interpolation for sub-sample accuracy
    int x0 = tauEstimate > 0 ? tauEstimate - 1 : tauEstimate;
    int x2 = tauEstimate < maxLag - 1 ? tauEstimate + 1 : tauEstimate;

    float betterTau;
    if (x0 == tauEstimate)
        betterTau = d_prime[tauEstimate] <= d_prime[x2] ? (float)tauEstimate : (float)x2;
    else if (x2 == tauEstimate)
        betterTau = d_prime[tauEstimate] <= d_prime[x0] ? (float)tauEstimate : (float)x0;
    else
    {
        float s0 = d_prime[x0], s1 = d_prime[tauEstimate], s2 = d_prime[x2];
        betterTau = (float)tauEstimate + (s2 - s0) / (2.0f * (2.0f * s1 - s2 - s0));
    }

    return (float)(sr / betterTau);
}

float AutoTuneProcessor::findClosestScalePitch(float hz)
{
    if (hz <= 0.0f) return hz;

    // Convert to MIDI note number
    double midiNote = 69.0 + 12.0 * std::log2(hz / 440.0);
    int    octave   = (int)(midiNote / 12);
    int    semitone = (int)std::round(midiNote) % 12;

    // Find closest note in scale
    int closestNote = semitone;
    float minDist   = 12.0f;

    auto checkScale = [&](const int* scaleNotes, int count)
    {
        for (int i = 0; i < count; ++i)
        {
            int candidate = (scaleNotes[i] + key) % 12;
            float dist    = std::abs((float)(candidate - semitone));
            if (dist > 6.0f) dist = 12.0f - dist;
            if (dist < minDist) { minDist = dist; closestNote = candidate; }
        }
    };

    switch (scale)
    {
        case 0: checkScale(CHROMATIC, 12); break;
        case 1: checkScale(MAJOR, 7); break;
        case 2: checkScale(MINOR, 7); break;
        default: checkScale(CHROMATIC, 12); break;
    }

    double targetMidi = octave * 12.0 + closestNote;
    return (float)(440.0 * std::pow(2.0, (targetMidi - 69.0) / 12.0));
}

void AutoTuneProcessor::shiftPitch(juce::AudioBuffer<float>& buffer, float semitones)
{
    // Phase vocoder pitch shifting
    // For production quality, replace this with Rubber Band Library:
    //   rubberband->setPitchScale(std::pow(2.0, semitones / 12.0));
    //   rubberband->process(inputChannels, numSamples, false);

    const double ratio = std::pow(2.0, semitones / 12.0);
    const int numSamples  = buffer.getNumSamples();
    const int hopSize     = fftSize / overlapFactor;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);

        // Simple resampling-based pitch shift (low quality, replace with Rubber Band)
        std::vector<float> temp(numSamples);
        for (int i = 0; i < numSamples; ++i)
        {
            double srcIdx = i / ratio;
            int    s0     = (int)srcIdx;
            float  frac   = (float)(srcIdx - s0);
            int    s1     = juce::jmin(s0 + 1, numSamples - 1);
            s0 = juce::jlimit(0, numSamples - 1, s0);
            temp[i] = data[s0] * (1.0f - frac) + data[s1] * frac;
        }
        std::copy(temp.begin(), temp.end(), data);
    }
}
