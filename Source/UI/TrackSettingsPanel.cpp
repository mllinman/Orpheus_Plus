#include "TrackSettingsPanel.h"

TrackSettingsPanel::TrackSettingsPanel(AudioEngine& e, AppState& s)
    : audioEngine(e), appState(s)
{
    // ── Scrollable content area ──
    viewport.setViewedComponent(&contentArea, false);
    viewport.setScrollBarsShown(true, false);
    addAndMakeVisible(viewport);

    // ── Track Info ──
    trackNameLabel.setText("No Track Selected", juce::dontSendNotification);
    trackNameLabel.setFont(juce::Font(16.0f).boldened());
    trackNameLabel.setColour(juce::Label::textColourId, OrpheusLookAndFeel::textPrimary());
    contentArea.addAndMakeVisible(trackNameLabel);

    trackNameEditor.setFont(juce::Font(14.0f));
    trackNameEditor.onReturnKey = [this] {
        if (currentTrack >= 0) {
            auto& tracks = appState.getTracks();
            if (currentTrack < (int)tracks.size()) {
                tracks[currentTrack].name = trackNameEditor.getText();
                appState.sendChangeMessage();
            }
        }
    };
    contentArea.addAndMakeVisible(trackNameEditor);

    trackTypeLabel.setFont(juce::Font(11.0f));
    trackTypeLabel.setColour(juce::Label::textColourId, OrpheusLookAndFeel::textMuted());
    contentArea.addAndMakeVisible(trackTypeLabel);

    // ── M/S/R Buttons ──
    for (auto* btn : { &muteBtn, &soloBtn, &armBtn }) {
        btn->setClickingTogglesState(true);
        contentArea.addAndMakeVisible(btn);
    }

    // ── Volume Knob ──
    volumeKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    volumeKnob.setRange(-60.0, 6.0, 0.1);
    volumeKnob.setValue(0.0, juce::dontSendNotification);
    volumeKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    volumeKnob.onValueChange = [this] {
        if (currentTrack >= 0)
            audioEngine.setTrackVolume(currentTrack, (float)juce::Decibels::decibelsToGain(volumeKnob.getValue()));
        volumeReadout.setText(juce::String(volumeKnob.getValue(), 1) + " dB", juce::dontSendNotification);
    };
    volumeLabel.setFont(juce::Font(9.0f).boldened());
    volumeLabel.setColour(juce::Label::textColourId, OrpheusLookAndFeel::textMuted());
    volumeLabel.setJustificationType(juce::Justification::centred);
    volumeReadout.setFont(juce::Font(10.0f));
    volumeReadout.setColour(juce::Label::textColourId, OrpheusLookAndFeel::accentSecondary());
    volumeReadout.setJustificationType(juce::Justification::centred);
    volumeReadout.setText("0.0 dB", juce::dontSendNotification);
    contentArea.addAndMakeVisible(volumeKnob);
    contentArea.addAndMakeVisible(volumeLabel);
    contentArea.addAndMakeVisible(volumeReadout);

    // ── Pan Knob ──
    panKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    panKnob.setRange(-1.0, 1.0, 0.01);
    panKnob.setValue(0.0, juce::dontSendNotification);
    panKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    panKnob.onValueChange = [this] {
        if (currentTrack >= 0)
            audioEngine.setTrackPan(currentTrack, (float)panKnob.getValue());
        float v = (float)panKnob.getValue();
        juce::String panText = v < -0.01f ? juce::String((int)(-v * 100)) + "L"
                             : v > 0.01f  ? juce::String((int)(v * 100)) + "R"
                             : "C";
        panReadout.setText(panText, juce::dontSendNotification);
    };
    panLabel.setFont(juce::Font(9.0f).boldened());
    panLabel.setColour(juce::Label::textColourId, OrpheusLookAndFeel::textMuted());
    panLabel.setJustificationType(juce::Justification::centred);
    panReadout.setFont(juce::Font(10.0f));
    panReadout.setColour(juce::Label::textColourId, OrpheusLookAndFeel::accentSecondary());
    panReadout.setJustificationType(juce::Justification::centred);
    panReadout.setText("C", juce::dontSendNotification);
    contentArea.addAndMakeVisible(panKnob);
    contentArea.addAndMakeVisible(panLabel);
    contentArea.addAndMakeVisible(panReadout);

    // ── I/O Routing ──
    auto styleLbl = [](juce::Label& lbl) {
        lbl.setFont(juce::Font(9.0f).boldened());
        lbl.setColour(juce::Label::textColourId, OrpheusLookAndFeel::textMuted());
    };
    styleLbl(inputLabel); styleLbl(outputLabel);
    inputCombo.addItem("Default Input", 1);
    inputCombo.addItem("None", 2);
    inputCombo.setSelectedId(1, juce::dontSendNotification);
    outputCombo.addItem("Master", 1);
    outputCombo.addItem("Bus 1", 2);
    outputCombo.addItem("Bus 2", 3);
    outputCombo.setSelectedId(1, juce::dontSendNotification);
    contentArea.addAndMakeVisible(inputLabel);
    contentArea.addAndMakeVisible(inputCombo);
    contentArea.addAndMakeVisible(outputLabel);
    contentArea.addAndMakeVisible(outputCombo);

    // ── Plugin Insert Slots ──
    insertsLabel.setFont(juce::Font(9.0f).boldened());
    insertsLabel.setColour(juce::Label::textColourId, OrpheusLookAndFeel::textMuted());
    contentArea.addAndMakeVisible(insertsLabel);
    for (int i = 0; i < NUM_INSERT_SLOTS; ++i) {
        insertSlots[i].setButtonText("Empty");
        insertSlots[i].setColour(juce::TextButton::buttonColourId, OrpheusLookAndFeel::bgDark());
        insertSlots[i].setColour(juce::TextButton::textColourOffId, OrpheusLookAndFeel::textMuted());
        contentArea.addAndMakeVisible(insertSlots[i]);
        insertBypasses[i].setButtonText("B");
        insertBypasses[i].setClickingTogglesState(true);
        contentArea.addAndMakeVisible(insertBypasses[i]);
    }

    // ── Sends ──
    sendsLabel.setFont(juce::Font(9.0f).boldened());
    sendsLabel.setColour(juce::Label::textColourId, OrpheusLookAndFeel::textMuted());
    contentArea.addAndMakeVisible(sendsLabel);
    for (int i = 0; i < NUM_SENDS; ++i) {
        sendLabels[i].setText("Send " + juce::String(i + 1), juce::dontSendNotification);
        sendLabels[i].setFont(juce::Font(9.0f));
        sendLabels[i].setColour(juce::Label::textColourId, OrpheusLookAndFeel::textMuted());
        sendLevelKnobs[i].setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        sendLevelKnobs[i].setRange(-60.0, 6.0, 0.1);
        sendLevelKnobs[i].setValue(-60.0, juce::dontSendNotification);
        sendLevelKnobs[i].setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        sendDestCombos[i].addItem("None", 1);
        sendDestCombos[i].addItem("Bus 1", 2);
        sendDestCombos[i].addItem("Bus 2", 3);
        sendDestCombos[i].setSelectedId(1, juce::dontSendNotification);
        contentArea.addAndMakeVisible(sendLabels[i]);
        contentArea.addAndMakeVisible(sendLevelKnobs[i]);
        contentArea.addAndMakeVisible(sendDestCombos[i]);
    }

    // ── Inline EQ ──
    eqLabel.setFont(juce::Font(9.0f).boldened());
    eqLabel.setColour(juce::Label::textColourId, OrpheusLookAndFeel::textMuted());
    eqEnable.setClickingTogglesState(true);
    contentArea.addAndMakeVisible(eqLabel);
    contentArea.addAndMakeVisible(eqEnable);
    const float defaultFreqs[4] = { 80.0f, 500.0f, 3000.0f, 10000.0f };
    const char* bandNames[4] = { "LOW", "LO-MID", "HI-MID", "HIGH" };
    for (int i = 0; i < NUM_EQ_BANDS; ++i) {
        eqBands[i].freq.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        eqBands[i].freq.setRange(20.0, 20000.0, 1.0);
        eqBands[i].freq.setSkewFactorFromMidPoint(1000.0);
        eqBands[i].freq.setValue(defaultFreqs[i], juce::dontSendNotification);
        eqBands[i].freq.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);

        eqBands[i].gain.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        eqBands[i].gain.setRange(-18.0, 18.0, 0.1);
        eqBands[i].gain.setValue(0.0, juce::dontSendNotification);
        eqBands[i].gain.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);

        eqBands[i].q.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        eqBands[i].q.setRange(0.1, 10.0, 0.01);
        eqBands[i].q.setValue(0.707, juce::dontSendNotification);
        eqBands[i].q.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);

        eqBands[i].label.setText(bandNames[i], juce::dontSendNotification);
        eqBands[i].label.setFont(juce::Font(8.0f).boldened());
        eqBands[i].label.setColour(juce::Label::textColourId, OrpheusLookAndFeel::textMuted());
        eqBands[i].label.setJustificationType(juce::Justification::centred);

        contentArea.addAndMakeVisible(eqBands[i].freq);
        contentArea.addAndMakeVisible(eqBands[i].gain);
        contentArea.addAndMakeVisible(eqBands[i].q);
        contentArea.addAndMakeVisible(eqBands[i].label);
    }

    // ── Quick Access ──
    autoTuneToggle.setClickingTogglesState(true);
    cleanupToggle.setClickingTogglesState(true);
    contentArea.addAndMakeVisible(autoTuneToggle);
    contentArea.addAndMakeVisible(cleanupToggle);

    startTimerHz(10);
}

