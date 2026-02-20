#include "MasteringModule.h"

MasteringModule::MasteringModule(AudioEngine& e) : audioEngine(e) 
{
    // Default EQ bands setup
    for (int i = 0; i < NUM_EQ_BANDS; ++i)
    {
        eqBands[i].frequency = 100.0 * std::pow(2.0, i);
        if (i == 0) eqBands[i].type = EQBand::Type::LowShelf;
        else if (i == NUM_EQ_BANDS - 1) eqBands[i].type = EQBand::Type::HighShelf;
        else eqBands[i].type = EQBand::Type::Peak;
        
        linearPhaseEQ.setBandParameters(i, static_cast<float>(eqBands[i].frequency), static_cast<float>(eqBands[i].q), static_cast<float>(eqBands[i].gain));
    }

    buildUI();
}

MasteringModule::~MasteringModule() {}

void MasteringModule::paint(juce::Graphics& g) 
{
    g.fillAll(juce::Colour(0xff121212)); // dark background
}

void MasteringModule::resized() 
{
    auto area = getLocalBounds();
    auto topArea = area.removeFromTop(40);
    eqToggle.setBounds(topArea.removeFromLeft(60).reduced(5));
    compToggle.setBounds(topArea.removeFromLeft(60).reduced(5));
    msToggle.setBounds(topArea.removeFromLeft(60).reduced(5));
    satToggle.setBounds(topArea.removeFromLeft(60).reduced(5));
    limiterToggle.setBounds(topArea.removeFromLeft(60).reduced(5));
    
    // Layout for the rest of UI would go here
}

void MasteringModule::prepare(const juce::dsp::ProcessSpec& spec) 
{
    currentSampleRate = spec.sampleRate;
    
    linearPhaseEQ.prepare(spec);
    lufsMeter.prepare(spec.sampleRate, spec.maximumBlockSize);
    
    if (!mbCompInitialized) {
        multibandComp.prepare(spec);
        mbCompInitialized = true;
    }
}

void MasteringModule::processBlock(juce::AudioBuffer<float>& buffer, double sampleRate) 
{
    if (buffer.getNumChannels() == 0) return;

    if (sampleRate != currentSampleRate) {
        juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32)buffer.getNumSamples(), (juce::uint32)buffer.getNumChannels() };
        prepare(spec);
    }

    // Mastering Chain Pipeline
    if (midSideEnabled) processMidSide(buffer, true); // encode to M/S
    
    if (eqEnabled) processEQ(buffer);
    if (mbCompEnabled) processMultibandComp(buffer);
    if (satEnabled) processSaturation(buffer);
    
    if (midSideEnabled) processMidSide(buffer, false); // decode back to L/R
    
    if (limiterEnabled) processLimiter(buffer);
    
    updateMeters(buffer);
}

void MasteringModule::updateChain() 
{
    // Used to refresh UI or internal state when chain order/enablement changes
    repaint();
}

void MasteringModule::processEQ(juce::AudioBuffer<float>& buffer) 
{
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    linearPhaseEQ.process(context);
}

void MasteringModule::processMidSide(juce::AudioBuffer<float>& buffer, bool encode) 
{
    if (buffer.getNumChannels() < 2) return;
    
    const float invSqrt2 = 0.70710678f;
    auto* L = buffer.getWritePointer(0);
    auto* R = buffer.getWritePointer(1);
    
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        float l = L[i];
        float r = R[i];
        if (encode) {
            L[i] = (l + r) * invSqrt2; // Mid
            R[i] = (l - r) * invSqrt2; // Side
        } else {
            L[i] = (l + r) * invSqrt2; // Left
            R[i] = (l - r) * invSqrt2; // Right
        }
    }
}

void MasteringModule::processMultibandComp(juce::AudioBuffer<float>& buffer) 
{
    if (mbCompInitialized) {
        multibandComp.process(buffer);
    }
}

