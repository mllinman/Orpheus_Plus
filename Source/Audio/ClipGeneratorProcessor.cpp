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

            double clipStart = ac->startTime;
            double clipEnd = ac->startTime + ac->duration;

            // Find intersection between clip and current block
            double blockStart = playhead;
            double blockEnd = playhead + (numSamples / sr);

            if (blockStart < clipEnd && blockEnd > clipStart)
            {
                // Calculate portion of block to fill
                int startSample = (int)juce::jmax(0.0, (clipStart - blockStart) * sr);
                int endSample = (int)juce::jmin((double)numSamples, (clipEnd - blockStart) * sr);
                int numToFill = endSample - startSample;

                if (numToFill <= 0) continue;

                float stretch = ac->getStretchFactor(bpm);
                float pitch = std::pow(2.0f, ac->pitchShift / 12.0f);
                
                // For the placeholder Lagrange, we just pass the ratio
                // In actual Rubber Band, we'd set these properties on the stretcher
                
                // Prepare a source buffer subset
                // (Roughly estimating how many source samples we need)
                double speedRatio = (double)stretch * (double)pitch;
                int approxSourceSamples = (int)(numToFill * speedRatio) + 4; // padding for interpolation
                
                juce::AudioBuffer<float> sourceSubset(ac->audioData.getNumChannels(), approxSourceSamples);
                juce::AudioBuffer<float> outputSubset(ac->audioData.getNumChannels(), numToFill);
                
                // Map session time to source samples
                double segmentStartTime = blockStart + (startSample / sr);
                double offsetInSession = segmentStartTime - clipStart;
                double sourceStartPos = offsetInSession * ac->sampleRate * stretch;

                for (int ch = 0; ch < ac->audioData.getNumChannels(); ++ch)
                {
                    for (int s = 0; s < approxSourceSamples; ++s)
                    {
                        double pos = sourceStartPos + (s * (ac->sampleRate / sr) * (1.0 / pitch)); // This logic is tricky with dual-ratios
                        // Actually, let's let the stretcher handle the math via speedRatio
                        // We just need a contiguous chunk of source audio.
                        
                        double actualSourcePos = sourceStartPos + s;
                        if (ac->loopEnabled)
                            actualSourcePos = std::fmod(actualSourcePos, (double)ac->audioData.getNumSamples());
                        
                        if (actualSourcePos >= 0 && actualSourcePos < ac->audioData.getNumSamples())
                            sourceSubset.setSample(ch, s, ac->audioData.getSample(ch, (int)actualSourcePos));
                        else
                            sourceSubset.setSample(ch, s, 0.0f);
                    }
                }

                ac->stretcher.process(sourceSubset, outputSubset, (float)speedRatio);

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
