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

            if (playhead < clipEnd && endTime > clipStart)
            {
                float stretch = ac->getStretchFactor(bpm);
                double sourceSr = ac->sampleRate;
                if (sourceSr <= 0) sourceSr = sr;

                int sourceNumSamples = ac->audioData.getNumSamples();
                if (sourceNumSamples == 0) continue;

                for (int i = 0; i < numSamples; ++i)
                {
                    double currentSessionTime = playhead + (i / sr);
                    if (currentSessionTime < clipStart || currentSessionTime >= clipEnd)
                        continue;

                    double offsetInSession = currentSessionTime - clipStart;
                    double sourcePosSamples = offsetInSession * sourceSr * stretch;

                    if (ac->loopEnabled)
                    {
                        sourcePosSamples = std::fmod(sourcePosSamples, (double)sourceNumSamples);
                    }
                    else if (sourcePosSamples >= sourceNumSamples)
                    {
                        continue;
                    }

                    int idx1 = (int)sourcePosSamples;
                    int idx2 = (idx1 + 1) % sourceNumSamples;
                    float fract = (float)(sourcePosSamples - idx1);

                    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                    {
                        int sourceCh = ch % ac->audioData.getNumChannels();
                        float s1 = ac->audioData.getSample(sourceCh, idx1);
                        float s2 = ac->audioData.getSample(sourceCh, idx2);
                        float interpolated = s1 + fract * (s2 - s1);

                        buffer.addSample(ch, i, interpolated);
                    }
                }
            }
        }
    }
}
