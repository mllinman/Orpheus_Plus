#include "TrackFaderProcessor.h"

TrackFaderProcessor::TrackFaderProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::discreteChannels(12), true))
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
    delayLine.prepare(spec);

    updateSweetener();

    smoothVolume.reset(sampleRate, 0.05);
    smoothVolume.setCurrentAndTargetValue(currentVolume.load());
    smoothPan.reset(sampleRate, 0.05);
    smoothPan.setCurrentAndTargetValue(currentPan.load());
}

void TrackFaderProcessor::releaseResources()
{
}

void TrackFaderProcessor::setVolume(float gain)
{
    currentVolume.store(gain);
}

void TrackFaderProcessor::setPan(float p)
{
    currentPan.store(juce::jlimit(-1.0f, 1.0f, p));
}


void TrackFaderProcessor::setMute(bool shouldMute)
{
    muted.store(shouldMute);
}

void TrackFaderProcessor::setSweetener(float amount)
{
    sweetenerAmount.store(juce::jlimit(0.0f, 1.0f, amount));
    updateSweetener();
}

void TrackFaderProcessor::setDelaySamples(int samples)
{
    currentDelaySamples.store(samples);
}

void TrackFaderProcessor::setSpatialEnabled(bool enabled)
{
    spatialEnabled.store(enabled);
}

void TrackFaderProcessor::setSpatialPosition(float azimuth, float elevation, float distance)
{
    std::lock_guard<std::mutex> lock(pannerMutex);
    panner3D.setPosition({azimuth, elevation, distance});
}

void TrackFaderProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();

    if (numChannels == 0 || numSamples == 0)
        return;

    // Apply sweetener
    if (sweetenerAmount.load() > 0.01f)
    {
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        
        highShelf.process(context);
        lowShelf.process(context);
        compressor.process(context);
    }


    // Volume & Pan with Modulation
    float volMod = volumeModOffset.load();
    float panMod = panModOffset.load();
    
    float targetVol = currentVolume.load() + volMod;
    targetVol = juce::jlimit(0.0f, 2.0f, targetVol); // clamp
    
    float targetPan = currentPan.load() + panMod;
    targetPan = juce::jlimit(-1.0f, 1.0f, targetPan);

    smoothVolume.setTargetValue(muted.load() ? 0.0f : targetVol);
    smoothPan.setTargetValue(targetPan);
    
    if (smoothVolume.isSmoothing())
        smoothVolume.applyGain(buffer, numSamples);
    else
        buffer.applyGain(smoothVolume.getTargetValue());

    float pan = smoothPan.getCurrentValue();
    if (numChannels >= 2 && !spatialEnabled.load())
    {
        float leftGain = (pan <= 0.0f) ? 1.0f : 1.0f - pan;
        float rightGain = (pan >= 0.0f) ? 1.0f : 1.0f + pan;
        
        auto* outL = buffer.getWritePointer(0);
        auto* outR = buffer.getWritePointer(1);
        for (int i = 0; i < numSamples; ++i)
        {
            outL[i] *= leftGain;
            outR[i] *= rightGain;
        }
        
        // Silence channels 2-11
        for (int ch = 2; ch < numChannels; ++ch)
            buffer.clear(ch, 0, numSamples);
    }
    else if (spatialEnabled.load() && numChannels == 12)
    {
        std::array<float, 12> gains;
        {
            std::lock_guard<std::mutex> lock(pannerMutex);
            gains = panner3D.calculateGains();
        }
        
        // Calculate mono sum of the first two input channels (assuming stereo source)
        juce::AudioBuffer<float> monoSource(1, numSamples);
        monoSource.copyFrom(0, 0, buffer, 0, 0, numSamples);
        if (buffer.getNumChannels() > 1) {
            monoSource.addFrom(0, 0, buffer, 1, 0, numSamples);
            monoSource.applyGain(0.5f); // average
        }
        
        // Output to 12 channels
        for (int ch = 0; ch < 12; ++ch)
        {
            buffer.copyFrom(ch, 0, monoSource, 0, 0, numSamples);
            buffer.applyGain(ch, 0, numSamples, gains[ch]);
        }
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
    
    // 4. Delay Compensation
    int targetDelay = currentDelaySamples.load();
    if (targetDelay > 0)
    {
        delayLine.setDelay((float)targetDelay);
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        delayLine.process(context);
    }

    peakL.store(currentPeakL);
    peakR.store(currentPeakR);
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
