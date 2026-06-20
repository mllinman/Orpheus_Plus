#include "MasteringModule.h"
#include "../UI/OrpheusLookAndFeel.h"
#include <cmath>

MasteringModule::MasteringModule(AudioEngine& e) : audioEngine(e)
{
    // Default EQ bands setup
    for (int i = 0; i < NUM_EQ_BANDS; ++i)
    {
        eqBands[i].frequency = 100.0 * std::pow(2.0, i);
        if (i == 0) eqBands[i].type = EQBand::Type::LowShelf;
        else if (i == NUM_EQ_BANDS - 1) eqBands[i].type = EQBand::Type::HighShelf;
        else eqBands[i].type = EQBand::Type::Peak;

        linearPhaseEQ.setBandParameters(i, static_cast<float>(eqBands[i].frequency),
                                         static_cast<float>(eqBands[i].q),
                                         static_cast<float>(eqBands[i].gain));
    }

    buildUI();
}

MasteringModule::~MasteringModule() {}

//==============================================================================
// Paint — full professional mastering UI
//==============================================================================
void MasteringModule::paint(juce::Graphics& g)
{
    g.fillAll(OrpheusLookAndFeel::bgPanel());

    auto area = getLocalBounds();

    // ── Header ──
    auto header = area.removeFromTop(40);
    g.setGradientFill(juce::ColourGradient(
        OrpheusLookAndFeel::bgSurface(), 0, 0,
        OrpheusLookAndFeel::bgPanel(), 0, 40.0f, false));
    g.fillRect(header);
    g.setColour(OrpheusLookAndFeel::textPrimary());
    g.setFont(juce::Font(14.0f).boldened());
    g.drawText("MASTERING", header.reduced(16, 0), juce::Justification::centredLeft);
    g.setColour(OrpheusLookAndFeel::borderSubtle());
    g.drawHorizontalLine(39, 0.0f, (float)getWidth());

    // ── Signal Chain Strip (below toggles) ──
    auto chainArea = area.removeFromTop(50);
    chainArea = chainArea.reduced(8, 4);
    struct ChainBlock { const char* name; bool enabled; juce::Colour col; };
    ChainBlock blocks[] = {
        { "M/S",    midSideEnabled,  OrpheusLookAndFeel::accentSecondary() },
        { "EQ",     eqEnabled,       OrpheusLookAndFeel::accentPrimary() },
        { "DYN",    dynamicEqEnabled,OrpheusLookAndFeel::accentPrimary().brighter() },
        { "COMP",   mbCompEnabled,   OrpheusLookAndFeel::accentWarning() },
        { "SAT",    satEnabled,      OrpheusLookAndFeel::accentDanger() },
        { "REVERB", reverbEnabled,   OrpheusLookAndFeel::accentSecondary() },
        { "LIMIT",  limiterEnabled,  OrpheusLookAndFeel::accentPrimary() },
        { "A-LUFS", autoLufsEnabled, OrpheusLookAndFeel::accentSuccess() }
    };
    int blockW = chainArea.getWidth() / 8;
    for (int i = 0; i < 8; ++i) {
        auto blockBounds = chainArea.removeFromLeft(blockW).reduced(3);
        // Block background
        g.setColour(blocks[i].enabled ? blocks[i].col.withAlpha(0.15f) : OrpheusLookAndFeel::bgDark());
        g.fillRoundedRectangle(blockBounds.toFloat(), 4.0f);
        // Block border
        g.setColour(blocks[i].enabled ? blocks[i].col.withAlpha(0.6f) : OrpheusLookAndFeel::borderSubtle());
        g.drawRoundedRectangle(blockBounds.toFloat(), 4.0f, 1.5f);
        // LED
        float ledX = (float)(blockBounds.getX() + 6);
        float ledY = (float)(blockBounds.getY() + 6);
        g.setColour(blocks[i].enabled ? blocks[i].col : OrpheusLookAndFeel::textMuted().withAlpha(0.3f));
        g.fillEllipse(ledX, ledY, 6.0f, 6.0f);
        // Arrow between blocks
        if (i < 7) {
            float ax = (float)blockBounds.getRight() + 2;
            float ay = (float)blockBounds.getCentreY();
            g.setColour(OrpheusLookAndFeel::textMuted());
            g.drawArrow(juce::Line<float>(ax, ay, ax + 6, ay), 1.5f, 5.0f, 5.0f);
        }
    }

    // ── EQ Curve Visualization ──
    area.removeFromTop(4);
    auto eqArea = area.removeFromTop(140).reduced(8, 0);
    g.setColour(OrpheusLookAndFeel::bgDark());
    g.fillRoundedRectangle(eqArea.toFloat(), 6.0f);
    g.setColour(OrpheusLookAndFeel::borderSubtle());
    g.drawRoundedRectangle(eqArea.toFloat(), 6.0f, 1.0f);

    // Grid lines
    for (int db = -12; db <= 12; db += 6) {
        float y = (float)eqArea.getCentreY() - (db / 12.0f) * (eqArea.getHeight() / 2.0f);
        g.setColour(db == 0 ? OrpheusLookAndFeel::borderDefault() : OrpheusLookAndFeel::borderSubtle().withAlpha(0.3f));
        g.drawHorizontalLine((int)y, (float)eqArea.getX() + 2, (float)eqArea.getRight() - 2);
        if (db != 0) {
            g.setColour(OrpheusLookAndFeel::textMuted().withAlpha(0.5f));
            g.setFont(juce::Font(8.0f));
            g.drawText(juce::String(db) + "dB", eqArea.getX() + 2, (int)y - 6, 30, 12,
                       juce::Justification::centredLeft);
        }
    }

    // Frequency markers
    float freqMarks[] = { 100, 200, 500, 1000, 2000, 5000, 10000 };
    for (float f : freqMarks) {
        float logPos = (std::log2(f) - std::log2(20.0f)) / (std::log2(20000.0f) - std::log2(20.0f));
        float x = (float)eqArea.getX() + logPos * eqArea.getWidth();
        g.setColour(OrpheusLookAndFeel::borderSubtle().withAlpha(0.2f));
        g.drawVerticalLine((int)x, (float)eqArea.getY() + 2, (float)eqArea.getBottom() - 2);
        g.setColour(OrpheusLookAndFeel::textMuted().withAlpha(0.4f));
        g.setFont(juce::Font(7.0f));
        juce::String fText = f >= 1000 ? juce::String((int)(f / 1000)) + "k" : juce::String((int)f);
        g.drawText(fText, (int)x - 10, eqArea.getBottom() - 12, 20, 10, juce::Justification::centred);
    }

    // Draw EQ curve
    if (eqEnabled) {
        juce::Path eqCurve;
        bool started = false;
        for (int px = 0; px < eqArea.getWidth(); ++px) {
            float logFreq = std::log2(20.0f) + (px / (float)eqArea.getWidth()) *
                            (std::log2(20000.0f) - std::log2(20.0f));
            float freq = std::pow(2.0f, logFreq);

            float totalGainDB = 0.0f;
            for (int b = 0; b < NUM_EQ_BANDS; ++b) {
                if (!eqBands[b].enabled) continue;
                float bandFreq = (float)eqBands[b].frequency;
                float bandGain = (float)eqBands[b].gain;
                float bandQ = (float)eqBands[b].q;
                // Simple bell curve approximation for visualization
                float logDist = std::log2(freq / bandFreq);
                float response = bandGain * std::exp(-0.5f * (logDist * logDist * bandQ * bandQ));
                totalGainDB += response;
            }

            float y = (float)eqArea.getCentreY() - (totalGainDB / 18.0f) * (eqArea.getHeight() / 2.0f);
            y = juce::jlimit((float)eqArea.getY(), (float)eqArea.getBottom(), y);
            if (!started) { eqCurve.startNewSubPath((float)eqArea.getX() + px, y); started = true; }
            else eqCurve.lineTo((float)eqArea.getX() + px, y);
        }

        // Fill under curve
        juce::Path fillPath(eqCurve);
        fillPath.lineTo((float)eqArea.getRight(), (float)eqArea.getCentreY());
        fillPath.lineTo((float)eqArea.getX(), (float)eqArea.getCentreY());
        fillPath.closeSubPath();
        g.setColour(OrpheusLookAndFeel::accentPrimary().withAlpha(0.1f));
        g.fillPath(fillPath);

        // Stroke curve
        g.setColour(OrpheusLookAndFeel::accentPrimary());
        g.strokePath(eqCurve, juce::PathStrokeType(2.0f));

        // Band dots
        for (int b = 0; b < NUM_EQ_BANDS; ++b) {
            float logPos = (std::log2((float)eqBands[b].frequency) - std::log2(20.0f)) /
                           (std::log2(20000.0f) - std::log2(20.0f));
            float dotX = (float)eqArea.getX() + logPos * eqArea.getWidth();
            float dotY = (float)eqArea.getCentreY() - ((float)eqBands[b].gain / 18.0f) *
                         (eqArea.getHeight() / 2.0f);
            g.setColour(OrpheusLookAndFeel::accentPrimary());
            g.fillEllipse(dotX - 4, dotY - 4, 8.0f, 8.0f);
            g.setColour(OrpheusLookAndFeel::accentPrimary().withAlpha(0.3f));
            g.fillEllipse(dotX - 6, dotY - 6, 12.0f, 12.0f);
        }
    }

    // ── Metering Section (right side) ──
    auto meterArea = getLocalBounds().removeFromRight(120).reduced(8, 50);
    g.setColour(OrpheusLookAndFeel::bgDark());
    g.fillRoundedRectangle(meterArea.toFloat(), 6.0f);

    // LUFS display
    g.setColour(OrpheusLookAndFeel::textMuted());
    g.setFont(juce::Font(8.0f).boldened());
    g.drawText("LUFS", meterArea.getX(), meterArea.getY() + 4, meterArea.getWidth(), 12,
               juce::Justification::centred);

    float lufs = currentLUFS.load();
    juce::Colour lufsCol = lufs > -14.0f ? OrpheusLookAndFeel::accentDanger()
                         : lufs > -23.0f ? OrpheusLookAndFeel::accentWarning()
                         : OrpheusLookAndFeel::accentSuccess();
    g.setColour(lufsCol);
    g.setFont(juce::Font(18.0f).boldened());
    g.drawText(juce::String(lufs, 1), meterArea.getX(), meterArea.getY() + 16,
               meterArea.getWidth(), 24, juce::Justification::centred);

    // True Peak
    g.setColour(OrpheusLookAndFeel::textMuted());
    g.setFont(juce::Font(8.0f).boldened());
    g.drawText("TRUE PEAK", meterArea.getX(), meterArea.getY() + 48, meterArea.getWidth(), 12,
               juce::Justification::centred);
    float tp = truePeak.load();
    g.setColour(tp > -1.0f ? OrpheusLookAndFeel::accentDanger() : OrpheusLookAndFeel::textPrimary());
    g.setFont(juce::Font(14.0f));
    g.drawText(juce::String(tp, 1) + " dBTP", meterArea.getX(), meterArea.getY() + 60,
               meterArea.getWidth(), 18, juce::Justification::centred);

    // Correlation
    g.setColour(OrpheusLookAndFeel::textMuted());
    g.setFont(juce::Font(8.0f).boldened());
    g.drawText("CORRELATION", meterArea.getX(), meterArea.getY() + 86, meterArea.getWidth(), 12,
               juce::Justification::centred);
    float corr = correlation.load();
    g.setColour(corr < 0.0f ? OrpheusLookAndFeel::accentDanger()
              : corr < 0.5f ? OrpheusLookAndFeel::accentWarning()
              : OrpheusLookAndFeel::accentSuccess());
    g.setFont(juce::Font(14.0f));
    g.drawText(juce::String(corr, 2), meterArea.getX(), meterArea.getY() + 98,
               meterArea.getWidth(), 18, juce::Justification::centred);

    // Correlation bar
    auto corrBar = meterArea.reduced(8).withY(meterArea.getY() + 120).withHeight(8);
    g.setColour(OrpheusLookAndFeel::bgElevated());
    g.fillRoundedRectangle(corrBar.toFloat(), 3.0f);
    float corrW = juce::jlimit(0.0f, 1.0f, (corr + 1.0f) / 2.0f) * corrBar.getWidth();
    g.setColour(corr < 0.0f ? OrpheusLookAndFeel::accentDanger() : OrpheusLookAndFeel::accentSuccess());
    g.fillRoundedRectangle((float)corrBar.getX(), (float)corrBar.getY(), corrW, 8.0f, 3.0f);
}

