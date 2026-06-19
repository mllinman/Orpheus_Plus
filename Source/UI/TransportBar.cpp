#include <JuceHeader.h>
#include "TransportBar.h"
#include "OrpheusLookAndFeel.h"

TransportBar::TransportBar(AudioEngine& e, juce::ApplicationCommandManager& c)
    : audioEngine(e), commandManager(c)
{
    audioEngine.addListener(this);

    auto styleBtn = [](juce::TextButton& btn, juce::Colour col)
    {
        btn.setColour(juce::TextButton::buttonColourId, col);
        btn.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    };

    styleBtn(rewindButton,  OrpheusLookAndFeel::bgElevated());
    styleBtn(playButton,    juce::Colour(0xff1b5e20).brighter(0.15f));
    styleBtn(stopButton,    OrpheusLookAndFeel::bgElevated());
    styleBtn(recordButton,  OrpheusLookAndFeel::accentDanger().darker(0.3f));
    styleBtn(loopButton,    OrpheusLookAndFeel::bgElevated());
    styleBtn(settingsButton,OrpheusLookAndFeel::bgElevated());

    // Use Unicode Icons for Transport
    rewindButton.setButtonText(juce::CharPointer_UTF8("\xe2\x8f\xaa")); // ⏪
    playButton.setButtonText(juce::CharPointer_UTF8("\xe2\x96\xb6"));   // ▶
    stopButton.setButtonText(juce::CharPointer_UTF8("\xe2\x96\xa0"));   // ■
    recordButton.setButtonText(juce::CharPointer_UTF8("\xe2\x8f\xba")); // ⏺
    loopButton.setButtonText(juce::CharPointer_UTF8("\xe2\x86\xba"));   // ↺
    settingsButton.setButtonText(juce::CharPointer_UTF8("\xe2\x9a\x99"));// ⚙

    rewindButton.onClick = [this] { audioEngine.stop(); };
    playButton.onClick   = [this] { audioEngine.togglePlayback(); };
    stopButton.onClick   = [this] { audioEngine.stop(); };
    recordButton.onClick = [this] { audioEngine.toggleRecord(); };
    loopButton.setToggleable(true);
    loopButton.onClick = [this] { audioEngine.setLooping(loopButton.getToggleState()); };
    settingsButton.setTooltip("Audio/MIDI Settings");
    
    // Command ID 5 is cmdOpenSettings from MainComponent
    settingsButton.onClick = [this] { commandManager.invokeDirectly(5, true); };

    for (auto* btn : { &rewindButton, &playButton, &stopButton, &recordButton, &loopButton, &settingsButton })
        addAndMakeVisible(btn);

    // BPM slider
    bpmSlider.setSliderStyle(juce::Slider::IncDecButtons);
    bpmSlider.setRange(20.0, 300.0, 0.1);
    bpmSlider.setValue(120.0, juce::dontSendNotification);
    bpmSlider.setNumDecimalPlacesToDisplay(1);
    bpmSlider.onValueChange = [this] {
        audioEngine.setBpm(bpmSlider.getValue());
    };
    bpmLabel.setJustificationType(juce::Justification::centred);
    bpmLabel.setFont(juce::Font(10.0f));
    addAndMakeVisible(bpmLabel);
    addAndMakeVisible(bpmSlider);

    // Time sig
    for (int i = 1; i <= 16; ++i)
        timeSigNumerator.addItem(juce::String(i), i);
    timeSigNumerator.setSelectedId(4, juce::dontSendNotification);

    for (int d : { 2, 4, 8, 16 })
        timeSigDenominator.addItem(juce::String(d), d);
    timeSigDenominator.setSelectedId(4, juce::dontSendNotification);

    addAndMakeVisible(timeSigNumerator);
    addAndMakeVisible(timeSigDenominator);

    // Position display (LCD Style)
    auto digitalFont = juce::Font(juce::Font::getDefaultMonospacedFontName(), 20.0f, juce::Font::bold);

    positionLabel.setFont(digitalFont);
    positionLabel.setColour(juce::Label::textColourId, juce::Colour(0xff00ffcc)); // Bright cyan LED
    positionLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(positionLabel);

    barBeatLabel.setText("1 | 1 | 000", juce::dontSendNotification);
    barBeatLabel.setFont(digitalFont);
    barBeatLabel.setColour(juce::Label::textColourId, juce::Colour(0xffffca28)); // Bright yellow LED
    barBeatLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(barBeatLabel);

    // Master volume
    masterVolumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    masterVolumeSlider.setRange(0.0, 1.0, 0.001);
    masterVolumeSlider.setValue(1.0, juce::dontSendNotification);
    masterVolumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    masterVolumeSlider.onValueChange = [this] {
        audioEngine.setMasterVolume((float)masterVolumeSlider.getValue());
    };
    masterVolumeLabel.setFont(juce::Font(10.0f));
    masterVolumeLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(masterVolumeLabel);
    addAndMakeVisible(masterVolumeSlider);

    monoButton.setToggleable(true);
    monoButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffFFB300));
    monoButton.onClick = [this] {
        audioEngine.setMonoSum(monoButton.getToggleState());
    };
    addAndMakeVisible(monoButton);

    startTimerHz(30);
}

