#include "AIHumanizerProcessor.h"
#include "../Util/OrpheusLogger.h"
#include "../Project/AppState.h"
#include "../Audio/AudioEngine.h"

AIHumanizerProcessor::AIHumanizerProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    // Initialize pink noise rows
    for (int i = 0; i < kPinkNoiseOctaves; ++i)
        pinkRows[i] = 0.0f;
}

AIHumanizerProcessor::~AIHumanizerProcessor() {}

void AIHumanizerProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();

    // Original wow/flutter delay
    delayLine.prepare(spec);
    delayLine.setMaximumDelayInSamples(44100);
    lfoPhase = 0.0f;

    // Micro-timing jitter delay lines (per-channel)
    juce::dsp::ProcessSpec monoSpec = spec;
    monoSpec.numChannels = 1;
    jitterDelayL.prepare(monoSpec);
    jitterDelayR.prepare(monoSpec);
    jitterDelayL.setMaximumDelayInSamples(512);
    jitterDelayR.setMaximumDelayInSamples(512);
    jitterUpdateInterval = (int)(sampleRate * 0.1); // Update target every ~100ms
    jitterUpdateCounter = 0;
    jitterTargetL = 0.0f;
    jitterTargetR = 0.0f;
    jitterCurrentL = 0.0f;
    jitterCurrentR = 0.0f;

    // Stereo decorrelation allpass buffers
    for (int i = 0; i < kNumAllpass; ++i)
    {
        allpassBufL[i].assign((size_t)allpassDelays[i], 0.0f);
        allpassBufR[i].assign((size_t)allpassDelays[i], 0.0f);
        allpassIdxL[i] = 0;
        allpassIdxR[i] = 0;
        allpassL[i].buffer = 0.0f;
        allpassR[i].buffer = 0.0f;
    }

    // DC blocker state
    dcBlockerX1L = 0.0f; dcBlockerY1L = 0.0f;
    dcBlockerX1R = 0.0f; dcBlockerY1R = 0.0f;

    // Dynamic breathing phase
    breathPhase1 = randomGenerator.nextFloat() * juce::MathConstants<float>::twoPi;
    breathPhase2 = randomGenerator.nextFloat() * juce::MathConstants<float>::twoPi;
    breathPhase3 = randomGenerator.nextFloat() * juce::MathConstants<float>::twoPi;

    // Pink noise state
    pinkIndex = 0;
    pinkNoiseValue = 0.0f;
    for (int i = 0; i < kPinkNoiseOctaves; ++i)
        pinkRows[i] = 0.0f;
}

void AIHumanizerProcessor::releaseResources()
{
}

void AIHumanizerProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    // Apply De-Chatter / De-Click first to clean the raw signal
    if (deChatter > 0.01f)
    {
        applyDeChatter(buffer);
    }

    // Apply Micro-Timing Jitter (breaks perfect grid alignment)
    if (microTiming > 0.01f)
    {
        applyMicroTimingJitter(buffer);
    }

    // Apply WOW and Flutter (Tape imperfection)
    if (flutter > 0.01f)
    {
        applyWowAndFlutter(buffer);
    }

    // Apply Saturation (Analog warmth)
    if (warmth > 0.01f)
    {
        applySaturation(buffer);
    }

    // Apply Harmonic Excitement (Even-order harmonics)
    if (harmonicExciter > 0.01f)
    {
        applyHarmonicExcitement(buffer);
    }

    // Apply Stereo Decorrelation (Natural stereo field)
    if (stereoWidth > 0.01f && buffer.getNumChannels() >= 2)
    {
        applyStereoDecorrelation(buffer);
    }

    // Apply Dynamic Breathing (Natural dynamics)
    if (dynamicBreathing > 0.01f)
    {
        applyDynamicBreathing(buffer);
    }

    // Apply Pink Noise Injection (Analog life)
    if (noiseFloor > 0.01f)
    {
        applyNoiseInjection(buffer);
    }
}

//==============================================================================
// Original DSP stages
//==============================================================================

