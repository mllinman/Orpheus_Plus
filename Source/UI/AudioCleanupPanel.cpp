#include "AudioCleanupPanel.h"

AudioCleanupPanel::AudioCleanupPanel(AudioEngine& e)
    : audioEngine(e)
{
    // Master bypass
    masterBypass.setClickingTogglesState(true);
    masterBypass.setColour(juce::ToggleButton::tickColourId, OrpheusLookAndFeel::accentDanger());
    addAndMakeVisible(masterBypass);

    auto setupKnob = [](juce::Slider& s, double min, double max, double def, double step = 0.01) {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setRange(min, max, step);
        s.setValue(def, juce::dontSendNotification);
        s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    };
    auto setupLabel = [](juce::Label& l) {
        l.setFont(juce::Font(8.0f).boldened());
        l.setColour(juce::Label::textColourId, OrpheusLookAndFeel::textMuted());
        l.setJustificationType(juce::Justification::centred);
    };

    // ── Noise Reduction ──
    noiseEnable.setClickingTogglesState(true);
    noiseEnable.onClick = [this] { processor.setNoiseReductionEnabled(noiseEnable.getToggleState()); };
    addAndMakeVisible(noiseEnable);
    setupKnob(noiseAmount, 0.0, 1.0, 0.6, 0.01);
    noiseAmount.onValueChange = [this] { processor.setNoiseReductionAmount((float)noiseAmount.getValue()); };
    setupLabel(noiseAmountLabel);
    setupKnob(noiseGateThresh, -80.0, 0.0, -60.0, 0.5);
    noiseGateThresh.onValueChange = [this] { processor.setNoiseGateThreshold((float)noiseGateThresh.getValue()); };
    setupLabel(noiseGateLabel);
    learnNoiseBtn.setColour(juce::TextButton::buttonColourId, OrpheusLookAndFeel::accentPrimary().darker(0.3f));
    learnNoiseBtn.onClick = [this] { processor.captureNoiseProfile(); };
    addAndMakeVisible(noiseAmount);
    addAndMakeVisible(noiseAmountLabel);
    addAndMakeVisible(noiseGateThresh);
    addAndMakeVisible(noiseGateLabel);
    addAndMakeVisible(learnNoiseBtn);

    // ── De-Click ──
    deClickEnable.setClickingTogglesState(true);
    deClickEnable.onClick = [this] { processor.setDeClickEnabled(deClickEnable.getToggleState()); };
    addAndMakeVisible(deClickEnable);
    setupKnob(deClickSensitivity, 0.0, 1.0, 0.5, 0.01);
    setupLabel(deClickLabel);
    addAndMakeVisible(deClickSensitivity);
    addAndMakeVisible(deClickLabel);

    // ── De-Esser ──
    deEsserEnable.setClickingTogglesState(true);
    deEsserEnable.onClick = [this] { processor.setDeEsserEnabled(deEsserEnable.getToggleState()); };
    addAndMakeVisible(deEsserEnable);
    setupKnob(deEsserFreq, 2000.0, 12000.0, 7500.0, 100.0);
    deEsserFreq.onValueChange = [this] { processor.setDeEsserFrequency((float)deEsserFreq.getValue()); };
    setupLabel(deEsserFreqLabel);
    setupKnob(deEsserThresh, -40.0, 0.0, -20.0, 0.5);
    deEsserThresh.onValueChange = [this] { processor.setDeEsserThreshold((float)deEsserThresh.getValue()); };
    setupLabel(deEsserThreshLabel);
    setupKnob(deEsserRange, -24.0, 0.0, -12.0, 0.5);
    deEsserRange.onValueChange = [this] { processor.setDeEsserRange((float)deEsserRange.getValue()); };
    setupLabel(deEsserRangeLabel);
    addAndMakeVisible(deEsserFreq); addAndMakeVisible(deEsserFreqLabel);
    addAndMakeVisible(deEsserThresh); addAndMakeVisible(deEsserThreshLabel);
    addAndMakeVisible(deEsserRange); addAndMakeVisible(deEsserRangeLabel);

    // ── Hum Removal ──
    humEnable.setClickingTogglesState(true);
    humEnable.onClick = [this] { processor.setHumRemovalEnabled(humEnable.getToggleState()); };
    addAndMakeVisible(humEnable);
    humFreqCombo.addItem("50 Hz", 1);
    humFreqCombo.addItem("60 Hz", 2);
    humFreqCombo.setSelectedId(1, juce::dontSendNotification);
    humFreqCombo.onChange = [this] {
        processor.setHumFrequency(humFreqCombo.getSelectedId() == 1 ? 50.0f : 60.0f);
    };
    setupLabel(humFreqLabel);
    setupKnob(humHarmonics, 1.0, 8.0, 3.0, 1.0);
    humHarmonics.onValueChange = [this] { processor.setHumHarmonics((int)humHarmonics.getValue()); };
    setupLabel(humHarmonicsLabel);
    addAndMakeVisible(humFreqCombo); addAndMakeVisible(humFreqLabel);
    addAndMakeVisible(humHarmonics); addAndMakeVisible(humHarmonicsLabel);

    // ── DC Offset ──
    dcEnable.setClickingTogglesState(true);
    dcEnable.setToggleState(true, juce::dontSendNotification);
    dcEnable.onClick = [this] { processor.setDCOffsetEnabled(dcEnable.getToggleState()); };
    addAndMakeVisible(dcEnable);

    startTimerHz(10);
}

