#include "AutoTunePanel.h"
#include "../Project/AppState.h"
#include <cmath>
#include <functional>

AutoTunePanel::AutoTunePanel(AudioEngine& e, AppState& state)
    : audioEngine(e), appState(state)
{
    auto setupKnob = [](juce::Slider& s, double min, double max, double def, double step = 0.01) {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setRange(min, max, step);
        s.setValue(def, juce::dontSendNotification);
        s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    };
    auto setupLabel = [](juce::Label& l) {
        l.setFont(juce::Font(9.0f).boldened());
        l.setColour(juce::Label::textColourId, OrpheusLookAndFeel::textMuted());
        l.setJustificationType(juce::Justification::centred);
    };
    auto setupReadout = [](juce::Label& l) {
        l.setFont(juce::Font(10.0f));
        l.setColour(juce::Label::textColourId, OrpheusLookAndFeel::accentSecondary());
        l.setJustificationType(juce::Justification::centred);
    };

    // Enable / Bypass
    enableToggle.setClickingTogglesState(true);
    enableToggle.onClick = [this] { if (auto* p = getProcessor()) p->setEnabled(enableToggle.getToggleState()); };
    bypassToggle.setClickingTogglesState(true);
    neuralModeToggle.setClickingTogglesState(true);
    neuralModeToggle.onClick = [this] { if (auto* p = getProcessor()) p->setNeuralMode(neuralModeToggle.getToggleState()); };
    addAndMakeVisible(enableToggle);
    addAndMakeVisible(bypassToggle);
    addAndMakeVisible(neuralModeToggle);

    // Key selector
    const char* noteNames[] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
    for (int i = 0; i < 12; ++i)
        keyCombo.addItem(noteNames[i], i + 1);
    keyCombo.setSelectedId(1, juce::dontSendNotification);
    keyCombo.onChange = [this] { if (auto* p = getProcessor()) p->setKey(keyCombo.getSelectedId() - 1); repaint(); };
    setupLabel(keyLabel);
    addAndMakeVisible(keyLabel);
    addAndMakeVisible(keyCombo);

    // Scale selector
    scaleCombo.addItem("Chromatic", 1);
    scaleCombo.addItem("Major", 2);
    scaleCombo.addItem("Minor", 3);
    scaleCombo.setSelectedId(2, juce::dontSendNotification);
    scaleCombo.onChange = [this] { if (auto* p = getProcessor()) p->setScale(scaleCombo.getSelectedId() - 1); repaint(); };
    setupLabel(scaleLabel);
    addAndMakeVisible(scaleLabel);
    addAndMakeVisible(scaleCombo);

    auto setupCtrl = [this, setupKnob, setupLabel, setupReadout](ParameterControl& pc, const juce::String& name, double min, double max, double def, double step) {
        setupKnob(pc.knob, min, max, def, step);
        setupLabel(pc.label);
        pc.label.setText(name, juce::dontSendNotification);
        setupReadout(pc.readout);
        pc.readout.setText(juce::String(def, 2), juce::dontSendNotification);
        addAndMakeVisible(pc.knob);
        addAndMakeVisible(pc.label);
        addAndMakeVisible(pc.readout);
    };

    setupCtrl(pitchCtrl, "PITCH", 0.0, 1.0, 0.5, 0.01);
    pitchCtrl.knob.onValueChange = [this] {
        if (auto* p = getProcessor()) p->setRetuneSpeed((float)pitchCtrl.knob.getValue());
        pitchCtrl.readout.setText(juce::String(pitchCtrl.knob.getValue(), 2), juce::dontSendNotification);
    };

    setupCtrl(volumeCtrl, "VOLUME", 0.0, 2.0, 1.0, 0.01);
    volumeCtrl.knob.onValueChange = [this] {
        if (auto* p = getProcessor()) p->setVolumeLevel((float)volumeCtrl.knob.getValue());
        volumeCtrl.readout.setText(juce::String(volumeCtrl.knob.getValue(), 2), juce::dontSendNotification);
    };

    setupCtrl(toneCtrl, "TONE", -12.0, 12.0, 0.0, 0.1);
    toneCtrl.knob.onValueChange = [this] {
        if (auto* p = getProcessor()) p->setFormantShift((float)toneCtrl.knob.getValue());
        toneCtrl.readout.setText(juce::String(toneCtrl.knob.getValue(), 1), juce::dontSendNotification);
    };

    setupCtrl(paceCtrl, "PACE", 0.5, 2.0, 1.0, 0.01);
    paceCtrl.knob.onValueChange = [this] {
        if (auto* p = getProcessor()) p->setPaceStretch((float)paceCtrl.knob.getValue());
        paceCtrl.readout.setText(juce::String(paceCtrl.knob.getValue(), 2), juce::dontSendNotification);
    };

    setupCtrl(rhythmCtrl, "RHYTHM", 0.0, 1.0, 0.0, 0.01);
    rhythmCtrl.knob.onValueChange = [this] {
        if (auto* p = getProcessor()) p->setRhythmQuantize((float)rhythmCtrl.knob.getValue());
        rhythmCtrl.readout.setText(juce::String(rhythmCtrl.knob.getValue(), 2), juce::dontSendNotification);
    };

    setupCtrl(articulationCtrl, "ARTICULATION", 0.0, 1.0, 0.0, 0.01);
    articulationCtrl.knob.onValueChange = [this] {
        if (auto* p = getProcessor()) p->setArticulation((float)articulationCtrl.knob.getValue());
        articulationCtrl.readout.setText(juce::String(articulationCtrl.knob.getValue(), 2), juce::dontSendNotification);
    };

    setupCtrl(resonanceCtrl, "RESONANCE", 0.0, 1.0, 0.5, 0.01);
    resonanceCtrl.knob.onValueChange = [this] {
        if (auto* p = getProcessor()) p->setResonance((float)resonanceCtrl.knob.getValue());
        resonanceCtrl.readout.setText(juce::String(resonanceCtrl.knob.getValue(), 2), juce::dontSendNotification);
    };

    setupCtrl(inflectionCtrl, "INFLECTION", -1.0, 1.0, 0.0, 0.01);
    inflectionCtrl.knob.onValueChange = [this] {
        if (auto* p = getProcessor()) p->setInflection((float)inflectionCtrl.knob.getValue());
        inflectionCtrl.readout.setText(juce::String(inflectionCtrl.knob.getValue(), 2), juce::dontSendNotification);
    };

    setupCtrl(emphasisCtrl, "EMPHASIS", 0.0, 1.0, 0.5, 0.01);
    emphasisCtrl.knob.onValueChange = [this] {
        if (auto* p = getProcessor()) p->setEmphasis((float)emphasisCtrl.knob.getValue());
        emphasisCtrl.readout.setText(juce::String(emphasisCtrl.knob.getValue(), 2), juce::dontSendNotification);
    };

    setupCtrl(projectionCtrl, "PROJECTION", 0.0, 1.0, 0.5, 0.01);
    projectionCtrl.knob.onValueChange = [this] {
        if (auto* p = getProcessor()) p->setProjection((float)projectionCtrl.knob.getValue());
        projectionCtrl.readout.setText(juce::String(projectionCtrl.knob.getValue(), 2), juce::dontSendNotification);
    };

    startTimerHz(20);
}