TransportBar::~TransportBar()
{
    audioEngine.removeListener(this);
    stopTimer();
}

void TransportBar::resized()
{
    auto bounds = getLocalBounds().reduced(4);

    // Transport buttons — wider for text labels
    auto btnArea = bounds.removeFromLeft(380);
    int btnW = btnArea.getWidth() / 6;
    rewindButton.setBounds(btnArea.removeFromLeft(btnW).reduced(2));
    playButton.setBounds(  btnArea.removeFromLeft(btnW).reduced(2));
    stopButton.setBounds(  btnArea.removeFromLeft(btnW).reduced(2));
    recordButton.setBounds(btnArea.removeFromLeft(btnW).reduced(2));
    loopButton.setBounds(  btnArea.removeFromLeft(btnW).reduced(2));
    settingsButton.setBounds(btnArea.reduced(2));

    // BPM
    auto bpmArea = bounds.removeFromLeft(120);
    bpmLabel.setBounds(bpmArea.removeFromTop(16));
    bpmSlider.setBounds(bpmArea);

    // Time signature
    auto timeSigArea = bounds.removeFromLeft(70);
    timeSigNumerator.setBounds(timeSigArea.removeFromTop(timeSigArea.getHeight() / 2).reduced(2));
    timeSigDenominator.setBounds(timeSigArea.reduced(2));

    // Position display
    auto posArea = bounds.removeFromLeft(240);
    positionLabel.setBounds(posArea.removeFromTop(posArea.getHeight() / 2));
    barBeatLabel.setBounds(posArea);

    // Master volume (right side)
    auto volArea = bounds.removeFromRight(200);
    masterVolumeLabel.setBounds(volArea.removeFromTop(16));
    monoButton.setBounds(volArea.removeFromLeft(40).reduced(2));
    masterVolumeSlider.setBounds(volArea);
}

