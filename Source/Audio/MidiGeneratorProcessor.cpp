#include "MidiGeneratorProcessor.h"
#include "../../Source/Timeline/MidiClip.h"

MidiGeneratorProcessor::MidiGeneratorProcessor(OrpheusTrackInfo& info, AudioEngine& engine)
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      trackInfo(info),
      audioEngine(engine)
{
}

MidiGeneratorProcessor::~MidiGeneratorProcessor()
{
}

void MidiGeneratorProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    synth.setup(sampleRate);
}

void MidiGeneratorProcessor::releaseResources()
{
}

void MidiGeneratorProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    if (!audioEngine.isPlaying() && !audioEngine.isExporting())
        return;

    double playhead = currentPlayhead.load();
    double sr = getSampleRate();
    int numSamples = buffer.getNumSamples();
    double endTime = playhead + (numSamples / sr);

    juce::MidiBuffer trackMidi;

    // Collect MIDI events from clips on this track that fall into the current block
    for (auto* clip : trackInfo.clips)
    {
        if (auto* mc = dynamic_cast<MidiClip*>(clip))
        {
            double clipStart = mc->startTime;
            double clipEnd = mc->startTime + mc->duration;

            if (playhead < clipEnd && endTime > clipStart)
            {
                // Simple version: iterate through the sequence
                // In a production DAW, we'd use a more efficient iterator
                auto& sequence = mc->midiData;
                for (int i = 0; i < sequence.getNumEvents(); ++i)
                {
                    auto* event = sequence.getEventPointer(i);
                    double eventTime = clipStart + (event->message.getTimeStamp() / mc->ppq * (60.0 / audioEngine.getBpm()));
                    
                    if (eventTime >= playhead && eventTime < endTime)
                    {
                        int sampleOffset = (int)((eventTime - playhead) * sr);
                        trackMidi.addEvent(event->message, sampleOffset);
                    }
                }
            }
        }
    }

    // Output the MIDI to the graph
    midiMessages.addEvents(trackMidi, 0, numSamples, 0);

    // Render the fallback synth ONLY if there are no plugins on this track
    bool hasPlugins = false;
    for (int slot : trackInfo.pluginSlots) {
        if (slot != -1) { hasPlugins = true; break; }
    }
    
    if (!hasPlugins)
    {
        synth.renderNextBlock(buffer, trackMidi, 0, numSamples);
    }
}