AutoTunePanel::~AutoTunePanel() { stopTimer(); }

VocalSuiteProcessor* AutoTunePanel::getProcessor()
{
    const int trackIndex = appState.getSelectedTrackIndex();
    if (trackIndex < 0)
        return nullptr;

    return audioEngine.getVocalSuiteForTrack(trackIndex);
}

void AutoTunePanel::timerCallback()
{
    int currentTrackIndex = appState.getSelectedTrackIndex();
    auto* p = getProcessor();

    if (p != nullptr) {
        if (currentTrackIndex != lastTrackIndex && currentTrackIndex >= 0) {
            lastTrackIndex = currentTrackIndex;
            keyCombo.setSelectedId(p->getKey() + 1, juce::dontSendNotification);
            scaleCombo.setSelectedId(p->getScale() + 1, juce::dontSendNotification);
            neuralModeToggle.setToggleState(p->getNeuralMode(), juce::dontSendNotification);
            
            pitchCtrl.knob.setValue(p->getPitchShift(), juce::dontSendNotification);
            volumeCtrl.knob.setValue(p->getVolumeLevel(), juce::dontSendNotification);
            toneCtrl.knob.setValue(p->getFormantShift(), juce::dontSendNotification);
            paceCtrl.knob.setValue(p->getPaceStretch(), juce::dontSendNotification);
            rhythmCtrl.knob.setValue(p->getRhythmQuantize(), juce::dontSendNotification);
            articulationCtrl.knob.setValue(p->getArticulation(), juce::dontSendNotification);
            resonanceCtrl.knob.setValue(p->getResonance(), juce::dontSendNotification);
            inflectionCtrl.knob.setValue(p->getInflection(), juce::dontSendNotification);
            emphasisCtrl.knob.setValue(p->getEmphasis(), juce::dontSendNotification);
            projectionCtrl.knob.setValue(p->getProjection(), juce::dontSendNotification);
            
            pitchCtrl.readout.setText(juce::String(p->getPitchShift(), 2), juce::dontSendNotification);
            volumeCtrl.readout.setText(juce::String(p->getVolumeLevel(), 2), juce::dontSendNotification);
            toneCtrl.readout.setText(juce::String(p->getFormantShift(), 2), juce::dontSendNotification);
            paceCtrl.readout.setText(juce::String(p->getPaceStretch(), 2), juce::dontSendNotification);
            rhythmCtrl.readout.setText(juce::String(p->getRhythmQuantize(), 2), juce::dontSendNotification);
            articulationCtrl.readout.setText(juce::String(p->getArticulation(), 2), juce::dontSendNotification);
            resonanceCtrl.readout.setText(juce::String(p->getResonance(), 2), juce::dontSendNotification);
            inflectionCtrl.readout.setText(juce::String(p->getInflection(), 2), juce::dontSendNotification);
            emphasisCtrl.readout.setText(juce::String(p->getEmphasis(), 2), juce::dontSendNotification);
            projectionCtrl.readout.setText(juce::String(p->getProjection(), 2), juce::dontSendNotification);
        }

        detectedPitch = p->getDetectedPitch();
        correctedPitch = p->getCorrectedPitch();

        if (detectedPitch > 0.0f && correctedPitch > 0.0f) {
            float ratio = correctedPitch / detectedPitch;
            centDeviation = 1200.0f * std::log2(ratio);
        } else {
            centDeviation = 0.0f;
        }
    } else {
        detectedPitch = 0.0f;
        correctedPitch = 0.0f;
        centDeviation = 0.0f;
    }

    repaint();
}

