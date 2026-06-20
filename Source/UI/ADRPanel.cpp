#include "ADRPanel.h"
#include "../Timeline/AudioClip.h"
#include <cmath>

ADRPanel::ADRPanel(AudioEngine& engine) : audioEngine(engine)
{
    addAndMakeVisible(analyzeLocationToneBtn);
    analyzeLocationToneBtn.onClick = [this] { analyzeLocationToneClicked(); };

    addAndMakeVisible(applyADRMatchBtn);
    applyADRMatchBtn.onClick = [this] { applyADRMatchClicked(); };
    applyADRMatchBtn.setEnabled(false); // Enable only after room tone is captured

    addAndMakeVisible(statusLabel);
    statusLabel.setText("Awaiting Location Audio Analysis...", juce::dontSendNotification);
    statusLabel.setJustificationType(juce::Justification::centred);
}

ADRPanel::~ADRPanel()
{
}

void ADRPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::transparentBlack);
    
    // Glassmorphic background
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 8.0f);
    
    g.setColour(juce::Colours::white);
    g.setFont(18.0f);
    g.drawText("Dialogue Leveling & Foley Match", getLocalBounds().removeFromTop(40), juce::Justification::centred, false);
}

void ADRPanel::resized()
{
    auto area = getLocalBounds().reduced(20);
    area.removeFromTop(30); // Header space

    statusLabel.setBounds(area.removeFromTop(30));
    area.removeFromTop(10);
    analyzeLocationToneBtn.setBounds(area.removeFromTop(40));
    area.removeFromTop(10);
    applyADRMatchBtn.setBounds(area.removeFromTop(40));
}

void ADRPanel::analyzeLocationToneClicked()
{
    // Fetch current active clip from engine
    auto* activeTrack = audioEngine.getTrack(0); // Mock target
    if (activeTrack && !activeTrack->clips.isEmpty())
    {
        if (auto* audioClip = dynamic_cast<AudioClip*>(activeTrack->clips.getFirst()))
        {
            adrProcessor.analyzeLocationAudio(audioClip->audioData);
            statusLabel.setText("Room Tone Extracted. Ready for ADR.", juce::dontSendNotification);
            applyADRMatchBtn.setEnabled(true);
            return;
        }
    }
    
    statusLabel.setText("No Audio Clip selected for tone analysis.", juce::dontSendNotification);
}

void ADRPanel::applyADRMatchClicked()
{
    // Apply to selected ADR clip
    auto* activeTrack = audioEngine.getTrack(1); // Mock target for ADR track
    if (activeTrack && !activeTrack->clips.isEmpty())
    {
        if (auto* audioClip = dynamic_cast<AudioClip*>(activeTrack->clips.getFirst()))
        {
            adrProcessor.processADRClip(audioClip->audioData);
            statusLabel.setText("ADR Leveling & Match EQ Applied!", juce::dontSendNotification);
            return;
        }
    }

    statusLabel.setText("No ADR Audio Clip found on Track 2.", juce::dontSendNotification);
}
