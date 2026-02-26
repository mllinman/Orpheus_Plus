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

    // Position display
    // Fix Font deprecation and casts
    auto monoFont = juce::Font(juce::Font::getDefaultMonospacedFontName(), 14.0f, juce::Font::plain);

    positionLabel.setFont(monoFont);
    positionLabel.setColour(juce::Label::textColourId, OrpheusLookAndFeel::accentSecondary());
    positionLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(positionLabel);

    barBeatLabel.setText("1 | 1 | 000", juce::dontSendNotification);
    barBeatLabel.setFont(monoFont);
    barBeatLabel.setColour(juce::Label::textColourId, OrpheusLookAndFeel::accentWarning());
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
    // Gradient matching legacy .transport-bar
    g.setGradientFill(juce::ColourGradient(
        OrpheusLookAndFeel::bgSurface(), 0, 0,
        OrpheusLookAndFeel::bgDarker(), 0, (float)getHeight(), false));
    g.fillRect(getLocalBounds());
    g.setColour(OrpheusLookAndFeel::borderSubtle());
    g.drawHorizontalLine(0, 0.0f, (float)getWidth());

    // Level meter bars
    auto meterArea = getLocalBounds().removeFromRight(80).reduced(4);
    auto mL = meterArea.removeFromLeft(meterArea.getWidth() / 2 - 2);
    auto mR = meterArea;

    g.setColour(OrpheusLookAndFeel::bgDark());
    g.fillRect(mL);
    g.fillRect(mR);

    float h = (float)mL.getHeight();
    // Meter gradient: green -> yellow -> red
    juce::Colour meterCol = peakL > 0.9f ? OrpheusLookAndFeel::accentDanger() :
                            peakL > 0.7f ? OrpheusLookAndFeel::accentWarning() :
                                           OrpheusLookAndFeel::accentSuccess();
    g.setColour(meterCol);
    g.fillRect(mL.withTrimmedTop((int)((1.0f - peakL) * h)));

    meterCol = peakR > 0.9f ? OrpheusLookAndFeel::accentDanger() :
               peakR > 0.7f ? OrpheusLookAndFeel::accentWarning() :
                               OrpheusLookAndFeel::accentSuccess();
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

    // Update play button text
    bool playing = audioEngine.isPlaying();
    playButton.setButtonText(playing ? "Pause" : "Play");

    bool recording = audioEngine.isRecording();
    recordButton.setColour(juce::TextButton::buttonColourId,
        recording ? OrpheusLookAndFeel::accentDanger() : OrpheusLookAndFeel::accentDanger().darker(0.3f));

    bool looping = audioEngine.isLooping();
    loopButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::cyan);
    loopButton.setToggleState(looping, juce::dontSendNotification);
}

void TransportBar::playbackStarted() { repaint(); }
void TransportBar::playbackStopped() { repaint(); }
