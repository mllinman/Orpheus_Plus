#include "VocalAutomationPanel.h"
#include "../MainComponent.h"
#include <cmath>

VocalAutomationPanel::VocalAutomationPanel(AudioEngine& e, MainComponent* mainComp)
    : audioEngine(e), mainComponent(mainComp)
{
    waveformBuffer.fill(0.0f);

    // Enable toggle
    enableToggle.setClickingTogglesState(true);
    enableToggle.onClick = [this] {
        if (auto* p = getActiveProcessor())
            p->setEnabled(enableToggle.getToggleState());
    };
    addAndMakeVisible(enableToggle);

    // Key selector
    const char* noteNames[] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
    for (int i = 0; i < 12; ++i)
        keyCombo.addItem(noteNames[i], i + 1);
    keyCombo.setSelectedId(1, juce::dontSendNotification);
    keyCombo.onChange = [this] {
        if (auto* p = getActiveProcessor())
            p->setKey(keyCombo.getSelectedId() - 1);
    };
    addAndMakeVisible(keyCombo);

    // Scale selector
    scaleCombo.addItem("Chromatic", 1);
    scaleCombo.addItem("Major", 2);
    scaleCombo.addItem("Minor", 3);
    scaleCombo.setSelectedId(2, juce::dontSendNotification);
    scaleCombo.onChange = [this] {
        if (auto* p = getActiveProcessor())
            p->setScale(scaleCombo.getSelectedId() - 1);
    };
    addAndMakeVisible(scaleCombo);

    // Setup knobs
    auto accent = OrpheusLookAndFeel::accentPrimary();
    auto cyan   = OrpheusLookAndFeel::accentSecondary();
    auto pink   = OrpheusLookAndFeel::accentTertiary();
    auto green  = OrpheusLookAndFeel::accentSuccess();
    auto amber  = OrpheusLookAndFeel::accentWarning();
    auto blue   = OrpheusLookAndFeel::accentInfo();

    setupKnob(pitchKnob,        "PITCH",        "st",  accent, -12.0, 12.0, 0.0, 0.1);
    setupKnob(volumeKnob,       "VOLUME",       "",    cyan,   0.0,   2.0,  1.0, 0.01);
    setupKnob(toneKnob,         "TONE",         "st",  pink,   -12.0, 12.0, 0.0, 0.1);
    setupKnob(paceKnob,         "PACE",         "x",   green,  0.5,   2.0,  1.0, 0.01);
    setupKnob(rhythmKnob,       "RHYTHM",       "%",   amber,  0.0,   1.0,  0.0, 0.01);
    setupKnob(articulationKnob, "ARTICULATION", "%",   blue,   0.0,   1.0,  0.0, 0.01);
    setupKnob(resonanceKnob,    "RESONANCE",    "%",   accent.brighter(0.3f), 0.0, 1.0, 0.0, 0.01);
    setupKnob(inflectionKnob,   "INFLECTION",   "%",   cyan.brighter(0.3f),   0.0, 1.0, 0.0, 0.01);
    setupKnob(emphasisKnob,     "EMPHASIS",     "%",   pink.brighter(0.3f),   0.0, 1.0, 0.0, 0.01);
    setupKnob(projectionKnob,   "PROJECTION",   "%",   green.brighter(0.3f),  0.0, 1.0, 0.0, 0.01);

    // Wire callbacks
    pitchKnob.slider.onValueChange = [this] {
        float val = (float)pitchKnob.slider.getValue();
        if (auto* p = getActiveProcessor()) p->setPitchShift(val);
        recordAutomation("pitch", val);
    };
    volumeKnob.slider.onValueChange = [this] {
        float val = (float)volumeKnob.slider.getValue();
        if (auto* p = getActiveProcessor()) p->setVolumeLevel(val);
        recordAutomation("volume", val);
    };
    toneKnob.slider.onValueChange = [this] {
        float val = (float)toneKnob.slider.getValue();
        if (auto* p = getActiveProcessor()) p->setFormantShift(val);
        recordAutomation("tone", val);
    };
    paceKnob.slider.onValueChange = [this] {
        float val = (float)paceKnob.slider.getValue();
        if (auto* p = getActiveProcessor()) p->setPaceStretch(val);
        recordAutomation("pace", val);
    };
    rhythmKnob.slider.onValueChange = [this] {
        float val = (float)rhythmKnob.slider.getValue();
        if (auto* p = getActiveProcessor()) p->setRhythmQuantize(val);
        recordAutomation("rhythm", val);
    };
    articulationKnob.slider.onValueChange = [this] {
        float val = (float)articulationKnob.slider.getValue();
        if (auto* p = getActiveProcessor()) p->setArticulation(val);
        recordAutomation("articulation", val);
    };
    resonanceKnob.slider.onValueChange = [this] {
        float val = (float)resonanceKnob.slider.getValue();
        if (auto* p = getActiveProcessor()) p->setResonance(val);
        recordAutomation("resonance", val);
    };
    inflectionKnob.slider.onValueChange = [this] {
        float val = (float)inflectionKnob.slider.getValue();
        if (auto* p = getActiveProcessor()) p->setInflection(val);
        recordAutomation("inflection", val);
    };
    emphasisKnob.slider.onValueChange = [this] {
        float val = (float)emphasisKnob.slider.getValue();
        if (auto* p = getActiveProcessor()) p->setEmphasis(val);
        recordAutomation("emphasis", val);
    };
    projectionKnob.slider.onValueChange = [this] {
        float val = (float)projectionKnob.slider.getValue();
        if (auto* p = getActiveProcessor()) p->setProjection(val);
        recordAutomation("projection", val);
    };

    // Retune speed
    retuneSpeedSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    retuneSpeedSlider.setRange(0.0, 1.0, 0.01);
    retuneSpeedSlider.setValue(0.5, juce::dontSendNotification);
    retuneSpeedSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 18);
    retuneSpeedSlider.onValueChange = [this] {
        if (auto* p = getActiveProcessor())
            p->setRetuneSpeed((float)retuneSpeedSlider.getValue());
    };
    addAndMakeVisible(retuneSpeedSlider);

    // Doubler
    doublerSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    doublerSlider.setRange(0.0, 1.0, 0.01);
    doublerSlider.setValue(0.0, juce::dontSendNotification);
    doublerSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 18);
    doublerSlider.onValueChange = [this] {
        if (auto* p = getActiveProcessor())
            p->setDoublerAmount((float)doublerSlider.getValue());
    };
    addAndMakeVisible(doublerSlider);

    // Harmony
    harmonySlider.setSliderStyle(juce::Slider::LinearHorizontal);
    harmonySlider.setRange(-12.0, 12.0, 1.0);
    harmonySlider.setValue(0.0, juce::dontSendNotification);
    harmonySlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 18);
    harmonySlider.onValueChange = [this] {
        if (auto* p = getActiveProcessor())
            p->setHarmonyInterval((int)harmonySlider.getValue());
    };
    addAndMakeVisible(harmonySlider);

    // Automation Management Buttons
    smoothBtn.setColour(juce::TextButton::buttonColourId, OrpheusLookAndFeel::bgDark());
    smoothBtn.setColour(juce::TextButton::buttonOnColourId, OrpheusLookAndFeel::accentInfo());
    smoothBtn.onClick = [this] {
        if (mainComponent) {
            int trackIndex = mainComponent->getAppState()->getSelectedTrackIndex();
            if (trackIndex >= 0) {
                // Smooth a range (0 to 1 hour) for all parameters (using an empty string as a signal or iterate)
                // We'll iterate the curves in the audio engine for this track
                auto& trackInfo = audioEngine.getTrackInfo(trackIndex);
                for (const auto& curve : trackInfo.automationCurves) {
                    audioEngine.smoothAutomationRange(trackIndex, curve.parameterID, 0.0, 3600.0);
                }
            }
        }
    };
    addAndMakeVisible(smoothBtn);

    clearBtn.setColour(juce::TextButton::buttonColourId, OrpheusLookAndFeel::bgDark());
    clearBtn.setColour(juce::TextButton::buttonOnColourId, OrpheusLookAndFeel::accentWarning());
    clearBtn.onClick = [this] {
        if (mainComponent) {
            int trackIndex = mainComponent->getAppState()->getSelectedTrackIndex();
            if (trackIndex >= 0) {
                auto& trackInfo = audioEngine.getTrackInfo(trackIndex);
                for (const auto& curve : trackInfo.automationCurves) {
                    audioEngine.deleteAutomationRange(trackIndex, curve.parameterID, 0.0, 3600.0);
                }
            }
        }
    };
    addAndMakeVisible(clearBtn);

    startTimerHz(30);
}

