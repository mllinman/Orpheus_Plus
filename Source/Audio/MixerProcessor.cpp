#include "MixerProcessor.h"
#include "../Mastering/MasteringModule.h"

MixerProcessor::MixerProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::discreteChannels(12), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

MixerProcessor::~MixerProcessor()
{
}

void MixerProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    smoothVolume.reset(sampleRate, 0.05);
    smoothVolume.setCurrentAndTargetValue(masterVolume.load());

    if (masteringModule)
    {
        juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32)samplesPerBlock, (juce::uint32)getTotalNumOutputChannels() };
        masteringModule->prepare(spec);
    }
}

void MixerProcessor::releaseResources()
{
}

void MixerProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    int numSamples = buffer.getNumSamples();

    // Clear output channels that aren't inputs
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, numSamples);

    // Binaural Downmix from 12 channels to 2 channels
    if (totalNumInputChannels >= 12 && totalNumOutputChannels == 2)
    {
        // Simple Downmix Matrix for 7.1.4 to Stereo (Binaural approximation)
        // L = L + C*0.7 + Ls*0.8 + Lss*0.6 + Ltf*0.5 + Ltr*0.6 + LFE*0.1
        // R = R + C*0.7 + Rs*0.8 + Rss*0.6 + Rtf*0.5 + Rtr*0.6 + LFE*0.1
        
        juce::AudioBuffer<float> downmix(2, numSamples);
        downmix.clear();
        
        auto* lOut = downmix.getWritePointer(0);
        auto* rOut = downmix.getWritePointer(1);
        
        auto* cL   = buffer.getReadPointer(0); // L
        auto* cR   = buffer.getReadPointer(1); // R
        auto* cC   = buffer.getReadPointer(2); // C
        auto* cLFE = buffer.getReadPointer(3); // LFE
        auto* cLs  = buffer.getReadPointer(4); // Ls
        auto* cRs  = buffer.getReadPointer(5); // Rs
        auto* cLss = buffer.getReadPointer(6); // Lss
        auto* cRss = buffer.getReadPointer(7); // Rss
        auto* cLtf = buffer.getReadPointer(8); // Ltf
        auto* cRtf = buffer.getReadPointer(9); // Rtf
        auto* cLtr = buffer.getReadPointer(10); // Ltr
        auto* cRtr = buffer.getReadPointer(11); // Rtr

        for (int i = 0; i < numSamples; ++i)
        {
            lOut[i] = cL[i] + cC[i]*0.707f + cLs[i]*0.8f + cLss[i]*0.6f + cLtf[i]*0.5f + cLtr[i]*0.6f + cLFE[i]*0.1f;
            rOut[i] = cR[i] + cC[i]*0.707f + cRs[i]*0.8f + cRss[i]*0.6f + cRtf[i]*0.5f + cRtr[i]*0.6f + cLFE[i]*0.1f;
        }

        buffer.copyFrom(0, 0, downmix, 0, 0, numSamples);
        buffer.copyFrom(1, 0, downmix, 1, 0, numSamples);
        
        // Zero out the rest
        for (int ch = 2; ch < buffer.getNumChannels(); ++ch)
            buffer.clear(ch, 0, numSamples);
    }

    // Apply Mastering
    if (masteringModule)
    {
        masteringModule->processBlock(buffer, getSampleRate());
    }

    // Apply master volume
    smoothVolume.setTargetValue(masterVolume.load());
    
    if (smoothVolume.isSmoothing())
        smoothVolume.applyGain(buffer, numSamples);
    else
        buffer.applyGain(smoothVolume.getTargetValue());

    // Calculate RMS for metering
    if (buffer.getNumChannels() > 0)
        rmsLeft.store(buffer.getRMSLevel(0, 0, numSamples));
    
    if (buffer.getNumChannels() > 1)
        rmsRight.store(buffer.getRMSLevel(1, 0, numSamples));
}

void MixerProcessor::setMasterVolume(float vol)
{
    masterVolume.store(vol);
}