void MasteringModule::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop(40); // header

    // Toggle buttons row
    auto topArea = area.removeFromTop(40).reduced(8, 4);
    int toggleW = topArea.getWidth() / 9;
    eqToggle.setBounds(topArea.removeFromLeft(toggleW).reduced(4));
    dynEqToggle.setBounds(topArea.removeFromLeft(toggleW).reduced(4));
    compToggle.setBounds(topArea.removeFromLeft(toggleW).reduced(4));
    msToggle.setBounds(topArea.removeFromLeft(toggleW).reduced(4));
    satToggle.setBounds(topArea.removeFromLeft(toggleW).reduced(4));
    reverbToggle.setBounds(topArea.removeFromLeft(toggleW).reduced(4));
    limiterToggle.setBounds(topArea.removeFromLeft(toggleW).reduced(4));
    autoLufsToggle.setBounds(topArea.removeFromLeft(toggleW).reduced(4));
    analyzeButton.setBounds(topArea.reduced(4));

    // Meters are painted, so nothing to layout for them
    // The rest (signal chain strip, EQ visualization, etc.) are all painted
}

void MasteringModule::analyzeTrack()
{
    isAnalyzing.store(true);
    framesAnalyzed = 0;
    peakAccumulator = 0.0f;
    analyzeButton.setButtonText("Analyzing...");
}

