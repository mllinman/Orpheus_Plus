#pragma once
#include <JuceHeader.h>
#include "../Transcription/TablatureGenerator.h"

class AudioEngine;

class TablaturePanel : public juce::Component,
                       public juce::ComboBox::Listener,
                       public juce::Button::Listener
{
public:
    TablaturePanel(AudioEngine& engine);
    ~TablaturePanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;
    void buttonClicked(juce::Button* button) override;

private:
    AudioEngine& audioEngine;
    Transcription::TablatureGenerator generator;
    std::vector<Transcription::TabNote> currentTabs;

    juce::ComboBox instrumentSelector;
    juce::ComboBox trackSelector;
    juce::TextButton generateBtn{"Transcribe Track"};

    void populateTrackList();
    void transcribeSelectedTrack();
};