TrackSettingsPanel::~TrackSettingsPanel() { stopTimer(); }

void TrackSettingsPanel::setTrackIndex(int index)
{
    currentTrack = index;
    refreshFromTrack();
}

void TrackSettingsPanel::refreshFromTrack()
{
    if (currentTrack < 0 || currentTrack >= (int)appState.getTracks().size()) {
        trackNameLabel.setText("No Track Selected", juce::dontSendNotification);
        trackTypeLabel.setText("", juce::dontSendNotification);
        trackNameEditor.setText("", juce::dontSendNotification);
        return;
    }

    auto& track = appState.getTracks()[(size_t)currentTrack];
    trackNameLabel.setText(track.name, juce::dontSendNotification);
    trackNameEditor.setText(track.name, juce::dontSendNotification);

    bool isAudio = (track.type == AppState::TrackInfo::Type::Audio);
    trackTypeLabel.setText(isAudio ? "AUDIO TRACK" : "MIDI TRACK", juce::dontSendNotification);

    if (isAudio)
        insertsLabel.setText("INSERTS", juce::dontSendNotification);
    else
        insertsLabel.setText("INSTRUMENTS & FX", juce::dontSendNotification);
}

void TrackSettingsPanel::timerCallback()
{
    if (currentTrack >= 0 && currentTrack < (int)appState.getTracks().size())
        repaint();
}

