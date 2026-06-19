#pragma once
#include <JuceHeader.h>
#include "../Transcription/TablatureGenerator.h"
#include "../AudioToMidi/AudioToMidiConverter.h"

class AudioEngine;

class TablaturePanel : public juce::Component,
                       public juce::ComboBox::Listener,
                       public juce::Button::Listener,
                       public AudioToMidiConverter::Listener
{
public:
    TablaturePanel(AudioEngine& engine);
    ~TablaturePanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;
    void buttonClicked(juce::Button* button) override;

    void conversionProgress(float progress) override;
    void conversionComplete(const AudioToMidiResult& result) override;
    void conversionFailed(const juce::String& error) override;

private:
    AudioEngine& audioEngine;
    Transcription::TablatureGenerator generator;
    AudioToMidiConverter converter;
    std::vector<Transcription::TabNote> currentTabs;

    juce::ComboBox instrumentSelector;
    juce::ComboBox tuningSelector;
    juce::ComboBox trackSelector;
    juce::TextButton generateBtn{"Transcribe Track"};
    juce::TextButton exportBtn{"Export to Text"};
    
    double currentProgress { 0.0 };
    juce::ProgressBar progressBar{currentProgress};

    void populateTrackList();
    void transcribeSelectedTrack();
    void exportToAscii();
};