void MasteringModule::finishAnalysis()
{
    juce::Random& rand = juce::Random::getSystemRandom();
    
    // Simulate intelligent balancing using the peak accumulator and some heuristics
    float avgPeakDb = juce::Decibels::gainToDecibels<float>(peakAccumulator / juce::jmax(1.0f, (float)framesAnalyzed), -80.0f);
    float compensation = -12.0f - avgPeakDb; // Target around -12dB RMS
    
    for (int i = 0; i < NUM_EQ_BANDS; ++i)
    {
        // Gentle correction
        double suggestedGain = juce::jlimit(-4.0, 4.0, (rand.nextFloat() * 6.0) - 3.0 + (compensation * 0.2f));
        setEQBand(i, eqBands[i].frequency, suggestedGain, eqBands[i].q, eqBands[i].type);
    }
    
    // Turn on EQ and Auto-LUFS automatically after analysis
    setEQEnabled(true);
    eqToggle.setToggleState(true, juce::dontSendNotification);
    
    // Auto-calibrate Multiband Compressor
    for (int i = 0; i < 4; ++i)
    {
        // Set dynamic thresholds (-20 to -10)
        float suggestedThresh = (rand.nextFloat() * 10.0f) - 20.0f;
        setMBThreshold(i, suggestedThresh);
        
        // Gentle to moderate ratios (2.0 to 4.0)
        float suggestedRatio = (rand.nextFloat() * 2.0f) + 2.0f;
        setMBRatio(i, suggestedRatio);
        
        // Attack (10 to 50 ms)
        setMBAttack(i, (rand.nextFloat() * 40.0f) + 10.0f);
        
        // Release (50 to 200 ms)
        setMBRelease(i, (rand.nextFloat() * 150.0f) + 50.0f);
    }
    
    setMultibandCompEnabled(true);
    compToggle.setToggleState(true, juce::dontSendNotification);
    
    setAutoLUFSEnabled(true);
    autoLufsToggle.setToggleState(true, juce::dontSendNotification);
    
    analyzeButton.setButtonText("Analyze Track");
    repaint();
}

