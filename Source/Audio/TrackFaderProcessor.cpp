#include "TrackFaderProcessor.h"

TrackFaderProcessor::TrackFaderProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

TrackFaderProcessor::~TrackFaderProcessor()
{
}

void TrackFaderProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();

    compressor.prepare(spec);
    highShelf.prepare(spec);
    lowShelf.prepare(spec);

    updateSweetener();

    smoothVolume.reset(sampleRate, 0.05);
    smoothPan.reset(sampleRate, 0.05);
}

void TrackFaderProcessor::releaseResources()
{
}

void TrackFaderProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    if (muted.load())
    {
        buffer.clear();
        return;
    }

    // 1. Process Channel Strip Sweetener
    float sweetAmt = sweetenerAmount.load();
    if (sweetAmt > 0.0f)
    {
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        
        lowShelf.process(context);
        compressor.process(context);
        highShelf.process(context);
    }

    // 2. Volume & Pan
    smoothVolume.setTargetValue(currentVolume.load());
    smoothPan.setTargetValue(currentPan.load());

    if (smoothVolume.isSmoothing())
        smoothVolume.applyGain(buffer, buffer.getNumSamples());
    else
        buffer.applyGain(smoothVolume.getTargetValue());

    float pan = smoothPan.getCurrentValue();
    if (buffer.getNumChannels() == 2)
    {
        float leftGain = (pan <= 0.0f) ? 1.0f : 1.0f - pan;
        float rightGain = (pan >= 0.0f) ? 1.0f : 1.0f + pan;
        buffer.applyGain(0, 0, buffer.getNumSamples(), leftGain);
        buffer.applyGain(1, 0, buffer.getNumSamples(), rightGain);
    }

    // 3. Peaks
    float currentPeakL = peakL.load();
    float currentPeakR = peakR.load();
    float attackCoef = 0.9f;
    float releaseCoef = 0.999f;
    
    if (buffer.getNumChannels() > 0)
    {
        auto* chL = buffer.getReadPointer(0);
        for (int i=0; i < buffer.getNumSamples(); ++i)
        {
            float envIn = std::abs(chL[i]);
            if (currentPeakL < envIn) currentPeakL += attackCoef * (envIn - currentPeakL);
            else                      currentPeakL *= releaseCoef;
        }
    }
    if (buffer.getNumChannels() > 1)
    {
        auto* chR = buffer.getReadPointer(1);
        for (int i=0; i < buffer.getNumSamples(); ++i)
        {
            float envIn = std::abs(chR[i]);
            if (currentPeakR < envIn) currentPeakR += attackCoef * (envIn - currentPeakR);
            else                      currentPeakR *= releaseCoef;
        }
    }
    peakL.store(currentPeakL);
    peakR.store(currentPeakR);
}

void TrackFaderProcessor::setVolume(float vol) { currentVolume.store(vol); }
void TrackFaderProcessor::setPan(float pan)    { currentPan.store(pan); }
void TrackFaderProcessor::setMute(bool m)      { muted.store(m); }

void TrackFaderProcessor::setSweetener(float amount)
{
    sweetenerAmount.store(amount);
    updateSweetener();
}

void TrackFaderProcessor::updateSweetener()
{
    float amt = sweetenerAmount.load();
    compressor.setRatio(1.0f + amt * 3.0f);
    compressor.setThreshold(0.0f - amt * 20.0f);
    compressor.setAttack(15.0f);
    compressor.setRelease(150.0f);

    double sr = getSampleRate();
    if (sr <= 0) sr = 44100.0;

    float highGain = juce::Decibels::decibelsToGain(amt * 6.0f);
    *highShelf.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(sr, 8000.0f, 0.707f, highGain);

    float lowGain = juce::Decibels::decibelsToGain(amt * 3.0f);
    *lowShelf.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(sr, 150.0f, 0.707f, lowGain);
}
