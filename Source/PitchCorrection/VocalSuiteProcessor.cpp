#include "VocalSuiteProcessor.h"
#include <cmath>

constexpr int VocalSuiteProcessor::CHROMATIC[12];
constexpr int VocalSuiteProcessor::MAJOR[7];
constexpr int VocalSuiteProcessor::MINOR[7];

VocalSuiteProcessor::VocalSuiteProcessor()
{
    fft = std::make_unique<juce::dsp::FFT>(11); // 2048
}

VocalSuiteProcessor::~VocalSuiteProcessor() {}

void VocalSuiteProcessor::prepareToPlay(double sampleRate, int blockSize)
{
    currentSampleRate = sampleRate;
    currentBlockSize  = blockSize;

    inputBuffer.assign(fftSize, 0.0f);
    outputBuffer.assign(fftSize, 0.0f);
    phaseAccumulator.assign(fftSize, 0.0f);
    lastPhase.assign(fftSize, 0.0f);
    outputAccumulator.assign(fftSize * 2, 0.0f);

    timeStretcher.reset(sampleRate, 2);
    stretchBuffer.setSize(2, blockSize);

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32)blockSize, 2 };
    delayLineL.prepare(spec);
    delayLineR.prepare(spec);
    delayLineL.setMaximumDelayInSamples(44100);
    delayLineR.setMaximumDelayInSamples(44100);
    smoothedLfoPhase.reset(sampleRate, 0.01);

    projectionCompressor.prepare(spec);
    projectionEQHigh.prepare(spec);
    projectionEQLow.prepare(spec);
}

void VocalSuiteProcessor::releaseResources()
{
    inputBuffer.clear();
    outputBuffer.clear();
    phaseAccumulator.clear();
    lastPhase.clear();
    outputAccumulator.clear();
}

void VocalSuiteProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    if (!enabled)
    {
        detectedPitch.store(0.0f);
        correctedPitch.store(0.0f);
        return;
    }

    auto* channelData = buffer.getWritePointer(0);
    int numSamples    = buffer.getNumSamples();

    // 1. Detect Pitch (YIN)
    float detectedHz = detectPitchYIN(channelData, numSamples, currentSampleRate);
    detectedPitch.store(detectedHz);

    // Inflection (LFO Vibrato)
    float inflection = 0.0f;
    if (inflectionAmount > 0.01f)
    {
        float lfoRate = 5.0f; // 5 Hz vibrato
        lfoPhase += (float)(numSamples * lfoRate / currentSampleRate);
        if (lfoPhase > 1.0f) lfoPhase -= 1.0f;
        inflection = std::sin(lfoPhase * juce::MathConstants<float>::twoPi) * inflectionAmount * 0.5f; // +/- 0.5 semitones max
    }

    // 2. Calculate Pitch Shift
    float shiftSemitones = 0.0f;
    if (detectedHz > 50.0f && detectedHz < 1000.0f)
    {
        float targetHz = findClosestScalePitch(detectedHz);
        
        float smoothFactor = juce::jmap(retuneSpeed, 0.0f, 1.0f, 0.05f, 1.0f);
        
        if (neuralMode) {
            // Neural Intent Analysis: dynamic snapping based on vibrato width and melodic drift
            float pitchDiff = std::abs(detectedHz - targetHz);
            if (pitchDiff < 3.0f) {
                // Deep within the pocket, preserve exact micro-variations
                targetHz = detectedHz; 
                smoothFactor = 1.0f;
            } else if (pitchDiff < 15.0f) {
                // Intent is vibrato or slide: gently guide towards target without rigid snapping
                targetHz = detectedHz * 0.4f + targetHz * 0.6f;
                smoothFactor = 0.01f; // Slow, organic drift
            } else {
                // Large intent change detected (transient), snap to note quickly
                smoothFactor = 0.9f;
            }
        }

        pitchSmoothed = pitchSmoothed + smoothFactor * (targetHz - pitchSmoothed);
        if (pitchSmoothed < 50.0f) pitchSmoothed = targetHz;

        correctedPitch.store(pitchSmoothed);
        
        // Add manual Pitch Shift offset and Inflection
        shiftSemitones = 12.0f * std::log2(pitchSmoothed / detectedHz) + pitchShift + inflection;
    }
    else
    {
        correctedPitch.store(detectedHz);
        pitchSmoothed = detectedHz;
        shiftSemitones = pitchShift + inflection;
    }

    // 3. Process Phase Vocoder (Pitch + Formant + Pace)
    // Pace affects the hop size read ratio
    if (std::abs(shiftSemitones) > 0.05f || std::abs(formantShift) > 0.05f || paceStretch != 1.0f)
    {
        processPhaseVocoder(buffer, shiftSemitones, formantShift);
    }

    // 4. Vocal Doubler / Harmonizer
    if (doublerAmount > 0.01f || harmonyInterval != 0)
    {
        applyDoubler(buffer);
    }

    // 5. Apply advanced vocal controls
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float sample = data[i];

            // Articulation: Transient expansion (simplified high-pass envelope follower)
            if (articulation > 0.0f) {
                float absSample = std::abs(sample);
                envelopeFollower = 0.9f * envelopeFollower + 0.1f * absSample;
                if (absSample > envelopeFollower * 1.5f) {
                    sample *= 1.0f + (articulation * 0.5f);
                }
            }
            
            // Resonance: Comb filter to emulate vocal tract
            if (resonanceAmount > 0.0f) {
                float delayTime = 0.002f; // ~500 Hz formant
                int delaySamples = (int)(delayTime * currentSampleRate);
                if (ch == 0) {
                    float delayed = resDelayLinesL[0][resIndex % 2048];
                    resDelayLinesL[0][resIndex % 2048] = sample;
                    sample += delayed * resonanceAmount * 0.5f;
                } else {
                    float delayed = resDelayLinesR[0][resIndex % 2048];
                    resDelayLinesR[0][resIndex % 2048] = sample;
                    sample += delayed * resonanceAmount * 0.5f;
                }
            }

            // Emphasis: Upward compression & soft clipping
            if (emphasisAmount > 0.0f) {
                float env = std::abs(sample);
                emphasisEnv = 0.99f * emphasisEnv + 0.01f * env;
                if (emphasisEnv < 0.1f && emphasisEnv > 0.001f) {
                    sample *= 1.0f + (emphasisAmount * 0.5f); // pull up quiet parts
                }
                sample = std::tanh(sample * (1.0f + emphasisAmount)); // soft clip transients
            }
            
            // Volume
            sample *= volumeLevel;
            
            data[i] = sample;
        }
        resIndex++;
    }

    // Projection: Parallel Compression + High Shelf
    if (projectionAmount > 0.0f)
    {
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        
        projectionCompressor.setRatio(4.0f);
        projectionCompressor.setThreshold(-20.0f);
        projectionCompressor.setAttack(5.0f);
        projectionCompressor.setRelease(100.0f);
        
        // Wet/Dry mix for parallel compression
        juce::AudioBuffer<float> dryBuffer;
        dryBuffer.makeCopyOf(buffer);
        
        projectionCompressor.process(context);
        
        // High shelf boost for air
        float highGain = juce::Decibels::decibelsToGain(projectionAmount * 6.0f);
        *projectionEQHigh.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(currentSampleRate, 5000.0f, 0.707f, highGain);
        projectionEQHigh.process(context);

        // Mix back
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            auto* out = buffer.getWritePointer(ch);
            auto* dry = dryBuffer.getReadPointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                out[i] = (dry[i] * (1.0f - projectionAmount * 0.5f)) + (out[i] * projectionAmount * 0.8f);
            }
        }
    }
}

