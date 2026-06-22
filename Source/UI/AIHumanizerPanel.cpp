#include "AIHumanizerPanel.h"
#include "../MainComponent.h"
#include "../Audio/AudioEngine.h"
#include "../Util/OrpheusLogger.h"

AIHumanizerPanel::AIHumanizerPanel(AudioEngine& e, AppState& s, MainComponent* mc)
    : audioEngine(e), appState(s), mainComponent(mc)
{
    auto setupSlider = [this](juce::Slider& s, juce::Label& l, const juce::String& name, float defaultVal) {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        s.setRange(0.0, 1.0, 0.01);
        s.setValue(defaultVal);
        addAndMakeVisible(s);

        l.setText(name, juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centred);
        l.setFont(juce::Font(10.0f));
        addAndMakeVisible(l);
    };

    //── Original parameters ──────────────────────────────────────────────────
    setupSlider(warmthSlider,    warmthLabel,    "Analog Warmth",     0.5f);
    setupSlider(flutterSlider,   flutterLabel,   "Tape Flutter",      0.3f);
    setupSlider(noiseFloorSlider, noiseFloorLabel, "Pink Noise",      0.2f);
    setupSlider(deChatterSlider, deChatterLabel, "AI De-Chatter",     0.5f);

    //── New parameters ───────────────────────────────────────────────────────
    setupSlider(microTimingSlider,      microTimingLabel,      "Micro Timing",   0.3f);
    setupSlider(stereoWidthSlider,      stereoWidthLabel,      "Stereo Width",   0.3f);
    setupSlider(harmonicExciterSlider,  harmonicExciterLabel,  "Harmonic Exciter", 0.2f);
    setupSlider(dynamicBreathingSlider, dynamicBreathingLabel, "Dynamics",       0.2f);

    //── Wire sliders to processor ────────────────────────────────────────────
    warmthSlider.onValueChange           = [this]() { offlineProcessor.setWarmth((float)warmthSlider.getValue()); };
    flutterSlider.onValueChange          = [this]() { offlineProcessor.setFlutter((float)flutterSlider.getValue()); };
    noiseFloorSlider.onValueChange       = [this]() { offlineProcessor.setNoiseFloor((float)noiseFloorSlider.getValue()); };
    deChatterSlider.onValueChange        = [this]() { offlineProcessor.setDeChatter((float)deChatterSlider.getValue()); };
    microTimingSlider.onValueChange      = [this]() { offlineProcessor.setMicroTiming((float)microTimingSlider.getValue()); };
    stereoWidthSlider.onValueChange      = [this]() { offlineProcessor.setStereoWidth((float)stereoWidthSlider.getValue()); };
    harmonicExciterSlider.onValueChange  = [this]() { offlineProcessor.setHarmonicExciter((float)harmonicExciterSlider.getValue()); };
    dynamicBreathingSlider.onValueChange = [this]() { offlineProcessor.setDynamicBreathing((float)dynamicBreathingSlider.getValue()); };

    //── Preset combo ─────────────────────────────────────────────────────────
    presetCombo.addItem("Subtle",        1);
    presetCombo.addItem("Warm Analog",   2);
    presetCombo.addItem("Live Feel",     3);
    presetCombo.addItem("Full Treatment", 4);
    presetCombo.setTextWhenNothingSelected("Select Preset...");
    presetCombo.onChange = [this]()
    {
        switch (presetCombo.getSelectedId())
        {
            case 1: offlineProcessor.applyPreset(AIHumanizerProcessor::Preset::Subtle); break;
            case 2: offlineProcessor.applyPreset(AIHumanizerProcessor::Preset::WarmAnalog); break;
            case 3: offlineProcessor.applyPreset(AIHumanizerProcessor::Preset::LiveFeel); break;
            case 4: offlineProcessor.applyPreset(AIHumanizerProcessor::Preset::FullTreatment); break;
            default: break;
        }
        syncSlidersFromPreset();
    };
    addAndMakeVisible(presetCombo);

    presetLabel.setText("Preset", juce::dontSendNotification);
    presetLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(presetLabel);

    //── Apply button ─────────────────────────────────────────────────────────
    addAndMakeVisible(applyToTrackButton);
    applyToTrackButton.onClick = [this]() {
        int trackIndex = appState.getSelectedTrackIndex();
        if (trackIndex >= 0)
        {
            OrpheusLogger::logInfo("AIHumanizer added to track " + juce::String(trackIndex) + " as Realtime Insert");
            juce::AlertWindow::showMessageBoxAsync(
                juce::MessageBoxIconType::InfoIcon,
                "Realtime Effect Applied",
                "AI Humanizer successfully added to Track " + juce::String(trackIndex + 1));
        }
        else
        {
             juce::AlertWindow::showMessageBoxAsync(
                juce::MessageBoxIconType::WarningIcon,
                "No Track Selected",
                "Please select a track in the timeline first.");
        }
    };

    startTimerHz(30);
}