void MasteringModule::processSaturation(juce::AudioBuffer<float>& buffer) 
{
    // Simple soft clipping
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float x = data[i];
            // Fast soft clip using tanh
            data[i] = std::tanh(x * 1.2f);
        }
    }
}

void MasteringModule::processLimiter(juce::AudioBuffer<float>& buffer) 
{
    const float ceilingLin = juce::Decibels::decibelsToGain(limiterCeiling);
    const float attackCoeff = std::exp(-1.0f / (0.001f * (float)currentSampleRate)); // 1ms lookahead attack simulation
    const float releaseCoeff = std::exp(-1.0f / ((limiterRelease * 0.001f) * (float)currentSampleRate));
    
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        float& env = (ch == 0) ? limiterEnvL : limiterEnvR;
        
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float absSample = std::abs(data[i]);
            if (absSample > env) {
                env = attackCoeff * env + (1.0f - attackCoeff) * absSample;
            } else {
                env = releaseCoeff * env + (1.0f - releaseCoeff) * absSample;
            }
            
            float gain = 1.0f;
            if (env > ceilingLin) {
                gain = ceilingLin / env;
            }
            
            data[i] *= gain;
        }
    }
}

void MasteringModule::updateMeters(const juce::AudioBuffer<float>& buffer) 
{
    // Rough Peak
    float peak = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        peak = juce::jmax(peak, buffer.getMagnitude(ch, 0, buffer.getNumSamples()));
    }
    float peakDb = juce::Decibels::gainToDecibels(peak, -70.0f);
    
    float alpha = 0.2f; // smoothing
    truePeak.store(truePeak.load() * (1.0f - alpha) + peakDb * alpha);
    
    // LUFS calc
    lufsMeter.process(buffer);
    currentLUFS.store(lufsMeter.getMomentaryLUFS());
}

void MasteringModule::buildUI() 
{
    addAndMakeVisible(eqToggle);
    addAndMakeVisible(compToggle);
    addAndMakeVisible(msToggle);
    addAndMakeVisible(satToggle);
    addAndMakeVisible(limiterToggle);
    
    eqToggle.onClick = [this] { setEQEnabled(eqToggle.getToggleState()); };
    compToggle.onClick = [this] { setMultibandCompEnabled(compToggle.getToggleState()); };
    msToggle.onClick = [this] { setMidSideEnabled(msToggle.getToggleState()); };
    satToggle.onClick = [this] { setSaturationEnabled(satToggle.getToggleState()); };
    limiterToggle.onClick = [this] { setLimiterEnabled(limiterToggle.getToggleState()); };
    
    eqToggle.setToggleState(eqEnabled, juce::dontSendNotification);
    compToggle.setToggleState(mbCompEnabled, juce::dontSendNotification);
    msToggle.setToggleState(midSideEnabled, juce::dontSendNotification);
    satToggle.setToggleState(satEnabled, juce::dontSendNotification);
    limiterToggle.setToggleState(limiterEnabled, juce::dontSendNotification);
}

void MasteringModule::setEQBand(int band, double freq, double gainDB, double q, EQBand::Type type) 
{
    if (band < 0 || band >= NUM_EQ_BANDS) return;
    
    eqBands[band].frequency = freq;
    eqBands[band].gain = gainDB;
    eqBands[band].q = q;
    eqBands[band].type = type;
    
    linearPhaseEQ.setBandParameters(band, static_cast<float>(freq), static_cast<float>(q), static_cast<float>(gainDB));
}

void MasteringModule::setMBThreshold(int band, float thresh) { multibandComp.setThreshold(band, thresh); }
void MasteringModule::setMBRatio(int band, float ratio) { multibandComp.setRatio(band, ratio); }
void MasteringModule::setMBAttack(int band, float ms) { multibandComp.setAttack(band, ms); }
void MasteringModule::setMBRelease(int band, float ms) { multibandComp.setRelease(band, ms); }
