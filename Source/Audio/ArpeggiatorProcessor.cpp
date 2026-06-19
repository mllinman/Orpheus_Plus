#include "ArpeggiatorProcessor.h"

ArpeggiatorProcessor::ArpeggiatorProcessor()
    : juce::AudioProcessor(juce::AudioProcessor::BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

ArpeggiatorProcessor::~ArpeggiatorProcessor() {}

void ArpeggiatorProcessor::prepareToPlay(double sampleRate, int)
{
    arpeggiator.setSampleRate(sampleRate);
}

void ArpeggiatorProcessor::releaseResources() {}

void ArpeggiatorProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear();
    
    // Assume 120 BPM if we can't get it from the host/playhead
    double bpm = 120.0;
    if (auto* playHead = getPlayHead())
    {
        if (auto pos = playHead->getPosition())
        {
            if (pos->getBpm().hasValue())
                bpm = *pos->getBpm();
        }
    }

    arpeggiator.setTempo(bpm);
    
    // Note: The new Arpeggiator class processes events natively within the block size
    arpeggiator.process(midiMessages, buffer.getNumSamples());
}