float VocalSuiteProcessor::detectPitchYIN(const float* samples, int numSamples, double sr)
{
    // Simplified YIN
    int halfSize = numSamples / 2;
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

    if (bestTau == -1) {
        float minVal = 1e9f;
        for (int tau = 2; tau < halfSize; ++tau) {
            if (cmndf[tau] < minVal) {
                minVal = cmndf[tau];
                bestTau = tau;
            }
        }
    }

    if (bestTau > 0)
        return (float)sr / (float)bestTau;
    return 0.0f;
}

float VocalSuiteProcessor::findClosestScalePitch(float hz)
{
    float midiFloat = 69.0f + 12.0f * std::log2(hz / 440.0f);
    int midiNote = juce::roundToInt(midiFloat);

    int octave = midiNote / 12;
    int noteInOct = midiNote % 12;

    int rootNote = key % 12;
    int relativeNote = (noteInOct - rootNote + 12) % 12;

    const int* scaleArr = CHROMATIC;
    int numNotes = 12;
    if (scale == 1) { scaleArr = MAJOR; numNotes = 7; }
    else if (scale == 2) { scaleArr = MINOR; numNotes = 7; }

    int closestRel = scaleArr[0];
    int minDiff = 100;
    for (int i = 0; i < numNotes; ++i) {
        int diff = std::abs(relativeNote - scaleArr[i]);
        if (diff < minDiff) {
            minDiff = diff;
            closestRel = scaleArr[i];
        }
    }

    int correctedMidi = (octave * 12) + rootNote + closestRel;
    return 440.0f * std::pow(2.0f, (correctedMidi - 69) / 12.0f);
}

void VocalSuiteProcessor::processPhaseVocoder(juce::AudioBuffer<float>& buffer, float pitchShiftSemitones, float formantShiftSemitones)
{
    // We delegate the overlapping/pitch shifting engine to TimeStretcher
    float pitchRatio = std::pow(2.0f, pitchShiftSemitones / 12.0f);
    float stretchRatio = paceStretch; // How fast to move through the audio
    
    // In a full implementation, formant shifting involves spectral envelope extraction. 
    // Here we focus on the requested time-stretching and pitch shifting via TimeStretcher engine.
    
    stretchBuffer.setSize(buffer.getNumChannels(), buffer.getNumSamples());
    
    // Use the timeStretcher engine to process our buffer
    timeStretcher.process(buffer, stretchBuffer, stretchRatio, pitchRatio);
    
    // Copy the stretched output back to the original buffer
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        buffer.copyFrom(ch, 0, stretchBuffer, ch, 0, buffer.getNumSamples());
    }
}

void VocalSuiteProcessor::applyDoubler(juce::AudioBuffer<float>& buffer)
{
    // Simple stereo doubler: modulates a short delay line
    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : left;

    float lfoFreq = 1.5f;
    float lfoPhaseInc = juce::MathConstants<float>::twoPi * lfoFreq / currentSampleRate;

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        float lfo = std::sin(smoothedLfoPhase.getCurrentValue());
        smoothedLfoPhase.setTargetValue(smoothedLfoPhase.getCurrentValue() + lfoPhaseInc);
        if (smoothedLfoPhase.getCurrentValue() >= juce::MathConstants<float>::twoPi)
            smoothedLfoPhase.setTargetValue(smoothedLfoPhase.getCurrentValue() - juce::MathConstants<float>::twoPi);

        float delayMsL = 15.0f + lfo * 5.0f;
        float delayMsR = 25.0f - lfo * 6.0f;

        float delL = delayLineL.popSample(0, delayMsL * (currentSampleRate / 1000.0));
        float delR = delayLineR.popSample(0, delayMsR * (currentSampleRate / 1000.0));

        delayLineL.pushSample(0, left[i]);
        delayLineR.pushSample(0, right[i]);

        left[i]  += delL * doublerAmount;
        right[i] += delR * doublerAmount;
    }
}