bool AutoTunePanel::isNoteInScale(int noteInOctave) const
{
    int scaleType = scaleCombo.getSelectedId() - 1;
    int key = keyCombo.getSelectedId() - 1;
    int adjusted = (noteInOctave - key + 12) % 12;

    if (scaleType == 0) return true; // chromatic
    if (scaleType == 1) { // major
        const int major[] = { 0,2,4,5,7,9,11 };
        for (int s : major) if (adjusted == s) return true;
        return false;
    }
    // minor
    const int minor[] = { 0,2,3,5,7,8,10 };
    for (int s : minor) if (adjusted == s) return true;
    return false;
}

void AutoTunePanel::paintPitchMeter(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    g.setColour(OrpheusLookAndFeel::bgDark());
    g.fillRoundedRectangle(bounds.toFloat(), 8.0f);
    
    // Neural Mode Glow
    if (auto* p = getProcessor()) {
        if (p->getNeuralMode()) {
            g.setColour(juce::Colour(0x3000FFFF)); // Subtle cyan glow
            g.fillRoundedRectangle(bounds.toFloat().expanded(4.0f), 12.0f);
            g.setColour(juce::Colour(0x6000FFFF));
            g.drawRoundedRectangle(bounds.toFloat(), 8.0f, 2.0f);
        } else {
            g.setColour(OrpheusLookAndFeel::borderSubtle());
            g.drawRoundedRectangle(bounds.toFloat(), 8.0f, 1.0f);
        }
    } else {
        g.setColour(OrpheusLookAndFeel::borderSubtle());
        g.drawRoundedRectangle(bounds.toFloat(), 8.0f, 1.0f);
    }

    auto center = bounds.getCentre();

    // Detected pitch text
    g.setColour(OrpheusLookAndFeel::textMuted());
    g.setFont(juce::Font(9.0f));
    g.drawText("DETECTED", bounds.getX(), bounds.getY() + 4, bounds.getWidth(), 14,
               juce::Justification::centred);

    if (detectedPitch > 0.0f) {
        // Note name from frequency
        int midiNote = (int)std::round(69.0 + 12.0 * std::log2(detectedPitch / 440.0));
        const char* noteNames[] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
        juce::String noteName = juce::String(noteNames[midiNote % 12]) + juce::String(midiNote / 12 - 1);

        g.setColour(OrpheusLookAndFeel::textPrimary());
        g.setFont(juce::Font(28.0f).boldened());
        g.drawText(noteName, bounds.withTrimmedTop(18).withTrimmedBottom(bounds.getHeight() / 2 - 10),
                   juce::Justification::centred);

        g.setFont(juce::Font(11.0f));
        g.setColour(OrpheusLookAndFeel::accentSecondary());
        g.drawText(juce::String(detectedPitch, 1) + " Hz",
                   bounds.withTrimmedTop(48).withTrimmedBottom(bounds.getHeight() / 2 - 20),
                   juce::Justification::centred);

        // Cent deviation meter
        auto meterBounds = bounds.reduced(20).withTrimmedTop(bounds.getHeight() / 2 + 10).withHeight(20);
        g.setColour(OrpheusLookAndFeel::bgElevated());
        g.fillRoundedRectangle(meterBounds.toFloat(), 4.0f);

        float deviation = juce::jlimit(-50.0f, 50.0f, centDeviation);
        float meterCenter = (float)meterBounds.getCentreX();
        float barWidth = (deviation / 50.0f) * (meterBounds.getWidth() / 2.0f);

        juce::Colour deviationCol = std::abs(deviation) < 10.0f
            ? OrpheusLookAndFeel::accentSuccess()
            : (std::abs(deviation) < 25.0f ? OrpheusLookAndFeel::accentWarning()
                                            : OrpheusLookAndFeel::accentDanger());
        g.setColour(deviationCol);
        if (barWidth > 0)
            g.fillRect(meterCenter, (float)meterBounds.getY() + 2, barWidth, (float)meterBounds.getHeight() - 4);
        else
            g.fillRect(meterCenter + barWidth, (float)meterBounds.getY() + 2, -barWidth, (float)meterBounds.getHeight() - 4);

        // Center line
        g.setColour(OrpheusLookAndFeel::textPrimary());
        g.drawVerticalLine((int)meterCenter, (float)meterBounds.getY(), (float)meterBounds.getBottom());

        // Cent readout
        g.setFont(juce::Font(10.0f));
        g.setColour(deviationCol);
        g.drawText((centDeviation >= 0 ? "+" : "") + juce::String(centDeviation, 0) + " cents",
                   meterBounds.withTrimmedTop(20), juce::Justification::centred);
    } else {
        g.setColour(OrpheusLookAndFeel::textMuted());
        g.setFont(juce::Font(16.0f));
        g.drawText("No signal", bounds.withTrimmedTop(20), juce::Justification::centred);
    }
}