void TrackSettingsPanel::paintSection(juce::Graphics& g, juce::Rectangle<int> bounds,
                                      const juce::String& title)
{
    g.setColour(OrpheusLookAndFeel::borderSubtle());
    g.drawHorizontalLine(bounds.getY(), (float)bounds.getX() + 8, (float)bounds.getRight() - 8);

    g.setColour(OrpheusLookAndFeel::textMuted());
    g.setFont(juce::Font(9.0f).boldened());
    g.drawText(title, bounds.getX() + 8, bounds.getY() + 2, bounds.getWidth() - 16, 14,
               juce::Justification::centredLeft);
}

void TrackSettingsPanel::paint(juce::Graphics& g)
{
    g.fillAll(OrpheusLookAndFeel::bgPanel());

    // Header gradient
    auto header = getLocalBounds().removeFromTop(36);
    g.setGradientFill(juce::ColourGradient(
        OrpheusLookAndFeel::bgSurface(), 0, 0,
        OrpheusLookAndFeel::bgPanel(), 0, 36.0f, false));
    g.fillRect(header);
    g.setColour(OrpheusLookAndFeel::borderSubtle());
    g.drawHorizontalLine(35, 0.0f, (float)getWidth());

    g.setColour(OrpheusLookAndFeel::textSecondary());
    g.setFont(juce::Font(11.0f).boldened());
    g.drawText("TRACK SETTINGS", header.reduced(12, 0), juce::Justification::centredLeft);
}

