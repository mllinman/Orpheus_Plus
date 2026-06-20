#pragma once
#include <JuceHeader.h>
#include "../AI/CompositionCoPilot.h"
#include "../Audio/AudioEngine.h"

class AICoPilotPanel : public juce::Component
{
public:
    AICoPilotPanel(AudioEngine& engine);
    ~AICoPilotPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    AudioEngine& audioEngine;
    CompositionCoPilot coPilot;

    // UI elements
    juce::Label titleLabel { "Title", "AI Composition Co-Pilot" };
    
    juce::Label genreLabel { "GenreLbl", "Genre:" };
    juce::TextEditor genreInput;

    juce::Label styleLabel { "StyleLbl", "Style:" };
    juce::TextEditor styleInput;

    juce::TextButton generateChordsBtn { "Generate Progression" };
    juce::TextButton autocompleteBtn { "Autocomplete Melody" };
    juce::TextButton extractRhythmBtn { "Extract Rhythm" };

    // Placeholder actions
    void onGenerateChords();
    void onAutocompleteMelody();
    void onExtractRhythm();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AICoPilotPanel)
};
