#include "AIHumanizerProcessor.h"
#include "../Util/OrpheusLogger.h"
#include "../Project/AppState.h"
#include "../Audio/AudioEngine.h"

AIHumanizerProcessor::AIHumanizerProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

AIHumanizerProcessor::~AIHumanizerProcessor() {}

void AIHumanizerProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();

    delayLine.prepare(spec);
    delayLine.setMaximumDelayInSamples(44100);
    lfoPhase = 0.0f;
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

    // Apply Stochastic Noise (Trick AI detectors by injecting analog life)
    if (noiseFloor > 0.01f)
    {
        applyNoiseInjection(buffer);
    }
}

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

void AIHumanizerProcessor::applyNoiseInjection(juce::AudioBuffer<float>& buffer)
{
    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();

    // Noise level maps from -80dB to -40dB depending on noiseFloor parameter
    float maxNoiseLevel = juce::Decibels::decibelsToGain(-80.0f + (noiseFloor * 40.0f));

    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        for (int i = 0; i < numSamples; ++i)
        {
            // Pink-ish or white noise. Just white noise for now.
            float noise = (randomGenerator.nextFloat() * 2.0f - 1.0f) * maxNoiseLevel;
            
            // Envelope following to only inject noise when audio is playing (analog tape hiss effect)
            float envelope = std::abs(channelData[i]) * 5.0f;
            envelope = juce::jlimit(0.01f, 1.0f, envelope);

            channelData[i] += noise * envelope;
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