void AIHumanizerProcessor::applySaturation(juce::AudioBuffer<float>& buffer)
{
    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();

    float drive = 1.0f + (warmth * 4.0f); // 1x to 5x drive
    float makeup = 1.0f / std::sqrt(drive);

    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        for (int i = 0; i < numSamples; ++i)
        {
            float s = channelData[i] * drive;
            // Soft clipper: f(x) = (2/pi) * atan(x)
            channelData[i] = (2.0f / juce::MathConstants<float>::pi) * std::atan(s) * makeup;
        }
    }
}

void AIHumanizerProcessor::applyWowAndFlutter(juce::AudioBuffer<float>& buffer)
{
    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();

    float lfoFreq = 1.5f; // 1.5 Hz wobble
    float lfoDepth = flutter * 2.0f; // Max 2ms delay modulation

    float phaseIncrement = (juce::MathConstants<float>::twoPi * lfoFreq) / (float)currentSampleRate;

    for (int i = 0; i < numSamples; ++i)
    {
        float lfoVal = std::sin(lfoPhase);
        lfoPhase += phaseIncrement;
        if (lfoPhase >= juce::MathConstants<float>::twoPi)
            lfoPhase -= juce::MathConstants<float>::twoPi;

        float delaySamples = 10.0f + (lfoVal * lfoDepth * (currentSampleRate / 1000.0f));

        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);
            float inSample = channelData[i];
            delayLine.pushSample(channel, inSample);
            channelData[i] = delayLine.popSample(channel, delaySamples);
        }
    }
}

void AIHumanizerProcessor::applyDeChatter(juce::AudioBuffer<float>& buffer)
{
    // A simplified de-clicker for "chatter" suppression.
    // Detects sharp transients (delta) and smoothes them out.
    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();

    float threshold = 0.8f - (deChatter * 0.7f); // Lower threshold = more aggressive

    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        float lastSample = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            float currentSample = channelData[i];
            float delta = std::abs(currentSample - lastSample);
            
            if (delta > threshold)
            {
                // Smooth it out by averaging
                currentSample = (currentSample + lastSample) * 0.5f;
                channelData[i] = currentSample;
            }
            lastSample = currentSample;
        }
    }
}

//==============================================================================
// New DSP stages
//==============================================================================

void AIHumanizerProcessor::applyMicroTimingJitter(juce::AudioBuffer<float>& buffer)
{
    // Subtle per-channel timing jitter using interpolated delay lines.
    // A slow random walk modulates the delay per-channel, creating
    // timing imperfections that break perfect grid alignment.
    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();

    // Maximum jitter in samples (0-20 samples at max intensity, ~0.45ms at 44.1kHz)
    float maxJitterSamples = microTiming * 20.0f;

    // Smoothing coefficient — how fast the current delay approaches the target
    float smoothCoeff = 1.0f - std::exp(-1.0f / (float)(currentSampleRate * 0.05f)); // 50ms time constant

    for (int i = 0; i < numSamples; ++i)
    {
        // Periodically pick new random jitter targets
        if (++jitterUpdateCounter >= jitterUpdateInterval)
        {
            jitterUpdateCounter = 0;
            jitterTargetL = (randomGenerator.nextFloat() * 2.0f - 1.0f) * maxJitterSamples;
            jitterTargetR = (randomGenerator.nextFloat() * 2.0f - 1.0f) * maxJitterSamples;
        }

        // Smooth towards target
        jitterCurrentL += (jitterTargetL - jitterCurrentL) * smoothCoeff;
        jitterCurrentR += (jitterTargetR - jitterCurrentR) * smoothCoeff;

        // Ensure delay is always positive (offset so minimum is 1 sample)
        float delayL = juce::jmax(1.0f, maxJitterSamples + jitterCurrentL + 1.0f);
        float delayR = juce::jmax(1.0f, maxJitterSamples + jitterCurrentR + 1.0f);

        if (numChannels >= 1)
        {
            auto* dataL = buffer.getWritePointer(0);
            jitterDelayL.pushSample(0, dataL[i]);
            dataL[i] = jitterDelayL.popSample(0, delayL);
        }
        if (numChannels >= 2)
        {
            auto* dataR = buffer.getWritePointer(1);
            jitterDelayR.pushSample(0, dataR[i]);
            dataR[i] = jitterDelayR.popSample(0, delayR);
        }
    }
}