VocalAutomationPanel::~VocalAutomationPanel() { stopTimer(); }

void VocalAutomationPanel::recordAutomation(const juce::String& paramID, float value) {
    int trackIndex = -1;
    if (mainComponent) trackIndex = mainComponent->getAppState()->getSelectedTrackIndex();
    if (trackIndex >= 0) {
        double time = audioEngine.getPlayheadPosition();
        audioEngine.recordAutomationPoint(trackIndex, paramID, time, value);
    }
}

void VocalAutomationPanel::setupKnob(VocalKnob& vk, const juce::String& name, const juce::String& unit,
                                      juce::Colour col, double min, double max, double def, double step)
{
    vk.name = name;
    vk.unit = unit;
    vk.colour = col;
    vk.displayValue = (float)def;
    vk.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    vk.slider.setRange(min, max, step);
    vk.slider.setValue(def, juce::dontSendNotification);
    vk.slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    vk.slider.setColour(juce::Slider::rotarySliderFillColourId, col);
    vk.slider.setColour(juce::Slider::rotarySliderOutlineColourId, OrpheusLookAndFeel::bgDark());
    vk.slider.setColour(juce::Slider::thumbColourId, col.brighter(0.4f));
    addAndMakeVisible(vk.slider);
}

VocalSuiteProcessor* VocalAutomationPanel::getActiveProcessor()
{
    // Fetch the processor from the AutoTunePanel instance via MainComponent
    if (mainComponent)
    {
        if (auto* atp = mainComponent->getAutoTunePanel())
        {
            return atp->getProcessor();
        }
    }
    return nullptr;
}