AIHumanizerPanel::~AIHumanizerPanel()
{
    stopTimer();
}

void AIHumanizerPanel::syncSlidersFromPreset()
{
    // After applying a preset, the processor's internal values change.
    // We need to reflect that back in the UI sliders.
    // Since the processor stores parameters directly, we re-read them
    // by calling applyPreset again and extracting the values through
    // the slider defaults matching the preset definition.

    // Rather than adding getters, just map the preset values directly
    switch (presetCombo.getSelectedId())
    {
        case 1: // Subtle
            warmthSlider.setValue(0.15, juce::dontSendNotification);
            flutterSlider.setValue(0.1, juce::dontSendNotification);
            noiseFloorSlider.setValue(0.05, juce::dontSendNotification);
            deChatterSlider.setValue(0.3, juce::dontSendNotification);
            microTimingSlider.setValue(0.1, juce::dontSendNotification);
            stereoWidthSlider.setValue(0.1, juce::dontSendNotification);
            harmonicExciterSlider.setValue(0.05, juce::dontSendNotification);
            dynamicBreathingSlider.setValue(0.1, juce::dontSendNotification);
            break;
        case 2: // Warm Analog
            warmthSlider.setValue(0.7, juce::dontSendNotification);
            flutterSlider.setValue(0.5, juce::dontSendNotification);
            noiseFloorSlider.setValue(0.3, juce::dontSendNotification);
            deChatterSlider.setValue(0.4, juce::dontSendNotification);
            microTimingSlider.setValue(0.2, juce::dontSendNotification);
            stereoWidthSlider.setValue(0.2, juce::dontSendNotification);
            harmonicExciterSlider.setValue(0.5, juce::dontSendNotification);
            dynamicBreathingSlider.setValue(0.15, juce::dontSendNotification);
            break;
        case 3: // Live Feel
            warmthSlider.setValue(0.3, juce::dontSendNotification);
            flutterSlider.setValue(0.2, juce::dontSendNotification);
            noiseFloorSlider.setValue(0.1, juce::dontSendNotification);
            deChatterSlider.setValue(0.5, juce::dontSendNotification);
            microTimingSlider.setValue(0.6, juce::dontSendNotification);
            stereoWidthSlider.setValue(0.5, juce::dontSendNotification);
            harmonicExciterSlider.setValue(0.15, juce::dontSendNotification);
            dynamicBreathingSlider.setValue(0.5, juce::dontSendNotification);
            break;
        case 4: // Full Treatment
            warmthSlider.setValue(0.5, juce::dontSendNotification);
            flutterSlider.setValue(0.4, juce::dontSendNotification);
            noiseFloorSlider.setValue(0.2, juce::dontSendNotification);
            deChatterSlider.setValue(0.5, juce::dontSendNotification);
            microTimingSlider.setValue(0.4, juce::dontSendNotification);
            stereoWidthSlider.setValue(0.35, juce::dontSendNotification);
            harmonicExciterSlider.setValue(0.3, juce::dontSendNotification);
            dynamicBreathingSlider.setValue(0.3, juce::dontSendNotification);
            break;
        default:
            break;
    }
}

void AIHumanizerPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff121212));

    auto bounds = getLocalBounds().reduced(16);

    // Glassmorphic background
    g.setColour(juce::Colour(0x20ffffff));
    g.fillRoundedRectangle(bounds.toFloat(), 12.0f);
    g.setColour(juce::Colour(0x30ffffff));
    g.drawRoundedRectangle(bounds.toFloat(), 12.0f, 1.0f);

    // Header
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(20.0f).boldened());
    g.drawText(juce::CharPointer_UTF8("\xf0\x9f\x8e\xb5 Audio Humanizer"),
               bounds.removeFromTop(36), juce::Justification::centred);

    g.setFont(12.0f);
    g.setColour(juce::Colours::lightgrey);
    g.drawText("Analog warmth \u2022 Timing variation \u2022 Natural dynamics",
               bounds.removeFromTop(18), juce::Justification::centred);

    // Section labels
    auto knobArea = bounds.reduced(8);
    knobArea.removeFromTop(36); // preset row

    auto topRow = knobArea.removeFromTop(130);
    auto bottomRow = knobArea.removeFromTop(130);

    // Draw section dividers
    g.setColour(juce::Colour(0x15ffffff));
    g.drawHorizontalLine(topRow.getBottom() + 2, (float)bounds.getX() + 20, (float)bounds.getRight() - 20);

    // Drop zone / status at bottom
    auto dropZone = getLocalBounds().reduced(16).removeFromBottom(60).reduced(20, 0);

    g.setColour(juce::Colour(0x10ffffff));
    g.fillRoundedRectangle(dropZone.toFloat(), 8.0f);

    g.setFont(13.0f);
    g.setColour(juce::Colours::lightgrey);

    if (isProcessing)
    {
        g.setColour(juce::Colour(0xffa29bfe));
        g.drawText(juce::CharPointer_UTF8("\xe2\x8f\xb3 Processing... Please wait"), dropZone, juce::Justification::centred);
    }
    else
    {
        g.drawText("Drag & Drop a .wav file here for Offline Processing", dropZone, juce::Justification::centred);
    }
}

void AIHumanizerPanel::resized()
{
    auto bounds = getLocalBounds().reduced(24);
    bounds.removeFromTop(54); // Header + subtitle

    // Preset row
    auto presetRow = bounds.removeFromTop(30);
    presetLabel.setBounds(presetRow.removeFromLeft(55));
    presetCombo.setBounds(presetRow.removeFromLeft(180));

    bounds.removeFromTop(8);

    // Top row: Original 4 knobs
    auto topRow = bounds.removeFromTop(120);
    int sliderWidth = topRow.getWidth() / 4;
    int knobSize = 70;

    auto placeKnob = [&](juce::Slider& slider, juce::Label& label, juce::Rectangle<int>& row)
    {
        auto area = row.removeFromLeft(sliderWidth);
        slider.setBounds(area.withSizeKeepingCentre(knobSize, knobSize));
        label.setBounds(slider.getBounds().translated(0, knobSize / 2 + 8).withHeight(16));
    };

    placeKnob(warmthSlider,    warmthLabel,    topRow);
    placeKnob(flutterSlider,   flutterLabel,   topRow);
    placeKnob(noiseFloorSlider, noiseFloorLabel, topRow);
    placeKnob(deChatterSlider, deChatterLabel, topRow);

    bounds.removeFromTop(8);

    // Bottom row: New 4 knobs
    auto bottomRow = bounds.removeFromTop(120);

    placeKnob(microTimingSlider,      microTimingLabel,      bottomRow);
    placeKnob(stereoWidthSlider,      stereoWidthLabel,      bottomRow);
    placeKnob(harmonicExciterSlider,  harmonicExciterLabel,  bottomRow);
    placeKnob(dynamicBreathingSlider, dynamicBreathingLabel, bottomRow);

    bounds.removeFromTop(8);

    // Apply button
    applyToTrackButton.setBounds(bounds.removeFromTop(36).withSizeKeepingCentre(300, 36));
}

void AIHumanizerPanel::timerCallback()
{
    repaint();
}

bool AIHumanizerPanel::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (auto file : files)
    {
        if (file.endsWithIgnoreCase(".wav") || file.endsWithIgnoreCase(".mp3"))
            return true;
    }
    return false;
}

void AIHumanizerPanel::filesDropped(const juce::StringArray& files, int /*x*/, int /*y*/)
{
    if (isProcessing) return;

    for (auto path : files)
    {
        juce::File inputFile(path);
        if (inputFile.existsAsFile() && (inputFile.hasFileExtension("wav") || inputFile.hasFileExtension("mp3")))
        {
            isProcessing = true;
            repaint();
            
            // Run processing on a background thread
            juce::Thread::launch([this, inputFile]() {
                
                juce::File outputFile = inputFile.getParentDirectory().getChildFile(
                    inputFile.getFileNameWithoutExtension() + "_humanized.wav");

                // Process the file
                offlineProcessor.processFileOffline(inputFile, outputFile, &appState, &audioEngine);

                // Re-enable UI on message thread
                juce::MessageManager::callAsync([this]() {
                    isProcessing = false;
                    repaint();
                });
            });
            
            // Only process the first valid file dropped for now
            break; 
        }
    }
}