void AIHumanizerProcessor::applyStereoDecorrelation(juce::AudioBuffer<float>& buffer)
{
    // Allpass-based mid/side decorrelation.
    // Processes only the Side channel through a chain of Schroeder allpass filters
    // with prime-number delay taps, adding frequency-dependent phase rotation
    // that simulates mic bleed / room interaction.
    int numSamples = buffer.getNumSamples();

    auto* dataL = buffer.getWritePointer(0);
    auto* dataR = buffer.getWritePointer(1);

    float wetAmount = stereoWidth * 0.6f; // Max 60% wet to avoid extreme widening

    for (int i = 0; i < numSamples; ++i)
    {
        // Encode to Mid/Side
        float mid  = (dataL[i] + dataR[i]) * 0.5f;
        float side = (dataL[i] - dataR[i]) * 0.5f;

        // Process side through allpass chain (only process side for decorrelation)
        float processedSide = side;
        for (int ap = 0; ap < kNumAllpass; ++ap)
        {
            int delayLen = allpassDelays[ap];
            float coeff = allpassCoeffs[ap];

            // Read from circular buffer
            float delayed = allpassBufL[ap][(size_t)allpassIdxL[ap]];

            // Classic allpass: y[n] = -g*x[n] + x[n-d] + g*y[n-d]
            //                       = delayed + coeff * (processedSide - delayed)
            //                  ... simplified Schroeder allpass
            float output = -coeff * processedSide + delayed;
            allpassBufL[ap][(size_t)allpassIdxL[ap]] = processedSide + coeff * output;

            // Advance circular buffer index
            allpassIdxL[ap] = (allpassIdxL[ap] + 1) % delayLen;

            processedSide = output;
        }

        // Blend original and processed side
        float finalSide = side + (processedSide - side) * wetAmount;

        // Decode back to L/R
        dataL[i] = mid + finalSide;
        dataR[i] = mid - finalSide;
    }
}

void AIHumanizerProcessor::applyHarmonicExcitement(juce::AudioBuffer<float>& buffer)
{
    // Even-order harmonic excitation using Chebyshev polynomial waveshaping.
    // T2(x) = 2x^2 - 1 generates primarily 2nd harmonic content,
    // which is the characteristic warmth of tube amplifiers.
    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();

    float mix = harmonicExciter * 0.3f; // Max 30% wet
    float dcBlockR = 0.995f; // DC blocker coefficient

    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        float& x1 = (channel == 0) ? dcBlockerX1L : dcBlockerX1R;
        float& y1 = (channel == 0) ? dcBlockerY1L : dcBlockerY1R;

        for (int i = 0; i < numSamples; ++i)
        {
            float dry = channelData[i];

            // Soft-limit input to [-1, 1] range for Chebyshev stability
            float x = juce::jlimit(-1.0f, 1.0f, dry);

            // T2(x) = 2x^2 - 1  (2nd order Chebyshev, generates 2nd harmonic)
            float chebyshev2 = 2.0f * x * x - 1.0f;

            // T3(x) = 4x^3 - 3x  (3rd order, generates 3rd harmonic, add subtle amount)
            float chebyshev3 = 4.0f * x * x * x - 3.0f * x;

            // Blend: mostly 2nd harmonic (warm), a touch of 3rd (presence)
            float excited = chebyshev2 * 0.7f + chebyshev3 * 0.3f;

            // DC blocker: y[n] = x[n] - x[n-1] + R * y[n-1]
            float dcBlocked = excited - x1 + dcBlockR * y1;
            x1 = excited;
            y1 = dcBlocked;

            // Blend wet/dry
            channelData[i] = dry + dcBlocked * mix;
        }
    }
}