void VocalAutomationPanel::timerCallback()
{
    int currentTrackIndex = -1;
    if (mainComponent && mainComponent->getAppState()) {
        currentTrackIndex = mainComponent->getAppState()->getSelectedTrackIndex();
    }

    if (auto* p = getActiveProcessor())
    {
        // Sync sliders on track change
        if (currentTrackIndex != lastTrackIndex && currentTrackIndex >= 0) {
            lastTrackIndex = currentTrackIndex;
            pitchKnob.slider.setValue(p->getPitchShift(), juce::dontSendNotification);
            volumeKnob.slider.setValue(p->getVolumeLevel(), juce::dontSendNotification);
            toneKnob.slider.setValue(p->getFormantShift(), juce::dontSendNotification);
            paceKnob.slider.setValue(p->getPaceStretch(), juce::dontSendNotification);
            rhythmKnob.slider.setValue(p->getRhythmQuantize(), juce::dontSendNotification);
            articulationKnob.slider.setValue(p->getArticulation(), juce::dontSendNotification);
            resonanceKnob.slider.setValue(p->getResonance(), juce::dontSendNotification);
            inflectionKnob.slider.setValue(p->getInflection(), juce::dontSendNotification);
            emphasisKnob.slider.setValue(p->getEmphasis(), juce::dontSendNotification);
            projectionKnob.slider.setValue(p->getProjection(), juce::dontSendNotification);
        }

        detectedPitch = p->getDetectedPitch();
        correctedPitch = p->getCorrectedPitch();
    }

    // Update waveform ring buffer with current pitch as visualization
    float currentPitch = detectedPitch.getCurrentValue();
    waveformBuffer[waveformWritePos % 128] = currentPitch > 0.0f
        ? std::sin(currentPitch * 0.02f + (float)waveformWritePos * 0.15f) * 0.5f
        : 0.0f;
    waveformWritePos++;

    repaint();
}

