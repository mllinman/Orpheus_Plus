#include "AutoTunePanel.h"
#include <cmath>

AutoTunePanel::AutoTunePanel(AudioEngine& e)
    : audioEngine(e)
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
    enableToggle.onClick = [this] { processor.setEnabled(enableToggle.getToggleState()); };
    bypassToggle.setClickingTogglesState(true);
    addAndMakeVisible(enableToggle);
    addAndMakeVisible(bypassToggle);

    // Key selector
    const char* noteNames[] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
    for (int i = 0; i < 12; ++i)
        keyCombo.addItem(noteNames[i], i + 1);
    keyCombo.setSelectedId(1, juce::dontSendNotification);
    keyCombo.onChange = [this] { processor.setKey(keyCombo.getSelectedId() - 1); repaint(); };
    setupLabel(keyLabel);
    addAndMakeVisible(keyLabel);
    addAndMakeVisible(keyCombo);

    // Scale selector
    scaleCombo.addItem("Chromatic", 1);
    scaleCombo.addItem("Major", 2);
    scaleCombo.addItem("Minor", 3);
    scaleCombo.setSelectedId(2, juce::dontSendNotification);
    scaleCombo.onChange = [this] { processor.setScale(scaleCombo.getSelectedId() - 1); repaint(); };
    setupLabel(scaleLabel);
    addAndMakeVisible(scaleLabel);
    addAndMakeVisible(scaleCombo);

    // Speed knob
    setupKnob(speedKnob, 0.0, 1.0, 0.5, 0.01);
    speedKnob.onValueChange = [this] {
        processor.setSpeed((float)speedKnob.getValue());
        int pct = (int)(speedKnob.getValue() * 100);
        speedReadout.setText(juce::String(pct) + "%", juce::dontSendNotification);
    };
    setupLabel(speedLabel);
    setupReadout(speedReadout);
    speedReadout.setText("50%", juce::dontSendNotification);
    addAndMakeVisible(speedKnob);
    addAndMakeVisible(speedLabel);
    addAndMakeVisible(speedReadout);

    // Formant knob
    setupKnob(formantKnob, -12.0, 12.0, 0.0, 0.1);
    formantKnob.onValueChange = [this] {
        processor.setFormantShift((float)formantKnob.getValue());
        formantReadout.setText(juce::String(formantKnob.getValue(), 1) + " st", juce::dontSendNotification);
    };
    setupLabel(formantLabel);
    setupReadout(formantReadout);
    formantReadout.setText("0.0 st", juce::dontSendNotification);
    addAndMakeVisible(formantKnob);
    addAndMakeVisible(formantLabel);
    addAndMakeVisible(formantReadout);

    // Robot knob
    setupKnob(robotKnob, 0.0, 1.0, 0.0, 0.01);
    robotKnob.onValueChange = [this] {
        processor.setRobotVoiceAmount((float)robotKnob.getValue());
        int pct = (int)(robotKnob.getValue() * 100);
        robotReadout.setText(juce::String(pct) + "%", juce::dontSendNotification);
    };
    setupLabel(robotLabel);
    setupReadout(robotReadout);
    robotReadout.setText("0%", juce::dontSendNotification);
    addAndMakeVisible(robotKnob);
    addAndMakeVisible(robotLabel);
    addAndMakeVisible(robotReadout);

    startTimerHz(20);
}

AutoTunePanel::~AutoTunePanel() { stopTimer(); }

void AutoTunePanel::timerCallback()
{
    detectedPitch = processor.getDetectedPitch();
    correctedPitch = processor.getCorrectedPitch();

    if (detectedPitch > 0.0f && correctedPitch > 0.0f) {
        float ratio = correctedPitch / detectedPitch;
        centDeviation = 1200.0f * std::log2(ratio);
    } else {
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
    g.setColour(OrpheusLookAndFeel::borderSubtle());
    g.drawRoundedRectangle(bounds.toFloat(), 8.0f, 1.0f);

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
    g.drawText("AUTOTUNE", header.reduced(16, 0), juce::Justification::centredLeft);

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
    enableToggle.setBounds(area.getRight() - 160, 10, 70, 24);
    bypassToggle.setBounds(area.getRight() - 80, 10, 70, 24);

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

    // Speed / Formant / Robot row
    auto knobRow = area.removeFromTop(rowH);
    int colW = knobRow.getWidth() / 3;

    auto speedArea = knobRow.removeFromLeft(colW);
    speedKnob.setBounds(speedArea.getCentreX() - knobSz / 2, speedArea.getY(), knobSz, knobSz);
    speedLabel.setBounds(speedArea.getX(), speedArea.getY() + knobSz, colW, 12);
    speedReadout.setBounds(speedArea.getX(), speedArea.getY() + knobSz + 12, colW, 14);

    auto formantArea = knobRow.removeFromLeft(colW);
    formantKnob.setBounds(formantArea.getCentreX() - knobSz / 2, formantArea.getY(), knobSz, knobSz);
    formantLabel.setBounds(formantArea.getX(), formantArea.getY() + knobSz, colW, 12);
    formantReadout.setBounds(formantArea.getX(), formantArea.getY() + knobSz + 12, colW, 14);

    auto robotArea = knobRow;
    robotKnob.setBounds(robotArea.getCentreX() - knobSz / 2, robotArea.getY(), knobSz, knobSz);
    robotLabel.setBounds(robotArea.getX(), robotArea.getY() + knobSz, robotArea.getWidth(), 12);
    robotReadout.setBounds(robotArea.getX(), robotArea.getY() + knobSz + 12, robotArea.getWidth(), 14);
}