void AIHumanizerProcessor::applyDynamicBreathing(juce::AudioBuffer<float>& buffer)
{
    // Natural dynamic variation using summed slow LFOs with irrational
    // frequency ratios, creating a Perlin-like smooth random gain modulation
    // that mimics the way a human performer's dynamics naturally fluctuate.
    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();

    // Three slow LFOs at irrational frequency ratios
    float freq1 = 0.13f;  // ~0.13 Hz
    float freq2 = 0.31f;  // ~0.31 Hz
    float freq3 = 0.79f;  // ~0.79 Hz

    float phaseInc1 = (juce::MathConstants<float>::twoPi * freq1) / (float)currentSampleRate;
    float phaseInc2 = (juce::MathConstants<float>::twoPi * freq2) / (float)currentSampleRate;
    float phaseInc3 = (juce::MathConstants<float>::twoPi * freq3) / (float)currentSampleRate;

    // Max gain deviation in dB (scales with parameter)
    float maxDeviationDB = dynamicBreathing * 1.5f; // Max ±1.5 dB

    for (int i = 0; i < numSamples; ++i)
    {
        // Sum three sine waves for pseudo-random smooth modulation
        float mod = (std::sin(breathPhase1) * 0.5f +
                     std::sin(breathPhase2) * 0.3f +
                     std::sin(breathPhase3) * 0.2f);

        // Convert to gain
        float gainDB = mod * maxDeviationDB;
        float gain = juce::Decibels::decibelsToGain(gainDB);

        for (int channel = 0; channel < numChannels; ++channel)
        {
            buffer.getWritePointer(channel)[i] *= gain;
        }

        breathPhase1 += phaseInc1;
        breathPhase2 += phaseInc2;
        breathPhase3 += phaseInc3;

        // Wrap phases
        if (breathPhase1 >= juce::MathConstants<float>::twoPi) breathPhase1 -= juce::MathConstants<float>::twoPi;
        if (breathPhase2 >= juce::MathConstants<float>::twoPi) breathPhase2 -= juce::MathConstants<float>::twoPi;
        if (breathPhase3 >= juce::MathConstants<float>::twoPi) breathPhase3 -= juce::MathConstants<float>::twoPi;
    }
}

void AIHumanizerProcessor::applyNoiseInjection(juce::AudioBuffer<float>& buffer)
{
    // Pink noise injection (1/f spectrum) using Voss-McCartney algorithm.
    // Pink noise is more natural-sounding than white noise and matches
    // the spectral characteristics of analog tape hiss and room tone.
    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();

    // Noise level maps from -80dB to -40dB depending on noiseFloor parameter
    float maxNoiseLevel = juce::Decibels::decibelsToGain(-80.0f + (noiseFloor * 40.0f));

    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        for (int i = 0; i < numSamples; ++i)
        {
            // Generate pink noise sample
            float noise = generatePinkNoiseSample() * maxNoiseLevel;
            
            // Envelope following to only inject noise when audio is playing (analog tape hiss effect)
            float envelope = std::abs(channelData[i]) * 5.0f;
            envelope = juce::jlimit(0.01f, 1.0f, envelope);

            channelData[i] += noise * envelope;
        }
    }
}

//==============================================================================
// Pink noise generator (Voss-McCartney algorithm)
//==============================================================================

float AIHumanizerProcessor::generatePinkNoiseSample()
{
    // Voss-McCartney algorithm: maintains 8 octaves of random values.
    // At each step, only the rows indicated by trailing zeros of the counter
    // are updated, producing a natural 1/f spectral rolloff.
    int lastIndex = pinkIndex;
    pinkIndex++;

    // Find which rows need updating (trailing zeros of counter)
    int diff = lastIndex ^ pinkIndex;

    float sum = 0.0f;
    for (int i = 0; i < kPinkNoiseOctaves; ++i)
    {
        if (diff & (1 << i))
        {
            // Update this row with new random value
            pinkRows[i] = (randomGenerator.nextFloat() * 2.0f - 1.0f);
        }
        sum += pinkRows[i];
    }

    // Add white noise component for the highest octave
    sum += (randomGenerator.nextFloat() * 2.0f - 1.0f);

    // Normalize: sum of 9 random values (8 octaves + 1 white), scale to ~[-1, 1]
    return sum / (float)(kPinkNoiseOctaves + 1);
}

//==============================================================================
// Preset application
//==============================================================================

