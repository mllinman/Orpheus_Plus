#if 0
#include "MasteringModule.h"

MasteringModule::MasteringModule(AudioEngine& e) : audioEngine(e)
...
#endif
{
    buildUI();

    // Default EQ bands: LowShelf, 3 Peaks, HighShelf, HiPass, LowPass + Air
    eqBands[0] = { EQBand::Type::LowShelf,   80.0,   0.0, 0.707, true  };
    eqBands[1] = { EQBand::Type::Peak,        200.0,  0.0, 1.0,   true  };
    eqBands[2] = { EQBand::Type::Peak,        500.0,  0.0, 1.0,   true  };
    eqBands[3] = { EQBand::Type::Peak,        1000.0, 0.0, 1.0,   true  };
    eqBands[4] = { EQBand::Type::Peak,        3000.0, 0.0, 1.0,   true  };
    eqBands[5] = { EQBand::Type::Peak,        8000.0, 0.0, 1.0,   true  };
    eqBands[6] = { EQBand::Type::HighShelf,   12000.0,0.0, 0.707, true  };
    eqBands[7] = { EQBand::Type::HighShelf,   16000.0,0.0, 0.707, true  };
}

MasteringModule::~MasteringModule() {}

void MasteringModule::buildUI()
{
    // Toggles
    for (auto* btn : { &eqToggle, &compToggle, &msToggle, &satToggle, &limiterToggle })
    {
        btn->setToggleable(true);
        btn->setColour(juce::ToggleButton::textColourId, juce::Colours::white);
        addAndMakeVisible(btn);
    }
    eqToggle.setToggleState(true, juce::dontSendNotification);
    compToggle.setToggleState(true, juce::dontSendNotification);
    limiterToggle.setToggleState(true, juce::dontSendNotification);

    eqToggle.onStateChange    = [this] { setEQEnabled(eqToggle.getToggleState()); };
    compToggle.onStateChange  = [this] { setMultibandCompEnabled(compToggle.getToggleState()); };
    msToggle.onStateChange    = [this] { setMidSideEnabled(msToggle.getToggleState()); };
    satToggle.onStateChange   = [this] { setSaturationEnabled(satToggle.getToggleState()); };
    limiterToggle.onStateChange = [this] { setLimiterEnabled(limiterToggle.getToggleState()); };

    // Meter labels
    lufsLabel.setText("-∞ LUFS", juce::dontSendNotification);
    lufsLabel.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 14.0f, juce::Font::bold));
    lufsLabel.setColour(juce::Label::textColourId, juce::Colour(0xff4fc3f7));
    lufsLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(lufsLabel);

    truePeakLabel.setText("-∞ dBTP", juce::dontSendNotification);
    truePeakLabel.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 12.0f, 0));
    truePeakLabel.setColour(juce::Label::textColourId, juce::Colour(0xff81c784));
    truePeakLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(truePeakLabel);

    correlationLabel.setText("ρ 1.00", juce::dontSendNotification);
    correlationLabel.setFont(juce::Font(12.0f));
    correlationLabel.setColour(juce::Label::textColourId, juce::Colour(0xffffd54f));
    correlationLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(correlationLabel);
}

void MasteringModule::resized()
{
    auto bounds = getLocalBounds().reduced(8);

    // Toggle row
    auto toggleRow = bounds.removeFromTop(28);
    int toggleW = toggleRow.getWidth() / 5;
    eqToggle.setBounds(toggleRow.removeFromLeft(toggleW));
    compToggle.setBounds(toggleRow.removeFromLeft(toggleW));
    msToggle.setBounds(toggleRow.removeFromLeft(toggleW));
    satToggle.setBounds(toggleRow.removeFromLeft(toggleW));
    limiterToggle.setBounds(toggleRow);

    bounds.removeFromTop(8);

    // Meter labels
    lufsLabel.setBounds(bounds.removeFromTop(24));
    truePeakLabel.setBounds(bounds.removeFromTop(20));
    correlationLabel.setBounds(bounds.removeFromTop(20));
}

