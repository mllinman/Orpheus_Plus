#include "VoiceCloningPanel.h"
#include "../MainComponent.h"
#include "../VoiceCloning/VoiceConversionProcessor.h"

VoiceCloningPanel::VoiceCloningPanel(AudioEngine& engine, MainComponent* mainComp)
    : audioEngine(engine), mainComponent(mainComp)
{
    addAndMakeVisible(loadBtn);
    addAndMakeVisible(enableToggle);
    addAndMakeVisible(timbreMixSlider);
    addAndMakeVisible(pitchShiftSlider);
    addAndMakeVisible(statusLabel);

    loadBtn.onClick = [this] { loadReferenceSong(); };
    enableToggle.onClick = [this] {
        // Find the active VoiceConversionProcessor and toggle it
        // For demonstration, we assume it's attached to the armed track or master
    };

    timbreMixSlider.setRange(0.0, 1.0, 0.01);
    timbreMixSlider.setValue(1.0);
    pitchShiftSlider.setRange(-12.0, 12.0, 0.1);
    pitchShiftSlider.setValue(0.0);

    statusLabel.setText("Awaiting Reference Song...", juce::dontSendNotification);
    statusLabel.setJustificationType(juce::Justification::centred);

    startTimerHz(30);
}

VoiceCloningPanel::~VoiceCloningPanel()
{
}

void VoiceCloningPanel::timerCallback()
{
    if (embeddingExtractor.isProcessing())
    {
        extractionProgress = embeddingExtractor.getProgress();
        statusLabel.setText("Extracting Voice Profile: " + juce::String(juce::roundToInt(extractionProgress * 100)) + "%", juce::dontSendNotification);
        repaint();
    }
}

void VoiceCloningPanel::loadReferenceSong()
{
    fChooser = std::make_unique<juce::FileChooser>("Select a song with vocals...",
                                                   juce::File::getSpecialLocation(juce::File::userMusicDirectory),
                                                   "*.wav;*.mp3;*.flac");

    auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    fChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc)
    {
        auto file = fc.getResult();
        if (file.existsAsFile())
        {
            processUploadedSong(file);
        }
    });
}

void VoiceCloningPanel::processUploadedSong(const juce::File& file)
{
    currentReferenceFile = file;
    statusLabel.setText("Separating Stems...", juce::dontSendNotification);

    // 1. Separate Stems
    audioEngine.getStemSeparator().separate(file, *mainComponent->getAppState(), [this](StemSeparationResult result) {
        
        if (result.vocals.existsAsFile()) {
            statusLabel.setText("Extracting AI Embedding...", juce::dontSendNotification);
            
            // 2. Train AI Voice Profile
            embeddingExtractor.trainProfileAsync(result.vocals, [this](bool success, const juce::String& onnxModelPath) {
                if (success) {
                    statusLabel.setText("Voice Profile Ready!", juce::dontSendNotification);
                    // 3. Send to Processor
                    // In a full implementation, you would iterate over tracks and send the .onnx model
                    // to the active VoiceConversionProcessor.
                    juce::Logger::writeToLog("Voice Cloning setup complete. Model path: " + onnxModelPath);
                } else {
                    statusLabel.setText("Voice Profile Training Failed.", juce::dontSendNotification);
                }
            });
        } else {
            statusLabel.setText("Stem Separation failed to produce vocals.", juce::dontSendNotification);
        }
    });
}

void VoiceCloningPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
    
    auto area = getLocalBounds().reduced(20);
    paintGlassmorphicCard(g, area, "AI Voice Timbre Transfer");
    
    g.setColour(juce::Colours::white);
    g.setFont(14.0f);
    g.drawText("Timbre Mix", timbreMixSlider.getBounds().translated(0, 40), juce::Justification::centred);
    g.drawText("Pitch Shift", pitchShiftSlider.getBounds().translated(0, 40), juce::Justification::centred);
}

void VoiceCloningPanel::paintGlassmorphicCard(juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& title)
{
    g.setColour(juce::Colour(0x30FFFFFF));
    g.fillRoundedRectangle(bounds.toFloat(), 15.0f);
    
    g.setColour(juce::Colour(0x60FFFFFF));
    g.drawRoundedRectangle(bounds.toFloat(), 15.0f, 1.0f);
    
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(24.0f, juce::Font::bold));
    g.drawText(title, bounds.withHeight(50).withX(bounds.getX() + 20), juce::Justification::centredLeft);
}

void VoiceCloningPanel::resized()
{
    auto area = getLocalBounds().reduced(40);
    area.removeFromTop(50); // Title space

    auto topRow = area.removeFromTop(40);
    loadBtn.setBounds(topRow.removeFromLeft(200));
    enableToggle.setBounds(topRow.removeFromRight(150));
    
    area.removeFromTop(20);
    statusLabel.setBounds(area.removeFromTop(30));
    
    area.removeFromTop(40);
    
    int dialSize = 100;
    auto dials = area.removeFromTop(dialSize);
    timbreMixSlider.setBounds(dials.removeFromLeft(dialSize));
    dials.removeFromLeft(50);
    pitchShiftSlider.setBounds(dials.removeFromLeft(dialSize));
}