void AIHumanizerProcessor::applyPreset(Preset preset)
{
    switch (preset)
    {
        case Preset::Subtle:
            warmth          = 0.15f;
            flutter         = 0.1f;
            noiseFloor      = 0.05f;
            deChatter       = 0.3f;
            microTiming     = 0.1f;
            stereoWidth     = 0.1f;
            harmonicExciter = 0.05f;
            dynamicBreathing = 0.1f;
            break;

        case Preset::WarmAnalog:
            warmth          = 0.7f;
            flutter         = 0.5f;
            noiseFloor      = 0.3f;
            deChatter       = 0.4f;
            microTiming     = 0.2f;
            stereoWidth     = 0.2f;
            harmonicExciter = 0.5f;
            dynamicBreathing = 0.15f;
            break;

        case Preset::LiveFeel:
            warmth          = 0.3f;
            flutter         = 0.2f;
            noiseFloor      = 0.1f;
            deChatter       = 0.5f;
            microTiming     = 0.6f;
            stereoWidth     = 0.5f;
            harmonicExciter = 0.15f;
            dynamicBreathing = 0.5f;
            break;

        case Preset::FullTreatment:
            warmth          = 0.5f;
            flutter         = 0.4f;
            noiseFloor      = 0.2f;
            deChatter       = 0.5f;
            microTiming     = 0.4f;
            stereoWidth     = 0.35f;
            harmonicExciter = 0.3f;
            dynamicBreathing = 0.3f;
            break;
    }
}

//==============================================================================
// Offline file processing
//==============================================================================

void AIHumanizerProcessor::processFileOffline(const juce::File& inputFile, const juce::File& outputFile, class AppState* appState, class AudioEngine* engine)
{
    OrpheusLogger::logInfo("AIHumanizerProcessor: Starting offline processing of " + inputFile.getFullPathName());

    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(inputFile));
    if (!reader)
    {
        OrpheusLogger::logError("AIHumanizerProcessor: Failed to read file.");
        return;
    }

    double sampleRate = reader->sampleRate;
    int numChannels = reader->numChannels;
    int lengthInSamples = (int)reader->lengthInSamples;

    juce::AudioBuffer<float> buffer(numChannels, lengthInSamples);
    reader->read(&buffer, 0, lengthInSamples, 0, true, true);

    // Process chunk by chunk to simulate real-time processing blocks
    prepareToPlay(sampleRate, 2048);

    int blockSize = 2048;
    juce::AudioBuffer<float> blockBuffer;
    blockBuffer.setSize(numChannels, blockSize);
    juce::MidiBuffer emptyMidi;

    for (int i = 0; i < lengthInSamples; i += blockSize)
    {
        int numSamples = juce::jmin(blockSize, lengthInSamples - i);
        for (int c = 0; c < numChannels; ++c) {
            blockBuffer.copyFrom(c, 0, buffer, c, i, numSamples);
        }
        
        processBlock(blockBuffer, emptyMidi);

        for (int c = 0; c < numChannels; ++c)
        {
            buffer.copyFrom(c, i, blockBuffer, c, 0, numSamples);
        }
    }

    // Write file
    std::unique_ptr<juce::AudioFormatWriter> writer(
        formatManager.findFormatForFileExtension(".wav")
                     ->createWriterFor(new juce::FileOutputStream(outputFile),
                                       sampleRate,
                                       numChannels,
                                       24,
                                       {},
                                       0));

    if (writer)
    {
        writer->writeFromAudioSampleBuffer(buffer, 0, lengthInSamples);
        writer.reset();
        OrpheusLogger::logInfo("AIHumanizerProcessor: Successfully created " + outputFile.getFullPathName());

        if (appState && engine)
        {
            int newTrackIdx = appState->addAudioTrack("Humanized");
            engine->addAudioTrack("Humanized"); // Sync
            
            auto* newClip = new AudioClip(outputFile, 0.0);
            engine->getTrackInfo(newTrackIdx).clips.add(newClip);
        }
    }
    else
    {
        OrpheusLogger::logError("AIHumanizerProcessor: Failed to create output file.");
    }
}