void TrackSettingsPanel::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop(36); // header
    viewport.setBounds(area);

    int w = area.getWidth() - 12; // scrollbar room
    int y = 8;
    int knobSize = 56;

    // ── Track Name ──
    trackNameLabel.setBounds(8, y, w, 20);
    y += 22;
    trackTypeLabel.setBounds(8, y, w / 2, 14);
    y += 18;
    trackNameEditor.setBounds(8, y, w, 24);
    y += 30;

    // ── M/S/R ──
    int msrW = 32;
    muteBtn.setBounds(8, y, msrW, 24);
    soloBtn.setBounds(8 + msrW + 4, y, msrW, 24);
    armBtn.setBounds(8 + (msrW + 4) * 2, y, msrW, 24);
    y += 32;

    // ── Volume / Pan ──
    int halfW = (w - 8) / 2;
    volumeKnob.setBounds(8, y, knobSize, knobSize);
    volumeLabel.setBounds(8, y + knobSize, knobSize, 12);
    volumeReadout.setBounds(8, y + knobSize + 12, knobSize, 14);

    panKnob.setBounds(8 + halfW, y, knobSize, knobSize);
    panLabel.setBounds(8 + halfW, y + knobSize, knobSize, 12);
    panReadout.setBounds(8 + halfW, y + knobSize + 12, knobSize, 14);
    y += knobSize + 30;

    // ── I/O Routing ──
    inputLabel.setBounds(8, y, w, 12); y += 14;
    inputCombo.setBounds(8, y, w, 24); y += 28;
    outputLabel.setBounds(8, y, w, 12); y += 14;
    outputCombo.setBounds(8, y, w, 24); y += 32;

    // ── Inserts ──
    insertsLabel.setBounds(8, y, w, 14); y += 16;
    for (int i = 0; i < NUM_INSERT_SLOTS; ++i) {
        insertBypasses[i].setBounds(8, y, 22, 22);
        insertSlots[i].setBounds(34, y, w - 30, 22);
        y += 24;
    }
    y += 8;

    // ── Sends ──
    sendsLabel.setBounds(8, y, w, 14); y += 16;
    for (int i = 0; i < NUM_SENDS; ++i) {
        sendLabels[i].setBounds(8, y, 60, 14);
        sendLevelKnobs[i].setBounds(8, y + 14, 36, 36);
        sendDestCombos[i].setBounds(48, y + 18, w - 52, 22);
        y += 54;
    }
    y += 4;

    // ── EQ ──
    eqLabel.setBounds(8, y, 30, 14);
    eqEnable.setBounds(42, y, 40, 20);
    y += 22;
    int bandW = (w - 8) / NUM_EQ_BANDS;
    int smallKnob = 32;
    for (int i = 0; i < NUM_EQ_BANDS; ++i) {
        int bx = 8 + i * bandW;
        eqBands[i].label.setBounds(bx, y, bandW, 12);
        eqBands[i].freq.setBounds(bx + (bandW - smallKnob) / 2, y + 12, smallKnob, smallKnob);
        eqBands[i].gain.setBounds(bx + (bandW - smallKnob) / 2, y + 12 + smallKnob, smallKnob, smallKnob);
        eqBands[i].q.setBounds(bx + (bandW - smallKnob) / 2, y + 12 + smallKnob * 2, smallKnob, smallKnob);
    }
    y += 12 + smallKnob * 3 + 8;

    // ── Quick Access ──
    autoTuneToggle.setBounds(8, y, w / 2 - 4, 28);
    cleanupToggle.setBounds(8 + w / 2, y, w / 2 - 4, 28);
    y += 36;

    contentArea.setSize(w + 12, y + 20);
}