AudioCleanupPanel::~AudioCleanupPanel() { stopTimer(); }

void AudioCleanupPanel::timerCallback() { repaint(); }

void AudioCleanupPanel::paintModuleCard(juce::Graphics& g, juce::Rectangle<int> bounds,
                                         const juce::String& name, bool enabled)
{
    g.setColour(enabled ? OrpheusLookAndFeel::bgElevated() : OrpheusLookAndFeel::bgDark());
    g.fillRoundedRectangle(bounds.toFloat(), 6.0f);
    g.setColour(enabled ? OrpheusLookAndFeel::accentPrimary().withAlpha(0.5f)
                        : OrpheusLookAndFeel::borderSubtle());
    g.drawRoundedRectangle(bounds.toFloat(), 6.0f, 1.0f);

    // Status LED
    float ledX = (float)(bounds.getX() + 10);
    float ledY = (float)(bounds.getY() + 10);
    g.setColour(enabled ? OrpheusLookAndFeel::accentSuccess() : OrpheusLookAndFeel::textMuted().withAlpha(0.3f));
    g.fillEllipse(ledX, ledY, 8.0f, 8.0f);
    if (enabled) {
        g.setColour(OrpheusLookAndFeel::accentSuccess().withAlpha(0.3f));
        g.fillEllipse(ledX - 2, ledY - 2, 12.0f, 12.0f);
    }
}

void AudioCleanupPanel::paint(juce::Graphics& g)
{
    g.fillAll(OrpheusLookAndFeel::bgPanel());

    // Header
    auto header = getLocalBounds().removeFromTop(40);
    g.setGradientFill(juce::ColourGradient(
        OrpheusLookAndFeel::bgSurface(), 0, 0,
        OrpheusLookAndFeel::bgPanel(), 0, 40.0f, false));
    g.fillRect(header);
    g.setColour(OrpheusLookAndFeel::textPrimary());
    g.setFont(juce::Font(14.0f).boldened());
    g.drawText("AUDIO CLEANUP", header.reduced(16, 0), juce::Justification::centredLeft);

    // Module cards
    auto area = getLocalBounds().reduced(12).withTrimmedTop(52);
    int cardH = 130;
    int gap = 8;

    paintModuleCard(g, area.removeFromTop(cardH), "Noise Reduction", noiseEnable.getToggleState());
    area.removeFromTop(gap);
    paintModuleCard(g, area.removeFromTop(70), "De-Click", deClickEnable.getToggleState());
    area.removeFromTop(gap);
    paintModuleCard(g, area.removeFromTop(cardH), "De-Esser", deEsserEnable.getToggleState());
    area.removeFromTop(gap);
    paintModuleCard(g, area.removeFromTop(100), "Hum Removal", humEnable.getToggleState());
    area.removeFromTop(gap);
    paintModuleCard(g, area.removeFromTop(50), "DC Offset", dcEnable.getToggleState());
}

