#include "AICoPilotPanel.h"
#include "OrpheusLookAndFeel.h"

AICoPilotPanel::AICoPilotPanel(AudioEngine& engine)
    : audioEngine(engine)
{
    addAndMakeVisible(titleLabel);
    titleLabel.setFont(juce::Font(20.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, OrpheusLookAndFeel::textPrimary());
    titleLabel.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(genreLabel);
    genreLabel.setColour(juce::Label::textColourId, OrpheusLookAndFeel::textSecondary());
    addAndMakeVisible(genreInput);
    genreInput.setText("Synthwave");

    addAndMakeVisible(styleLabel);
    styleLabel.setColour(juce::Label::textColourId, OrpheusLookAndFeel::textSecondary());
    addAndMakeVisible(styleInput);
    styleInput.setText("Upbeat, Arpeggiated");

    addAndMakeVisible(generateChordsBtn);
    generateChordsBtn.onClick = [this] { onGenerateChords(); };
    generateChordsBtn.setColour(juce::TextButton::buttonColourId, OrpheusLookAndFeel::accentPrimary());

    addAndMakeVisible(autocompleteBtn);
    autocompleteBtn.onClick = [this] { onAutocompleteMelody(); };
    autocompleteBtn.setColour(juce::TextButton::buttonColourId, OrpheusLookAndFeel::accentSecondary());

    addAndMakeVisible(extractRhythmBtn);
    extractRhythmBtn.onClick = [this] { onExtractRhythm(); };
    extractRhythmBtn.setColour(juce::TextButton::buttonColourId, OrpheusLookAndFeel::accentSecondary());
}

AICoPilotPanel::~AICoPilotPanel()
{
}

void AICoPilotPanel::paint(juce::Graphics& g)
{
    g.fillAll(OrpheusLookAndFeel::bgPanel());

    g.setColour(OrpheusLookAndFeel::borderSubtle());
    g.drawRect(getLocalBounds(), 1);
}

void AICoPilotPanel::resized()
{
    auto area = getLocalBounds().reduced(10);

    titleLabel.setBounds(area.removeFromTop(40));
    area.removeFromTop(10); // spacing

    auto inputArea = area.removeFromTop(30);
    genreLabel.setBounds(inputArea.removeFromLeft(60));
    genreInput.setBounds(inputArea.removeFromLeft(120).reduced(2));
    
    inputArea.removeFromLeft(20); // spacing
    styleLabel.setBounds(inputArea.removeFromLeft(50));
    styleInput.setBounds(inputArea.reduced(2));

    area.removeFromTop(20); // spacing

    int btnHeight = 35;
    generateChordsBtn.setBounds(area.removeFromTop(btnHeight).reduced(0, 2));
    autocompleteBtn.setBounds(area.removeFromTop(btnHeight).reduced(0, 2));
    extractRhythmBtn.setBounds(area.removeFromTop(btnHeight).reduced(0, 2));
}

void AICoPilotPanel::onGenerateChords()
{
    auto genre = genreInput.getText();
    auto style = styleInput.getText();
    
    // Create new chord progression and place it into a new Midi Track
    auto newClip = coPilot.generateProgression(genre, style, 4, audioEngine.getBpm());
    
    int newTrackIdx = audioEngine.addMidiTrack("Co-Pilot: " + genre);
    auto* trackInfo = audioEngine.getTrack(newTrackIdx);
    
    if (trackInfo)
    {
        trackInfo->clips.add(newClip.release());
        // Request UI update via AppState change message in a real environment
    }
}

void AICoPilotPanel::onAutocompleteMelody()
{
    // Normally we'd grab the selected MidiClip.
    // Here we'll just demonstrate taking the first clip of the first MIDI track.
    for (int i = 0; i < audioEngine.getNumTracks(); ++i)
    {
        auto* t = audioEngine.getTrack(i);
        if (t && t->type == OrpheusTrackInfo::Type::Midi && !t->clips.isEmpty())
        {
            if (auto* midiClip = dynamic_cast<MidiClip*>(t->clips[0]))
            {
                auto newClip = coPilot.autocompleteMelody(*midiClip, 4, audioEngine.getBpm());
                t->clips.set(0, newClip.release());
                return;
            }
        }
    }
}

void AICoPilotPanel::onExtractRhythm()
{
    // Normally we'd select an AudioClip and a MidiClip.
    // Demonstrate grabbing first Audio clip and applying to first MIDI clip.
    AudioClip* sourceAudio = nullptr;
    MidiClip* targetMidi = nullptr;

    for (int i = 0; i < audioEngine.getNumTracks(); ++i)
    {
        auto* t = audioEngine.getTrack(i);
        if (t && !t->clips.isEmpty())
        {
            if (t->type == OrpheusTrackInfo::Type::Audio && !sourceAudio)
                sourceAudio = dynamic_cast<AudioClip*>(t->clips[0]);
            else if (t->type == OrpheusTrackInfo::Type::Midi && !targetMidi)
                targetMidi = dynamic_cast<MidiClip*>(t->clips[0]);
        }
    }

    if (sourceAudio && targetMidi)
    {
        coPilot.extractRhythmAndApply(*sourceAudio, *targetMidi);
    }
}