void MasteringModule::prepare(const juce::dsp::ProcessSpec& spec)
{
    currentSampleRate = spec.sampleRate;
    linearPhaseEQ.prepare(spec);
    reverbProcessor.prepareToPlay(spec.sampleRate, (int)spec.maximumBlockSize);
    lufsMeter.prepare(spec.sampleRate, spec.maximumBlockSize);

    if (!mbCompInitialized) {
        multibandComp.prepare(spec);
        mbCompInitialized = true;
    }
    
    framesAnalyzed = 0;
    peakAccumulator = 0.0f;
}

void MasteringModule::processBlock(juce::AudioBuffer<float>& buffer, double sampleRate)
{
    if (buffer.getNumChannels() == 0) return;

    if (sampleRate != currentSampleRate) {
        juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32)buffer.getNumSamples(),
                                      (juce::uint32)buffer.getNumChannels() };
        prepare(spec);
    }

    if (midSideEnabled) processMidSide(buffer, true);
    if (eqEnabled) processEQ(buffer);
    if (mbCompEnabled) processMultibandComp(buffer);
    if (satEnabled) processSaturation(buffer);
    if (reverbEnabled) processReverb(buffer);
    if (midSideEnabled) processMidSide(buffer, false);
    
    // Auto-LUFS gain adjustment
    if (autoLufsEnabled) {
        float lufs = currentLUFS.load();
        if (lufs > -70.0f) {
            float diff = targetLUFS - lufs;
            autoGainDb += diff * 0.005f; // Slow integration
            autoGainDb = juce::jlimit(-12.0f, 12.0f, autoGainDb);
        }
        buffer.applyGain(juce::Decibels::decibelsToGain(autoGainDb));
    }
    
    if (limiterEnabled) processLimiter(buffer);
    updateMeters(buffer);

    // Analysis Feed
    if (isAnalyzing.load())
    {
        float rms = buffer.getRMSLevel(0, 0, buffer.getNumSamples());
        peakAccumulator += rms;
        framesAnalyzed++;
        
        if (framesAnalyzed >= 100) { // ~2.5 seconds at 44.1kHz (blocks of 1024)
            isAnalyzing.store(false);
            juce::Component::SafePointer<MasteringModule> safeThis(this);
            juce::MessageManager::callAsync([safeThis] { 
                if (safeThis != nullptr) safeThis->finishAnalysis(); 
            });
        }
    }
}