void AudioCleanupPanel::resized()
{
    auto area = getLocalBounds().reduced(12).withTrimmedTop(44);
    int knobSz = 40;

    // Master bypass
    masterBypass.setBounds(area.getRight() - 80, 10, 70, 24);

    area.removeFromTop(8);

    // ── Noise Reduction card ──
    auto noiseArea = area.removeFromTop(130).reduced(8);
    noiseEnable.setBounds(noiseArea.removeFromTop(24));
    auto noiseKnobRow = noiseArea.removeFromTop(knobSz + 14);
    auto nk1 = noiseKnobRow.removeFromLeft(knobSz + 8);
    noiseAmount.setBounds(nk1.removeFromTop(knobSz));
    noiseAmountLabel.setBounds(nk1);
    auto nk2 = noiseKnobRow.removeFromLeft(knobSz + 8);
    noiseGateThresh.setBounds(nk2.removeFromTop(knobSz));
    noiseGateLabel.setBounds(nk2);
    learnNoiseBtn.setBounds(noiseArea.removeFromTop(24).withWidth(150));
    area.removeFromTop(8);

    // ── De-Click card ──
    auto dcArea = area.removeFromTop(70).reduced(8);
    deClickEnable.setBounds(dcArea.removeFromTop(24));
    auto dcKnobRow = dcArea.removeFromTop(knobSz + 14);
    auto dk1 = dcKnobRow.removeFromLeft(knobSz + 8);
    deClickSensitivity.setBounds(dk1.removeFromTop(knobSz));
    deClickLabel.setBounds(dk1);
    area.removeFromTop(8);

    // ── De-Esser card ──
    auto deArea = area.removeFromTop(130).reduced(8);
    deEsserEnable.setBounds(deArea.removeFromTop(24));
    auto deKnobRow = deArea.removeFromTop(knobSz + 14);
    auto dek1 = deKnobRow.removeFromLeft(knobSz + 8);
    deEsserFreq.setBounds(dek1.removeFromTop(knobSz));
    deEsserFreqLabel.setBounds(dek1);
    auto dek2 = deKnobRow.removeFromLeft(knobSz + 8);
    deEsserThresh.setBounds(dek2.removeFromTop(knobSz));
    deEsserThreshLabel.setBounds(dek2);
    auto dek3 = deKnobRow.removeFromLeft(knobSz + 8);
    deEsserRange.setBounds(dek3.removeFromTop(knobSz));
    deEsserRangeLabel.setBounds(dek3);
    area.removeFromTop(8);

    // ── Hum Removal card ──
    auto humArea = area.removeFromTop(100).reduced(8);
    humEnable.setBounds(humArea.removeFromTop(24));
    auto humKnobRow = humArea.removeFromTop(knobSz + 14);
    humFreqLabel.setBounds(humKnobRow.removeFromLeft(60).removeFromTop(12));
    humFreqCombo.setBounds(humKnobRow.removeFromLeft(70).reduced(2));
    auto hk1 = humKnobRow.removeFromLeft(knobSz + 8);
    humHarmonics.setBounds(hk1.removeFromTop(knobSz));
    humHarmonicsLabel.setBounds(hk1);
    area.removeFromTop(8);

    // ── DC Offset card ──
    auto dcoArea = area.removeFromTop(50).reduced(8);
    dcEnable.setBounds(dcoArea.removeFromTop(24));
}
