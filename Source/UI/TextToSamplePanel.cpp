#include "TextToSamplePanel.h"

TextToSamplePanel::TextToSamplePanel()
{
    addAndMakeVisible(promptEditor);
    promptEditor.setMultiLine(true);
    promptEditor.setReturnKeyStartsNewLine(false);
    promptEditor.setTextToShowWhenEmpty("Describe a sound (e.g., 'vintage 808 kick with heavy tape saturation')...", juce::Colours::grey);

    addAndMakeVisible(generateButton);
    generateButton.onClick = [this] { triggerGeneration(); };

    addAndMakeVisible(statusLabel);
    statusLabel.setJustificationType(juce::Justification::centred);
    statusLabel.setText("Ready.", juce::dontSendNotification);
}

TextToSamplePanel::~TextToSamplePanel()
{
    stopTimer();
}

void TextToSamplePanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e24)); // Dark background
    g.setColour(juce::Colours::white);
    g.setFont(16.0f);
    g.drawText("AI Sound Designer", getLocalBounds().removeFromTop(30), juce::Justification::centred, true);
}

void TextToSamplePanel::resized()
{
    auto area = getLocalBounds().reduced(10);
    area.removeFromTop(30); // header

    promptEditor.setBounds(area.removeFromTop(80));
    area.removeFromTop(10); // spacer
    
    generateButton.setBounds(area.removeFromTop(40));
    area.removeFromTop(10); // spacer
    
    statusLabel.setBounds(area.removeFromTop(30));
}

void TextToSamplePanel::triggerGeneration()
{
    if (isGenerating) return;

    juce::String prompt = promptEditor.getText();
    if (prompt.isEmpty()) return;

    statusLabel.setText("Generating audio via ONNX...", juce::dontSendNotification);
    generateButton.setEnabled(false);
    isGenerating = true;

    // Launch asynchronously (in a real app, this would spawn a juce::Thread)
    generator.generateSampleFromText(prompt, 2.0, 44100.0);
    startTimer(100);
}

void TextToSamplePanel::timerCallback()
{
    if (generator.isFinished())
    {
        stopTimer();
        isGenerating = false;
        generateButton.setEnabled(true);
        statusLabel.setText("Generation complete! Drag waveform to timeline.", juce::dontSendNotification);
        
        // Retrieve buffer and display it (UI waveform mock)
        juce::AudioBuffer<float> generatedBuffer = generator.getGeneratedBuffer();
        juce::Logger::writeToLog("Generated " + juce::String(generatedBuffer.getNumSamples()) + " samples.");
    }
}
