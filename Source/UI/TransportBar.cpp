#include <JuceHeader.h>
#include "TransportBar.h"

TransportBar::TransportBar(AudioEngine& e, juce::ApplicationCommandManager& c)
    : audioEngine(e), commandManager(c)
{
    audioEngine.addListener(this);

    auto styleBtn = [](juce::TextButton& btn, juce::Colour col = juce::Colour(0xff2d2d44))
    {
        btn.setColour(juce::TextButton::buttonColourId, col);
        btn.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    };

    styleBtn(rewindButton);
    styleBtn(playButton,   juce::Colour(0xff1b5e20));
    styleBtn(stopButton,   juce::Colour(0xff2d2d44));
    styleBtn(recordButton, juce::Colour(0xffb71c1c));
    styleBtn(loopButton);

    rewindButton.onClick = [this] { audioEngine.stop(); };
    playButton.onClick   = [this] { audioEngine.togglePlayback(); };
    stopButton.onClick   = [this] { audioEngine.stop(); };
    recordButton.onClick = [this] { audioEngine.toggleRecord(); };
    loopButton.setToggleable(true);

    for (auto* btn : { &rewindButton, &playButton, &stopButton, &recordButton, &loopButton })
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

    // Position display
    // Fix Font deprecation and casts
    auto monoFont = juce::Font(juce::Font::getDefaultMonospacedFontName(), 14.0f, juce::Font::plain);

    positionLabel.setFont(monoFont);
    positionLabel.setColour(juce::Label::textColourId, juce::Colour(0xff4fc3f7));
    positionLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(positionLabel);

    barBeatLabel.setText("1 | 1 | 000", juce::dontSendNotification);
    barBeatLabel.setFont(monoFont);
    barBeatLabel.setColour(juce::Label::textColourId, juce::Colour(0xffffd54f));
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

    // Transport buttons
    auto btnArea = bounds.removeFromLeft(200);
    int btnW = btnArea.getWidth() / 5;
    rewindButton.setBounds(btnArea.removeFromLeft(btnW).reduced(2));
    playButton.setBounds(  btnArea.removeFromLeft(btnW).reduced(2));
    stopButton.setBounds(  btnArea.removeFromLeft(btnW).reduced(2));
    recordButton.setBounds(btnArea.removeFromLeft(btnW).reduced(2));
    loopButton.setBounds(  btnArea.reduced(2));

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
    auto volArea = bounds.removeFromRight(160);
    masterVolumeLabel.setBounds(volArea.removeFromTop(16));
    masterVolumeSlider.setBounds(volArea);
}

void TransportBar::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff16213e));
    g.setColour(juce::Colour(0xff0d0d1a));
    g.drawHorizontalLine(getHeight() - 1, 0.0f, (float)getWidth());

    // Level meter bars (simple)
    auto meterArea = getLocalBounds().removeFromRight(80).reduced(4);
    auto mL = meterArea.removeFromLeft(meterArea.getWidth() / 2 - 2);
    auto mR = meterArea;

    g.setColour(juce::Colour(0xff1a1a2e));
    g.fillRect(mL);
    g.fillRect(mR);

    float h = (float)mL.getHeight();
    juce::Colour meterCol = peakL > 0.9f ? juce::Colour(0xffe94560) :
                            peakL > 0.7f ? juce::Colour(0xffffd54f) :
                                           juce::Colour(0xff4caf50);
    g.setColour(meterCol);
    g.fillRect(mL.withTrimmedTop((int)((1.0f - peakL) * h)));

    meterCol = peakR > 0.9f ? juce::Colour(0xffe94560) :
               peakR > 0.7f ? juce::Colour(0xffffd54f) :
                               juce::Colour(0xff4caf50);
    g.setColour(meterCol);
    g.fillRect(mR.withTrimmedTop((int)((1.0f - peakR) * h)));
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

    // Update play button appearance
    bool playing = audioEngine.isPlaying();
    playButton.setButtonText(playing ? "⏸" : "▶");

    bool recording = audioEngine.isRecording();
    recordButton.setColour(juce::TextButton::buttonColourId,
        recording ? juce::Colour(0xffe94560) : juce::Colour(0xffb71c1c));
}

void TransportBar::playbackStarted() { repaint(); }
void TransportBar::playbackStopped() { repaint(); }
