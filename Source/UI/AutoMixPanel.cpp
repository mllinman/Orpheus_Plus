#include "AutoMixPanel.h"

AutoMixPanel::AutoMixPanel(AudioEngine* engine, MasteringModule* master)
    : juce::Component("Auto-Mix Assistant"), audioEngine(engine), masteringModule(master)
{
    addAndMakeVisible(runMixButton);
    addAndMakeVisible(referenceLabel);

    referenceLabel.setJustificationType(juce::Justification::centred);
    referenceLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.7f));
    
    // Aesthetic Styling
    runMixButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a2a35));
    runMixButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    
    runMixButton.onClick = [this]() {
        if (audioEngine != nullptr)
        {
            autoMixer.analyzeSession(audioEngine);
            autoMixer.applyBalancing(audioEngine);
        }
    };
}

void AutoMixPanel::paint(juce::Graphics& g)
{
    // Glassmorphic background
    g.fillAll(juce::Colour(0xff121218).withAlpha(0.85f));
    
    auto bounds = getLocalBounds().toFloat();
    
    // Draw drop zone border
    g.setColour(juce::Colour(0xff4a4a5d));
    g.drawRoundedRectangle(bounds.reduced(20.0f), 10.0f, 2.0f);
}

void AutoMixPanel::resized()
{
    auto area = getLocalBounds().reduced(30);
    
    referenceLabel.setBounds(area.removeFromTop(area.getHeight() / 2));
    
    area.removeFromTop(20);
    runMixButton.setBounds(area.removeFromTop(40).withSizeKeepingCentre(200, 40));
}

bool AutoMixPanel::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (auto file : files)
    {
        if (file.endsWithIgnoreCase(".wav") || file.endsWithIgnoreCase(".mp3") || file.endsWithIgnoreCase(".flac"))
            return true;
    }
    return false;
}

void AutoMixPanel::filesDropped(const juce::StringArray& files, int /*x*/, int /*y*/)
{
    for (auto path : files)
    {
        juce::File f(path);
        if (f.existsAsFile())
        {
            currentReferenceFile = f;
            referenceLabel.setText("Reference: " + f.getFileName(), juce::dontSendNotification);
            
            if (masteringModule != nullptr)
            {
                masteringModule->loadReferenceFile(currentReferenceFile);
            }
            break; // Just take the first valid audio file
        }
    }
}