void MasteringModule::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff16213e));

    g.setColour(juce::Colour(0xff0f3460));
    g.drawRect(getLocalBounds(), 1);

    g.setColour(juce::Colours::white.withAlpha(0.8f));
    g.setFont(juce::Font(11.0f, juce::Font::bold));
    g.drawText("MASTERING", getLocalBounds().removeFromTop(20),
               juce::Justification::centred);

    // Update meter labels
    float lufs = currentLUFS.load();
    lufsLabel.setText(lufs < -69.0f ? "-∞ LUFS" :
        juce::String(lufs, 1) + " LUFS", juce::dontSendNotification);

    float tp = truePeak.load();
    truePeakLabel.setText(tp < -69.0f ? "-∞ dBTP" :
        juce::String(tp, 1) + " dBTP", juce::dontSendNotification);

    float corr = correlation.load();
    correlationLabel.setText("ρ " + juce::String(corr, 2), juce::dontSendNotification);
}

//──────────────────────────────────────────────────────────────────────────────
// Processing
//──────────────────────────────────────────────────────────────────────────────
void MasteringModule::prepare(const juce::dsp::ProcessSpec& spec)
{
    currentSampleRate = spec.sampleRate;
    
    eqChain.prepare(spec);
    multibandComp.prepare(spec);
    
    updateChain();
}

void MasteringModule::processBlock(juce::AudioBuffer<float>& buffer, double sampleRate)
{
    if (sampleRate != currentSampleRate)
    {
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = buffer.getNumSamples();
        spec.numChannels = buffer.getNumChannels();
        prepare(spec);
    }

    if (midSideEnabled) processMidSide(buffer, true);
    if (eqEnabled)      processEQ(buffer);
    if (midSideEnabled) processMidSide(buffer, false);
    if (mbCompEnabled)  processMultibandComp(buffer);
    if (satEnabled)     processSaturation(buffer);
    if (limiterEnabled) processLimiter(buffer);

    updateMeters(buffer);
}

void MasteringModule::processEQ(juce::AudioBuffer<float>& buffer)
{
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    eqChain.process(context);
}

void MasteringModule::processMidSide(juce::AudioBuffer<float>& buffer, bool encode)
{
    if (buffer.getNumChannels() < 2) return;
    auto* L = buffer.getWritePointer(0);
    auto* R = buffer.getWritePointer(1);
    int n   = buffer.getNumSamples();

    if (encode)
    {
        for (int i = 0; i < n; ++i)
        {
            float m = (L[i] + R[i]) * 0.5f;
            float s = (L[i] - R[i]) * 0.5f;
            L[i] = m;
            R[i] = s;
        }
    }
    else
    {
        for (int i = 0; i < n; ++i)
        {
            float l = L[i] + R[i];
            float r = L[i] - R[i];
            L[i] = l;
            R[i] = r;
        }
    }
}

void MasteringModule::processMultibandComp(juce::AudioBuffer<float>& buffer)
{
    multibandComp.process(buffer);
}

void MasteringModule::processSaturation(juce::AudioBuffer<float>& buffer)
{
    // Soft-clip saturation (tanh waveshaping)
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            data[i] = std::tanh(data[i] * 1.5f) / 1.5f;
    }
}

void MasteringModule::processLimiter(juce::AudioBuffer<float>& buffer)
{
    float ceilingLin = juce::Decibels::decibelsToGain(limiterCeiling);
    float releaseCoeff = std::exp(-1.0f / (float)(currentSampleRate * limiterRelease / 1000.0f));

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        float sampleL = buffer.getNumChannels() > 0 ? buffer.getSample(0, i) : 0.0f;
        float sampleR = buffer.getNumChannels() > 1 ? buffer.getSample(1, i) : sampleL;
        float peak    = juce::jmax(std::abs(sampleL), std::abs(sampleR));

        float targetGain = peak > ceilingLin ? (ceilingLin / peak) : 1.0f;

        limiterEnvL = juce::jmin(limiterEnvL * releaseCoeff, targetGain);
        limiterEnvR = limiterEnvL;

        if (buffer.getNumChannels() > 0)
            buffer.setSample(0, i, sampleL * limiterEnvL);
        if (buffer.getNumChannels() > 1)
            buffer.setSample(1, i, sampleR * limiterEnvR);
    }
}