void VocalAutomationPanel::paintGlassmorphicCard(juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& title)
{
    // Glassmorphic card background
    g.setColour(juce::Colour(0x18ffffff));
    g.fillRoundedRectangle(bounds.toFloat(), 12.0f);

    // Border glow
    g.setColour(juce::Colour(0x30ffffff));
    g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 12.0f, 1.0f);

    // Title
    if (title.isNotEmpty())
    {
        g.setColour(OrpheusLookAndFeel::textMuted());
        g.setFont(juce::Font(10.0f).boldened());
        g.drawText(title, bounds.getX() + 12, bounds.getY() + 6, bounds.getWidth() - 24, 14,
                   juce::Justification::centredLeft);
    }
}

void VocalAutomationPanel::paintVocalWaveform(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    paintGlassmorphicCard(g, bounds, "LIVE VOCAL WAVEFORM");

    auto waveArea = bounds.reduced(12, 24).withTrimmedTop(6);
    float midY = waveArea.getCentreY();
    float scaleY = waveArea.getHeight() * 0.4f;

    juce::Path waveform;
    bool first = true;
    for (int i = 0; i < 128; ++i)
    {
        int idx = (waveformWritePos + i) % 128;
        float x = waveArea.getX() + (float)i / 127.0f * waveArea.getWidth();
        float y = midY - waveformBuffer[idx] * scaleY;

        if (first)
        {
            waveform.startNewSubPath(x, y);
            first = false;
        }
        else
            waveform.lineTo(x, y);
    }

    // Gradient stroke
    g.setGradientFill(juce::ColourGradient(
        OrpheusLookAndFeel::accentPrimary(), waveArea.getX(), midY,
        OrpheusLookAndFeel::accentSecondary(), waveArea.getRight(), midY, false));
    g.strokePath(waveform, juce::PathStrokeType(2.0f));

    // Center line
    g.setColour(OrpheusLookAndFeel::borderSubtle());
    g.drawHorizontalLine((int)midY, waveArea.getX(), waveArea.getRight());

    // Pitch readout
    g.setColour(OrpheusLookAndFeel::textPrimary());
    g.setFont(juce::Font(14.0f).boldened());
    float currentPitch = detectedPitch.getCurrentValue();
    if (currentPitch > 20.0f)
    {
        int midiNote = (int)std::round(69.0 + 12.0 * std::log2(currentPitch / 440.0));
        const char* noteNames[] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
        juce::String noteName = juce::String(noteNames[midiNote % 12]) + juce::String(midiNote / 12 - 1);
        g.drawText(noteName + "  " + juce::String(currentPitch, 1) + " Hz",
                   waveArea.removeFromRight(160).withTrimmedTop(4),
                   juce::Justification::centredRight);
    }
    else
    {
        g.setColour(OrpheusLookAndFeel::textMuted());
        g.drawText("No signal", waveArea.removeFromRight(120).withTrimmedTop(4),
                   juce::Justification::centredRight);
    }
}