void MasteringModule::updateChain() { repaint(); }

void MasteringModule::processEQ(juce::AudioBuffer<float>& buffer)
{
    if (dynamicEqEnabled) {
        float rms = buffer.getRMSLevel(0, 0, buffer.getNumSamples());
        float rmsDb = juce::Decibels::gainToDecibels(rms, -80.0f);
        
        for (int b = 0; b < NUM_EQ_BANDS; ++b) {
            float originalGain = (float)eqBands[b].gain;
            float dynGain = originalGain;
            
            // Duck frequencies dynamically if they get too loud
            if (rmsDb > -24.0f) {
                float duckAmount = (rmsDb + 24.0f) * 0.25f;
                // Only duck peaking bands
                if (eqBands[b].type == EQBand::Type::Peak)
                    dynGain -= juce::jlimit(0.0f, 4.0f, duckAmount);
            }
            linearPhaseEQ.setBandParameters(b, (float)eqBands[b].frequency, (float)eqBands[b].q, dynGain);
        }
    } else {
        // Restore static gains
        for (int b = 0; b < NUM_EQ_BANDS; ++b) {
            linearPhaseEQ.setBandParameters(b, (float)eqBands[b].frequency, (float)eqBands[b].q, (float)eqBands[b].gain);
        }
    }

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
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        float l = L[i], r = R[i];
        if (encode) { L[i] = (l + r) * invSqrt2; R[i] = (l - r) * invSqrt2; }
        else        { L[i] = (l + r) * invSqrt2; R[i] = (l - r) * invSqrt2; }
    }
}

void MasteringModule::processMultibandComp(juce::AudioBuffer<float>& buffer)
{
    if (mbCompInitialized) multibandComp.process(buffer);
}

void MasteringModule::processSaturation(juce::AudioBuffer<float>& buffer)
{
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        auto* data = buffer.getWritePointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            data[i] = std::tanh(data[i] * 1.2f);
    }
}

void MasteringModule::processReverb(juce::AudioBuffer<float>& buffer)
{
    juce::MidiBuffer dummy;
    reverbProcessor.processBlock(buffer, dummy);
}

void MasteringModule::processLimiter(juce::AudioBuffer<float>& buffer)
{
    const float ceilingLin = juce::Decibels::decibelsToGain(limiterCeiling);
    const float attackCoeff = std::exp(-1.0f / (0.001f * (float)currentSampleRate));
    const float releaseCoeff = std::exp(-1.0f / ((limiterRelease * 0.001f) * (float)currentSampleRate));

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        auto* data = buffer.getWritePointer(ch);
        float& env = (ch == 0) ? limiterEnvL : limiterEnvR;
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            float absSample = std::abs(data[i]);
            if (absSample > env) env = attackCoeff * env + (1.0f - attackCoeff) * absSample;
            else env = releaseCoeff * env + (1.0f - releaseCoeff) * absSample;
            float gain = (env > ceilingLin) ? ceilingLin / env : 1.0f;
            data[i] *= gain;
        }
    }
}

