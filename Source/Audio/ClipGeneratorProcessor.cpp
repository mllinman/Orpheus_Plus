#include "ClipGeneratorProcessor.h"
#include "AudioEngine.h"

ClipGeneratorProcessor::ClipGeneratorProcessor(OrpheusTrackInfo& info, AudioEngine& engine)
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      trackInfo(info),
      audioEngine(engine)
{
}

ClipGeneratorProcessor::~ClipGeneratorProcessor()
{
}

void ClipGeneratorProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
}

void ClipGeneratorProcessor::releaseResources()
{
}

void ClipGeneratorProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    buffer.clear();

    if (!audioEngine.isPlaying() && !audioEngine.isExporting())
        return;

    double sr = getSampleRate();
    if (sr <= 0) sr = 44100.0;

    int numSamples = buffer.getNumSamples();
    double playhead = currentPlayhead.load();
    double endTime = playhead + (numSamples / sr);
    double bpm = audioEngine.getBpm();

    for (auto* clip : trackInfo.clips)
    {
        if (auto* ac = dynamic_cast<AudioClip*>(clip))
        {
            if (!ac->isLoaded) continue;

            // Safety: skip clips with no actual audio data
            int srcChannels = ac->audioData.getNumChannels();
            int srcSamples  = ac->audioData.getNumSamples();
            if (srcChannels <= 0 || srcSamples <= 0 || ac->sampleRate <= 0.0)
                continue;

            double clipStart = ac->startTime;
            double clipEnd = ac->startTime + ac->duration;

            // Sanity check duration
            if (ac->duration <= 0.0) continue;

            // Find intersection between clip and current block
            double blockStart = playhead;
            double blockEnd = playhead + (numSamples / sr);

            if (blockStart < clipEnd && blockEnd > clipStart)
            {
                // Calculate portion of block to fill
                int startSample = (int)juce::jmax(0.0, (clipStart - blockStart) * sr);
                int endSample = (int)juce::jmin((double)numSamples, (clipEnd - blockStart) * sr);
                int numToFill = endSample - startSample;

                if (numToFill <= 0 || startSample >= numSamples) continue;
                // Clamp numToFill to prevent buffer overrun
                numToFill = juce::jmin(numToFill, numSamples - startSample);

                float stretch = ac->getStretchFactor(bpm);
                if (stretch <= 0.0f) stretch = 1.0f; // safety
                float pitch = std::pow(2.0f, ac->pitchShift / 12.0f);
                if (pitch <= 0.0f) pitch = 1.0f; // safety
                
                double speedRatio = (double)stretch * (double)pitch;
                if (speedRatio <= 0.0) speedRatio = 1.0;
                int approxSourceSamples = (int)(numToFill * speedRatio) + 4;
                
                // Safety: cap source samples to prevent huge allocations
                approxSourceSamples = juce::jmin(approxSourceSamples, srcSamples);
                if (approxSourceSamples <= 0) continue;

                juce::AudioBuffer<float> sourceSubset(srcChannels, approxSourceSamples);
                juce::AudioBuffer<float> outputSubset(srcChannels, numToFill);
                sourceSubset.clear();
                outputSubset.clear();
                
                // Map session time to source samples
                double segmentStartTime = blockStart + (startSample / sr);
                double offsetInSession = segmentStartTime - clipStart;
                double sourceStartPos = offsetInSession * ac->sampleRate * stretch;

                for (int ch = 0; ch < srcChannels; ++ch)
                {
                    for (int s = 0; s < approxSourceSamples; ++s)
                    {
                        double actualSourcePos = sourceStartPos + s;
                        
                        if (ac->loopEnabled && srcSamples > 0)
                            actualSourcePos = std::fmod(actualSourcePos, (double)srcSamples);
                        
                        int idx = (int)actualSourcePos;
                        if (idx >= 0 && idx < srcSamples)
                            sourceSubset.setSample(ch, s, ac->audioData.getSample(ch, idx));
                    }
                }

                ac->stretcher.process(sourceSubset, outputSubset, (float)speedRatio);

                if (ac->muted) continue;

                // Apply fades and gain
                for (int s = 0; s < numToFill; ++s)
                {
                    double currentSampleTime = blockStart + ((double)(startSample + s) / sr);
                    double clipLocalTime = currentSampleTime - ac->startTime;
                    
                    float sampleGain = (float)ac->gain;
                    
                    if (ac->fadeIn > 0.0 && clipLocalTime < ac->fadeIn)
                    {
                        float fadeFactor = (float)(clipLocalTime / ac->fadeIn);
                        if (ac->fadeInCurve == Clip::FadeCurve::Exponential) fadeFactor = fadeFactor * fadeFactor;
                        else if (ac->fadeInCurve == Clip::FadeCurve::S_Curve) fadeFactor = 0.5f - 0.5f * std::cos(fadeFactor * juce::MathConstants<float>::pi);
                        sampleGain *= fadeFactor;
                    }
                    if (ac->fadeOut > 0.0 && clipLocalTime > ac->duration - ac->fadeOut)
                    {
                        float fadeFactor = (float)((ac->duration - clipLocalTime) / ac->fadeOut);
                        if (ac->fadeOutCurve == Clip::FadeCurve::Exponential) fadeFactor = fadeFactor * fadeFactor;
                        else if (ac->fadeOutCurve == Clip::FadeCurve::S_Curve) fadeFactor = 0.5f - 0.5f * std::cos(fadeFactor * juce::MathConstants<float>::pi);
                        sampleGain *= fadeFactor;
                    }
                    
                    if (sampleGain != 1.0f)
                    {
                        for (int ch = 0; ch < outputSubset.getNumChannels(); ++ch)
                            outputSubset.setSample(ch, s, outputSubset.getSample(ch, s) * sampleGain);
                    }
                }

                // Mix into main buffer
                for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                {
                    int srcCh = ch % outputSubset.getNumChannels();
                    buffer.addFrom(ch, startSample, outputSubset, srcCh, 0, numToFill);
                }
            }
        }
    }
}