void AutoTunePanel::paintMiniKeyboard(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    g.setColour(OrpheusLookAndFeel::bgDark());
    g.fillRoundedRectangle(bounds.toFloat(), 6.0f);

    static const bool isBlack[12] = { false,true,false,true,false,false,true,false,true,false,true,false };
    int whiteCount = 0;
    for (int i = 0; i < 12; ++i) if (!isBlack[i]) whiteCount++;

    float whiteW = (float)bounds.getWidth() / whiteCount;
    float keyH = (float)bounds.getHeight();

    int detectedMidi = -1;
    if (detectedPitch > 20.0f)
        detectedMidi = (int)std::round(69.0 + 12.0 * std::log2(detectedPitch / 440.0));
    int detectedNoteInOctave = detectedMidi >= 0 ? detectedMidi % 12 : -1;

    // Draw white keys
    int wIdx = 0;
    for (int note = 0; note < 12; ++note) {
        if (isBlack[note]) continue;
        float x = (float)bounds.getX() + wIdx * whiteW;
        bool inScale = isNoteInScale(note);
        bool isDetected = (note == detectedNoteInOctave);

        if (isDetected) {
            g.setColour(OrpheusLookAndFeel::accentPrimary());
        } else if (inScale) {
            g.setColour(juce::Colour(0xffddeeff));
        } else {
            g.setColour(juce::Colour(0xff888899));
        }
        g.fillRect(x + 1, (float)bounds.getY(), whiteW - 2, keyH);

        if (isDetected) {
            g.setColour(OrpheusLookAndFeel::accentPrimary().withAlpha(0.4f));
            g.fillRect(x, (float)bounds.getY(), whiteW, keyH);
        }
        wIdx++;
    }

    // Draw black keys
    wIdx = 0;
    for (int note = 0; note < 12; ++note) {
        if (isBlack[note]) {
            float x = (float)bounds.getX() + wIdx * whiteW - whiteW * 0.3f;
            bool inScale = isNoteInScale(note);
            bool isDetected = (note == detectedNoteInOctave);

            if (isDetected)
                g.setColour(OrpheusLookAndFeel::accentPrimary().darker(0.3f));
            else if (inScale)
                g.setColour(juce::Colour(0xff222233));
            else
                g.setColour(juce::Colour(0xff444455));

            g.fillRect(x, (float)bounds.getY(), whiteW * 0.6f, keyH * 0.6f);
        } else {
            wIdx++;
        }
    }
}