void TransportBar::paint(juce::Graphics& g)
{
    // Hardware console metallic background
    juce::ColourGradient grad(juce::Colour(0xff2a2a3e), 0, 0,
                              juce::Colour(0xff12121e), 0, (float)getHeight(), false);
    g.setGradientFill(grad);
    g.fillRect(getLocalBounds());

    // Top and bottom bevels
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.drawHorizontalLine(1, 0.0f, (float)getWidth());
    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.drawHorizontalLine(getHeight() - 1, 0.0f, (float)getWidth());

    // LCD Screen Background
    auto lcdArea = getLocalBounds().removeFromLeft(450 + 120 + 70 + 240).removeFromRight(240).reduced(2, 4).toFloat();
    // Inset shadow
    g.setColour(juce::Colours::black.withAlpha(0.9f));
    g.fillRoundedRectangle(lcdArea, 4.0f);
    // Dark glass reflection
    juce::ColourGradient lcdGrad(juce::Colour(0xff08141e), lcdArea.getX(), lcdArea.getY(),
                                 juce::Colour(0xff020810), lcdArea.getX(), lcdArea.getBottom(), false);
    g.setGradientFill(lcdGrad);
    g.fillRoundedRectangle(lcdArea, 4.0f);
    g.setColour(juce::Colours::white.withAlpha(0.05f));
    g.drawRoundedRectangle(lcdArea, 4.0f, 1.0f);

    // Level meter bars (Segmented LED style)
    auto meterArea = getLocalBounds().removeFromRight(80).reduced(6, 6);
    auto mL = meterArea.removeFromLeft(meterArea.getWidth() / 2 - 2);
    auto mR = meterArea;

    // Meter backgrounds (dark empty LEDs)
    g.setColour(juce::Colour(0xff101015));
    g.fillRect(mL);
    g.fillRect(mR);

    auto drawMeter = [&](juce::Rectangle<int> bounds, float peak)
    {
        int numSegments = 20;
        float segmentH = bounds.getHeight() / (float)numSegments;
        float gap = 1.0f;
        int activeSegments = (int)(peak * numSegments);

        for (int i = 0; i < numSegments; ++i)
        {
            float yPos = bounds.getBottom() - (i + 1) * segmentH;
            juce::Rectangle<float> seg(bounds.getX(), yPos + gap / 2, bounds.getWidth(), segmentH - gap);

            if (i < activeSegments)
            {
                juce::Colour c = (i > 16) ? OrpheusLookAndFeel::accentDanger() :
                                 (i > 13) ? OrpheusLookAndFeel::accentWarning() :
                                            OrpheusLookAndFeel::accentSuccess();
                g.setColour(c);
                g.fillRect(seg);
                // LED Glow
                g.setColour(c.withAlpha(0.3f));
                g.fillRect(seg.expanded(1.0f));
            }
            else
            {
                g.setColour(juce::Colours::white.withAlpha(0.03f));
                g.fillRect(seg);
            }
        }
    };

    drawMeter(mL, peakL);
    drawMeter(mR, peakR);
}

void TransportBar::updatePositionDisplay()
{
    double pos = audioEngine.getPlayheadPosition();
    int hours = (int)(pos / 3600.0);
    int mins  = (int)(pos / 60.0) % 60;
    int secs  = (int)pos % 60;
    int ms    = (int)(pos * 1000.0) % 1000;

    positionLabel.setText(
        juce::String::formatted("%d:%02d:%02d.%03d", hours, mins, secs, ms),
        juce::dontSendNotification);

    double bpm = audioEngine.getBpm();
    double secondsPerBeat = 60.0 / bpm;
    double secondsPerBar  = secondsPerBeat * audioEngine.getTimeSigNumerator();
    int    bar  = (int)(pos / secondsPerBar) + 1;
    double rem  = std::fmod(pos, secondsPerBar);
    int    beat = (int)(rem / secondsPerBeat) + 1;
    int    tick = (int)((std::fmod(rem, secondsPerBeat) / secondsPerBeat) * 1000.0);

    barBeatLabel.setText(
        juce::String::formatted("%d | %d | %03d", bar, beat, tick),
        juce::dontSendNotification);
}

void TransportBar::timerCallback()
{
    updatePositionDisplay();

    peakL = audioEngine.getMasterPeakLeft();
    peakR = audioEngine.getMasterPeakRight();
    repaint(getWidth() - 84, 0, 84, getHeight());

    // Update play button state
    bool playing = audioEngine.isPlaying();
    playButton.setColour(juce::TextButton::buttonColourId, playing ? juce::Colour(0xff2e7d32) : juce::Colour(0xff1b5e20));
    playButton.setButtonText(playing ? juce::CharPointer_UTF8("\xe2\x8f\xb8") : juce::CharPointer_UTF8("\xe2\x96\xb6")); // Pause: ⏸, Play: ▶

    bool recording = audioEngine.isRecording();
    recordButton.setColour(juce::TextButton::buttonColourId,
        recording ? OrpheusLookAndFeel::accentDanger() : OrpheusLookAndFeel::accentDanger().darker(0.3f));

    bool looping = audioEngine.isLooping();
    loopButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::cyan);
    loopButton.setToggleState(looping, juce::dontSendNotification);
}

void TransportBar::playbackStarted() { repaint(); }
void TransportBar::playbackStopped() { repaint(); }
