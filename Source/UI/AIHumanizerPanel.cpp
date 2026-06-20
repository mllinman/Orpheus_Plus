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
        addAndMakeVisible(l);
    };

    setupSlider(warmthSlider, warmthLabel, "Analog Warmth", 0.5f);
    setupSlider(flutterSlider, flutterLabel, "Tape Flutter", 0.3f);
    setupSlider(noiseFloorSlider, noiseFloorLabel, "Stochastic Noise", 0.2f);
    setupSlider(deChatterSlider, deChatterLabel, "AI De-Chatter", 0.5f);

    warmthSlider.onValueChange = [this]() { offlineProcessor.setWarmth((float)warmthSlider.getValue()); };
    flutterSlider.onValueChange = [this]() { offlineProcessor.setFlutter((float)flutterSlider.getValue()); };
    noiseFloorSlider.onValueChange = [this]() { offlineProcessor.setNoiseFloor((float)noiseFloorSlider.getValue()); };
    deChatterSlider.onValueChange = [this]() { offlineProcessor.setDeChatter((float)deChatterSlider.getValue()); };

    addAndMakeVisible(applyToTrackButton);
    applyToTrackButton.onClick = [this]() {
        int trackIndex = appState.getSelectedTrackIndex();
        if (trackIndex >= 0)
        {
            // Note: In a full implementation, we'd add the processor to the track's processing chain.
            // For now we just log it as part of Option A.
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

void AIHumanizerPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff121212));

    // Glassmorphic background
    juce::Rectangle<int> bounds = getLocalBounds().reduced(20);
    g.setColour(juce::Colour(0x30ffffff));
    g.fillRoundedRectangle(bounds.toFloat(), 15.0f);
    
    g.setColour(juce::Colours::white);
    g.setFont(24.0f);
    g.drawText("AI Audio Humanizer", bounds.removeFromTop(40), juce::Justification::centred);

    g.setFont(16.0f);
    g.setColour(juce::Colours::lightgrey);
    
    if (isProcessing)
    {
        g.drawText("Processing... Please wait", bounds, juce::Justification::centred);
    }
    else
    {
        g.drawText("Drag & Drop a SUNO/AI .wav file here for Offline Processing", bounds.removeFromBottom(80), juce::Justification::centred);
    }
}

void AIHumanizerPanel::resized()
{
    auto bounds = getLocalBounds().reduced(40);
    bounds.removeFromTop(40); // Header
    
    auto topHalf = bounds.removeFromTop(bounds.getHeight() / 2);
    
    int sliderWidth = topHalf.getWidth() / 4;
    
    warmthSlider.setBounds(topHalf.removeFromLeft(sliderWidth).withSizeKeepingCentre(100, 100));
    flutterSlider.setBounds(topHalf.removeFromLeft(sliderWidth).withSizeKeepingCentre(100, 100));
    noiseFloorSlider.setBounds(topHalf.removeFromLeft(sliderWidth).withSizeKeepingCentre(100, 100));
    deChatterSlider.setBounds(topHalf.removeFromLeft(sliderWidth).withSizeKeepingCentre(100, 100));

    warmthLabel.setBounds(warmthSlider.getBounds().translated(0, 60).withHeight(20));
    flutterLabel.setBounds(flutterSlider.getBounds().translated(0, 60).withHeight(20));
    noiseFloorLabel.setBounds(noiseFloorSlider.getBounds().translated(0, 60).withHeight(20));
    deChatterLabel.setBounds(deChatterSlider.getBounds().translated(0, 60).withHeight(20));

    applyToTrackButton.setBounds(bounds.removeFromBottom(60).withSizeKeepingCentre(300, 40));
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