void AutoTunePanel::paint(juce::Graphics& g)
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
    g.drawText("VOCAL SUITE", header.reduced(16, 0), juce::Justification::centredLeft);

    auto area = getLocalBounds().reduced(16).withTrimmedTop(48);

    // Pitch meter
    auto pitchArea = area.removeFromTop(180);
    paintPitchMeter(g, pitchArea);

    area.removeFromTop(16);

    // Mini keyboard
    auto keyboardArea = area.removeFromBottom(60);
    paintMiniKeyboard(g, keyboardArea);
}

void AutoTunePanel::resized()
{
    auto area = getLocalBounds().reduced(16).withTrimmedTop(48);

    // Enable/bypass
    enableToggle.setBounds(area.getRight() - 250, 10, 70, 24);
    bypassToggle.setBounds(area.getRight() - 170, 10, 70, 24);
    neuralModeToggle.setBounds(area.getRight() - 90, 10, 100, 24);

    area.removeFromTop(188); // pitch meter
    area.removeFromBottom(68); // keyboard

    // Controls grid
    int knobSz = 64;
    int rowH = knobSz + 30;

    // Key/Scale row
    auto ksRow = area.removeFromTop(36);
    keyLabel.setBounds(ksRow.removeFromLeft(30));
    keyCombo.setBounds(ksRow.removeFromLeft(80).reduced(2));
    ksRow.removeFromLeft(12);
    scaleLabel.setBounds(ksRow.removeFromLeft(40));
    scaleCombo.setBounds(ksRow.removeFromLeft(100).reduced(2));
    area.removeFromTop(12);

    // 10 Vocal Controls Grid
    knobSz = 48;
    rowH = knobSz + 30;
    
    auto layOutKnob = [&](ParameterControl& pc, juce::Rectangle<int>& a) {
        pc.knob.setBounds(a.getCentreX() - knobSz / 2, a.getY(), knobSz, knobSz);
        pc.label.setBounds(a.getX(), a.getY() + knobSz, a.getWidth(), 12);
        pc.readout.setBounds(a.getX(), a.getY() + knobSz + 12, a.getWidth(), 14);
    };

    auto row1 = area.removeFromTop(rowH);
    area.removeFromTop(8);
    auto row2 = area.removeFromTop(rowH);

    int colW = row1.getWidth() / 5;
    
    // Row 1
    auto a1 = row1.removeFromLeft(colW); layOutKnob(pitchCtrl, a1);
    auto a2 = row1.removeFromLeft(colW); layOutKnob(volumeCtrl, a2);
    auto a3 = row1.removeFromLeft(colW); layOutKnob(toneCtrl, a3);
    auto a4 = row1.removeFromLeft(colW); layOutKnob(paceCtrl, a4);
    auto a5 = row1; layOutKnob(rhythmCtrl, a5);

    // Row 2
    colW = row2.getWidth() / 5;
    auto b1 = row2.removeFromLeft(colW); layOutKnob(articulationCtrl, b1);
    auto b2 = row2.removeFromLeft(colW); layOutKnob(resonanceCtrl, b2);
    auto b3 = row2.removeFromLeft(colW); layOutKnob(inflectionCtrl, b3);
    auto b4 = row2.removeFromLeft(colW); layOutKnob(emphasisCtrl, b4);
    auto b5 = row2; layOutKnob(projectionCtrl, b5);
}
