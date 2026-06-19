#include "TablaturePanel.h"
#include "OrpheusLookAndFeel.h"
#include "../Audio/AudioEngine.h"

TablaturePanel::TablaturePanel(AudioEngine& engine)
    : audioEngine(engine)
{
    instrumentSelector.addItem("Acoustic Guitar", 1);
    instrumentSelector.addItem("Electric Guitar", 2);
    instrumentSelector.addItem("Bass", 3);
    instrumentSelector.setSelectedId(1, juce::dontSendNotification);
    
    instrumentSelector.addListener(this);
    generateBtn.addListener(this);

    addAndMakeVisible(instrumentSelector);
    addAndMakeVisible(trackSelector);
    addAndMakeVisible(generateBtn);

    populateTrackList();
}

TablaturePanel::~TablaturePanel()
{
    instrumentSelector.removeListener(this);
    generateBtn.removeListener(this);
}

void TablaturePanel::populateTrackList()
{
    trackSelector.clear();
    int numTracks = audioEngine.getNumTracks();
    int validId = 1;
    for (int i = 0; i < numTracks; ++i)
    {
        auto& track = audioEngine.getTrackInfo(i);
        // We only allow transcribing Audio or Midi tracks
        if (track.type == OrpheusTrackInfo::Type::Audio || track.type == OrpheusTrackInfo::Type::Midi)
        {
            trackSelector.addItem(track.name + (track.type == OrpheusTrackInfo::Type::Audio ? " (Audio)" : " (MIDI)"), i + 1);
            if (validId == 1) trackSelector.setSelectedId(i + 1, juce::dontSendNotification);
            validId++;
        }
    }
}

void TablaturePanel::comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == &instrumentSelector)
    {
        int id = instrumentSelector.getSelectedId();
        if (id == 1) generator.setInstrument(Transcription::InstrumentType::AcousticGuitar);
        else if (id == 2) generator.setInstrument(Transcription::InstrumentType::ElectricGuitar);
        else if (id == 3) generator.setInstrument(Transcription::InstrumentType::Bass);
        currentTabs.clear();
        repaint();
    }
}

void TablaturePanel::buttonClicked(juce::Button* button)
{
    if (button == &generateBtn)
    {
        transcribeSelectedTrack();
        repaint();
    }
}

void TablaturePanel::transcribeSelectedTrack() {
    int trackIndex = trackSelector.getSelectedId() - 1;
    if (trackIndex < 0 || trackIndex >= audioEngine.getNumTracks()) return;

    auto& track = audioEngine.getTrackInfo(trackIndex);
    std::vector<std::pair<int, float>> extractedNotes;

    for (auto* clip : track.clips)
    {
        if (auto* midiClip = dynamic_cast<MidiClip*>(clip))
        {
            auto& seq = midiClip->midiData;
            for (int i = 0; i < seq.getNumEvents(); ++i) {
                auto* event = seq.getEventPointer(i);
                if (event->message.isNoteOn()) {
                    extractedNotes.push_back({event->message.getNoteNumber(), (float)event->message.getTimeStamp()});
                }
            }
        }
    }

    currentTabs = generator.generateTabs(extractedNotes);
}

void TablaturePanel::paint(juce::Graphics& g)
{
    g.fillAll(OrpheusLookAndFeel::bgDarkest());
    
    // Header
    auto headerArea = getLocalBounds().removeFromTop(50);
    g.setColour(OrpheusLookAndFeel::bgPanel().brighter(0.1f));
    g.fillRect(headerArea);
    g.setColour(OrpheusLookAndFeel::textPrimary());
    g.setFont(18.0f);
    g.drawText("TABLATURE GENERATOR", headerArea.withLeft(20), juce::Justification::centredLeft);

    auto area = getLocalBounds().removeFromBottom(getHeight() - 50);

    // Draw Tab Lines
    area.reduce(20, 20);
    int numStrings = generator.getNumStrings();
    if (numStrings == 0) return;

    float lineSpacing = 20.0f;
    float tabHeight = (numStrings - 1) * lineSpacing;
    float startY = area.getY() + (area.getHeight() - tabHeight) / 2.0f;

    g.setColour(juce::Colours::grey);
    for (int i = 0; i < numStrings; ++i) {
        float y = startY + i * lineSpacing;
        g.drawLine(area.getX(), y, area.getRight(), y, 2.0f);
        
        // Draw string name
        g.setColour(OrpheusLookAndFeel::textSecondary());
        g.setFont(12.0f);
        g.drawText(generator.getStringName(i), area.getX() - 20, y - 10, 20, 20, juce::Justification::centred);
        g.setColour(juce::Colours::grey);
    }

    // Draw Notes
    if (currentTabs.empty()) {
        g.setColour(OrpheusLookAndFeel::textSecondary());
        g.setFont(16.0f);
        g.drawText("Select a track and click 'Transcribe Track'...", area, juce::Justification::centred);
        return;
    }

    g.setColour(OrpheusLookAndFeel::accentPrimary());
    g.setFont(14.0f);
    
    float pixelsPerBeat = 60.0f;
    float startX = area.getX() + 40.0f;

    for (const auto& note : currentTabs) {
        float x = startX + note.beatPosition * pixelsPerBeat;
        float y = startY + note.stringIndex * lineSpacing;

        // Draw background circle for the number
        g.setColour(OrpheusLookAndFeel::bgDarkest());
        g.fillEllipse(x - 10, y - 10, 20, 20);

        // Draw fret number
        g.setColour(OrpheusLookAndFeel::accentPrimary());
        g.drawText(juce::String(note.fret), x - 10, y - 10, 20, 20, juce::Justification::centred);
    }
}

void TablaturePanel::resized()
{
    auto area = getLocalBounds();
    auto header = area.removeFromTop(50);
    
    generateBtn.setBounds(header.removeFromRight(150).reduced(5));
    trackSelector.setBounds(header.removeFromRight(200).reduced(5));
    instrumentSelector.setBounds(header.removeFromRight(150).reduced(5));
}