void VocalAutomationPanel::paintParameterArc(juce::Graphics& g, juce::Rectangle<int> bounds,
                                              float value, float minVal, float maxVal,
                                              juce::Colour col, const juce::String& label,
                                              const juce::String& readout)
{
    auto center = bounds.getCentre().toFloat();
    float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.35f;

    float startAngle = juce::MathConstants<float>::pi * 0.75f;
    float endAngle   = juce::MathConstants<float>::pi * 2.25f;

    // Background arc
    juce::Path bgArc;
    bgArc.addCentredArc(center.x, center.y - 4, radius, radius, 0.0f, startAngle, endAngle, true);
    g.setColour(OrpheusLookAndFeel::bgDark());
    g.strokePath(bgArc, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Value arc
    float normVal = juce::jlimit(0.0f, 1.0f, (value - minVal) / (maxVal - minVal));
    float valAngle = startAngle + normVal * (endAngle - startAngle);
    juce::Path valArc;
    valArc.addCentredArc(center.x, center.y - 4, radius, radius, 0.0f, startAngle, valAngle, true);

    juce::ColourGradient arcGrad(col.withAlpha(0.6f), center.x - radius, center.y,
                                  col, center.x + radius, center.y, false);
    g.setGradientFill(arcGrad);
    g.strokePath(valArc, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Glow dot at end
    float dotX = center.x + radius * std::cos(valAngle);
    float dotY = (center.y - 4) + radius * std::sin(valAngle);
    g.setColour(col.withAlpha(0.3f));
    g.fillEllipse(dotX - 6, dotY - 6, 12, 12);
    g.setColour(col);
    g.fillEllipse(dotX - 3, dotY - 3, 6, 6);

    // Label
    g.setColour(OrpheusLookAndFeel::textMuted());
    g.setFont(juce::Font(9.0f).boldened());
    g.drawText(label, bounds.getX(), bounds.getBottom() - 28, bounds.getWidth(), 12,
               juce::Justification::centred);

    // Readout
    g.setColour(col);
    g.setFont(juce::Font(10.0f));
    g.drawText(readout, bounds.getX(), bounds.getBottom() - 16, bounds.getWidth(), 14,
               juce::Justification::centred);
}

void VocalAutomationPanel::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(OrpheusLookAndFeel::bgPanel());

    // Header gradient
    auto header = getLocalBounds().removeFromTop(44);
    g.setGradientFill(juce::ColourGradient(
        OrpheusLookAndFeel::accentPrimary().withAlpha(0.15f), 0, 0,
        OrpheusLookAndFeel::bgPanel(), 0, 44.0f, false));
    g.fillRect(header);

    g.setColour(OrpheusLookAndFeel::textPrimary());
    g.setFont(juce::Font(15.0f).boldened());
    g.drawText(juce::CharPointer_UTF8("\xe2\x99\xab  VOCAL AUTOMATION"), header.reduced(16, 0),
               juce::Justification::centredLeft);

    // Separator
    g.setColour(OrpheusLookAndFeel::borderSubtle());
    g.drawHorizontalLine(44, 0, getWidth());

    auto area = getLocalBounds().withTrimmedTop(48).reduced(12);

    // Waveform card
    auto waveArea = area.removeFromTop(90);
    paintVocalWaveform(g, waveArea);

    area.removeFromTop(10);

    // Knobs section: "Primary Controls" card
    auto primaryArea = area.removeFromTop(110);
    paintGlassmorphicCard(g, primaryArea, "PRIMARY CONTROLS");

    auto pKnobs = primaryArea.reduced(8, 22).withTrimmedTop(4);
    int colW = pKnobs.getWidth() / 5;
    int knobH = pKnobs.getHeight();

    auto drawKnob = [&](VocalKnob& vk, int col) {
        auto r = juce::Rectangle<int>(pKnobs.getX() + col * colW, pKnobs.getY(), colW, knobH);
        paintParameterArc(g, r, (float)vk.slider.getValue(),
                          (float)vk.slider.getMinimum(), (float)vk.slider.getMaximum(),
                          vk.colour, vk.name,
                          juce::String(vk.slider.getValue(), 2) + vk.unit);
    };

    drawKnob(pitchKnob, 0);
    drawKnob(volumeKnob, 1);
    drawKnob(toneKnob, 2);
    drawKnob(paceKnob, 3);
    drawKnob(rhythmKnob, 4);

    area.removeFromTop(10);

    // "Expression Controls" card
    auto expressionArea = area.removeFromTop(110);
    paintGlassmorphicCard(g, expressionArea, "EXPRESSION CONTROLS");

    auto eKnobs = expressionArea.reduced(8, 22).withTrimmedTop(4);
    colW = eKnobs.getWidth() / 5;
    knobH = eKnobs.getHeight();

    auto drawKnob2 = [&](VocalKnob& vk, int col) {
        auto r = juce::Rectangle<int>(eKnobs.getX() + col * colW, eKnobs.getY(), colW, knobH);
        paintParameterArc(g, r, (float)vk.slider.getValue(),
                          (float)vk.slider.getMinimum(), (float)vk.slider.getMaximum(),
                          vk.colour, vk.name,
                          juce::String(vk.slider.getValue(), 2) + vk.unit);
    };

    drawKnob2(articulationKnob, 0);
    drawKnob2(resonanceKnob, 1);
    drawKnob2(inflectionKnob, 2);
    drawKnob2(emphasisKnob, 3);
    drawKnob2(projectionKnob, 4);

    area.removeFromTop(10);

    // "FX Chain" card (retune, doubler, harmony)
    auto fxArea = area.removeFromTop(100);
    paintGlassmorphicCard(g, fxArea, "FX CHAIN");

    // Labels for sliders
    auto fxInner = fxArea.reduced(12, 24).withTrimmedTop(4);
    int rowH = fxInner.getHeight() / 3;
    g.setColour(OrpheusLookAndFeel::textMuted());
    g.setFont(juce::Font(9.0f).boldened());
    g.drawText("RETUNE SPEED", fxInner.getX(), fxInner.getY(), 90, rowH, juce::Justification::centredLeft);
    g.drawText("DOUBLER",      fxInner.getX(), fxInner.getY() + rowH, 90, rowH, juce::Justification::centredLeft);
    g.drawText("HARMONY",      fxInner.getX(), fxInner.getY() + rowH * 2, 90, rowH, juce::Justification::centredLeft);
}

void VocalAutomationPanel::resized()
{
    auto area = getLocalBounds().withTrimmedTop(48).reduced(12);

    // Waveform
    area.removeFromTop(90);
    area.removeFromTop(10);

    // Enable toggle
    enableToggle.setBounds(getWidth() - 100, 12, 80, 24);

    // Key/Scale in header
    keyCombo.setBounds(getWidth() - 280, 12, 70, 24);
    scaleCombo.setBounds(getWidth() - 200, 12, 90, 24);

    // Primary controls (knobs overlap painted arcs)
    auto primaryArea = area.removeFromTop(110).reduced(8, 22).withTrimmedTop(4);
    int colW = primaryArea.getWidth() / 5;
    int knobSz = juce::jmin(colW - 8, primaryArea.getHeight() - 30);

    auto placeKnob = [&](VocalKnob& vk, int col) {
        int cx = primaryArea.getX() + col * colW + colW / 2;
        int cy = primaryArea.getY() + (primaryArea.getHeight() - 28) / 2;
        vk.slider.setBounds(cx - knobSz / 2, cy - knobSz / 2, knobSz, knobSz);
    };

    placeKnob(pitchKnob, 0);
    placeKnob(volumeKnob, 1);
    placeKnob(toneKnob, 2);
    placeKnob(paceKnob, 3);
    placeKnob(rhythmKnob, 4);

    area.removeFromTop(10);

    // Expression controls
    auto expressionArea = area.removeFromTop(110).reduced(8, 22).withTrimmedTop(4);
    colW = expressionArea.getWidth() / 5;
    knobSz = juce::jmin(colW - 8, expressionArea.getHeight() - 30);

    auto placeKnob2 = [&](VocalKnob& vk, int col) {
        int cx = expressionArea.getX() + col * colW + colW / 2;
        int cy = expressionArea.getY() + (expressionArea.getHeight() - 28) / 2;
        vk.slider.setBounds(cx - knobSz / 2, cy - knobSz / 2, knobSz, knobSz);
    };

    placeKnob2(articulationKnob, 0);
    placeKnob2(resonanceKnob, 1);
    placeKnob2(inflectionKnob, 2);
    placeKnob2(emphasisKnob, 3);
    placeKnob2(projectionKnob, 4);

    area.removeFromTop(10);

    // FX Chain sliders and buttons
    auto fxArea = area.removeFromTop(100).reduced(12, 24).withTrimmedTop(4);
    int rowH = fxArea.getHeight() / 3;
    int sliderX = fxArea.getX() + 95;
    int sliderW = fxArea.getWidth() - 95;

    retuneSpeedSlider.setBounds(sliderX, fxArea.getY(), sliderW, rowH);
    doublerSlider.setBounds(sliderX, fxArea.getY() + rowH, sliderW, rowH);
    harmonySlider.setBounds(sliderX, fxArea.getY() + rowH * 2, sliderW, rowH);

    // Automation Management Buttons
    // Place them in the bottom area
    auto btnArea = getLocalBounds().removeFromBottom(40).reduced(12, 8);
    int btnW = btnArea.getWidth() / 2 - 4;
    smoothBtn.setBounds(btnArea.getX(), btnArea.getY(), btnW, btnArea.getHeight());
    clearBtn.setBounds(btnArea.getX() + btnW + 8, btnArea.getY(), btnW, btnArea.getHeight());
}