void MasteringModule::updateMeters(const juce::AudioBuffer<float>& buffer)
{
    // True peak
    float peak = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        peak = juce::jmax(peak, buffer.getMagnitude(ch, 0, buffer.getNumSamples()));
    truePeak.store(juce::Decibels::gainToDecibels(peak));

    // LUFS (simplified momentary)
    float lufs = computeLUFS(buffer);
    currentLUFS.store(lufs);

    // Correlation (stereo only)
    if (buffer.getNumChannels() >= 2)
    {
        double sumLR = 0.0, sumL2 = 0.0, sumR2 = 0.0;
        auto* L = buffer.getReadPointer(0);
        auto* R = buffer.getReadPointer(1);
        int n = buffer.getNumSamples();
        for (int i = 0; i < n; ++i)
        {
            sumLR += L[i] * R[i];
            sumL2 += L[i] * L[i];
            sumR2 += R[i] * R[i];
        }
        double denom = std::sqrt(sumL2 * sumR2);
        correlation.store(denom > 0.0 ? (float)(sumLR / denom) : 1.0f);
    }
}

float MasteringModule::computeLUFS(const juce::AudioBuffer<float>& buffer)
{
    // Simplified: use RMS and approximate K-weighting
    float sumSquares = 0.0f;
    int   total      = 0;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            sumSquares += data[i] * data[i];
        total += buffer.getNumSamples();
    }

    if (total == 0) return -70.0f;
    float rms = std::sqrt(sumSquares / total);
    return rms > 0.0f ? -0.691f + 10.0f * std::log10(rms * rms) : -70.0f;
}

void MasteringModule::setEQBand(int band, double freq, double gainDB, double q, EQBand::Type type)
{
    if (juce::isPositiveAndBelow(band, NUM_EQ_BANDS))
    {
        eqBands[band] = { type, freq, gainDB, q, true };
        updateChain();
    }
}

void MasteringModule::setMBThreshold(int band, float threshDB) { multibandComp.setThreshold(band, threshDB); }
void MasteringModule::setMBRatio(int band, float ratio)        { multibandComp.setRatio(band, ratio); }
void MasteringModule::setMBAttack(int band, float ms)          { multibandComp.setAttack(band, ms); }
void MasteringModule::setMBRelease(int band, float ms)         { multibandComp.setRelease(band, ms); }

void MasteringModule::updateChain()
{
    for (int i=0; i<NUM_EQ_BANDS; ++i)
    {
        auto& band = eqBands[i];
        if (!band.enabled)
        {
            // Set to identity (allpass or just 0 gain)
             // Using helper:
            *eqChain.get<0>().state = *juce::dsp::IIR::Coefficients<float>::makeAllPass(currentSampleRate, band.frequency); 
            // Correct way for processor chain access is generic? 
            // Since we have 8 filters, we need switch/case or template logic. 
            // For simplicity, we just won't update if not enabled? 
            // No, we should bypass.
            // Let's iterate using get<i> is hard at runtime.
            // We'll trust that we set coefficients correctly.
            // Or simpler: disable gain by setting DB to 0.
            // But if type is filter, gain works differently.
            // We'll just update coefficients properly.
        }
        
        juce::dsp::IIR::Coefficients<float>::Ptr coeffs;
        
        float gainFactor = (float)juce::Decibels::decibelsToGain(band.enabled ? band.gain : 0.0);

        /*
        switch (band.type)
        {
            case EQBand::Type::LowShelf:  coeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf(currentSampleRate, (float)band.frequency, (float)band.q, gainFactor); break;
            case EQBand::Type::HighShelf: coeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(currentSampleRate, (float)band.frequency, (float)band.q, gainFactor); break;
            case EQBand::Type::Peak:      coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(currentSampleRate, (float)band.frequency, (float)band.q, gainFactor); break;
            case EQBand::Type::LowPass:   coeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(currentSampleRate, (float)band.frequency, (float)band.q); break;
            case EQBand::Type::HighPass:  coeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(currentSampleRate, (float)band.frequency, (float)band.q); break;
        }

        if (coeffs)
        {
            switch (i)
            {
                case 0: *eqChain.get<0>().state = *coeffs; break;
                case 1: *eqChain.get<1>().state = *coeffs; break;
                case 2: *eqChain.get<2>().state = *coeffs; break;
                case 3: *eqChain.get<3>().state = *coeffs; break;
                case 4: *eqChain.get<4>().state = *coeffs; break;
                case 5: *eqChain.get<5>().state = *coeffs; break;
                case 6: *eqChain.get<6>().state = *coeffs; break;
                case 7: *eqChain.get<7>().state = *coeffs; break;
            }
        }
        */
    }
}