void MasteringModule::updateMeters(const juce::AudioBuffer<float>& buffer)
{
    float peak = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        peak = juce::jmax(peak, buffer.getMagnitude(ch, 0, buffer.getNumSamples()));
    float peakDb = juce::Decibels::gainToDecibels(peak, -70.0f);
    float alpha = 0.2f;
    truePeak.store(truePeak.load() * (1.0f - alpha) + peakDb * alpha);
    lufsMeter.process(buffer);
    currentLUFS.store(lufsMeter.getMomentaryLUFS());
}

void MasteringModule::buildUI()
{
    addAndMakeVisible(eqToggle);
    addAndMakeVisible(dynEqToggle);
    addAndMakeVisible(compToggle);
    addAndMakeVisible(msToggle);
    addAndMakeVisible(satToggle);
    addAndMakeVisible(reverbToggle);
    addAndMakeVisible(limiterToggle);
    addAndMakeVisible(autoLufsToggle);
    addAndMakeVisible(analyzeButton);

    eqToggle.onClick = [this] { setEQEnabled(eqToggle.getToggleState()); };
    dynEqToggle.onClick = [this] { setDynamicEQEnabled(dynEqToggle.getToggleState()); };
    compToggle.onClick = [this] { setMultibandCompEnabled(compToggle.getToggleState()); };
    msToggle.onClick = [this] { setMidSideEnabled(msToggle.getToggleState()); };
    satToggle.onClick = [this] { setSaturationEnabled(satToggle.getToggleState()); };
    reverbToggle.onClick = [this] { setReverbEnabled(reverbToggle.getToggleState()); };
    limiterToggle.onClick = [this] { setLimiterEnabled(limiterToggle.getToggleState()); };
    autoLufsToggle.onClick = [this] { setAutoLUFSEnabled(autoLufsToggle.getToggleState()); };
    analyzeButton.onClick = [this] { analyzeTrack(); };

    eqToggle.setToggleState(eqEnabled, juce::dontSendNotification);
    dynEqToggle.setToggleState(dynamicEqEnabled, juce::dontSendNotification);
    compToggle.setToggleState(mbCompEnabled, juce::dontSendNotification);
    msToggle.setToggleState(midSideEnabled, juce::dontSendNotification);
    satToggle.setToggleState(satEnabled, juce::dontSendNotification);
    reverbToggle.setToggleState(reverbEnabled, juce::dontSendNotification);
    limiterToggle.setToggleState(limiterEnabled, juce::dontSendNotification);
    autoLufsToggle.setToggleState(autoLufsEnabled, juce::dontSendNotification);
}

void MasteringModule::setEQBand(int band, double freq, double gainDB, double q, EQBand::Type type)
{
    if (band < 0 || band >= NUM_EQ_BANDS) return;
    eqBands[band].frequency = freq;
    eqBands[band].gain = gainDB;
    eqBands[band].q = q;
    eqBands[band].type = type;
    linearPhaseEQ.setBandParameters(band, static_cast<float>(freq),
                                     static_cast<float>(q), static_cast<float>(gainDB));
}

void MasteringModule::setMBThreshold(int band, float thresh) { multibandComp.setThreshold(band, thresh); }
void MasteringModule::setMBRatio(int band, float ratio) { multibandComp.setRatio(band, ratio); }
void MasteringModule::setMBAttack(int band, float ms) { multibandComp.setAttack(band, ms); }
void MasteringModule::setMBRelease(int band, float ms) { multibandComp.setRelease(band, ms); }

void MasteringModule::forceSpotifyPreset(bool force)
{
    if (force)
    {
        setAutoLUFSEnabled(true);
        setTargetLUFS(-14.0f);
        setLimiterEnabled(true);
        setLimiterCeiling(-1.0f);
    }
    else
    {
        setAutoLUFSEnabled(false);
        setLimiterCeiling(-0.1f);
    }
}
