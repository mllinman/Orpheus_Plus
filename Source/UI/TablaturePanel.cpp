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
    
    tuningSelector.addItem("Standard", 1);
    tuningSelector.addItem("Drop D", 2);
    tuningSelector.addItem("Half-Step Down", 3);
    tuningSelector.addItem("Open G", 4);
    tuningSelector.setSelectedId(1, juce::dontSendNotification);
    
    instrumentSelector.addListener(this);
    tuningSelector.addListener(this);
    generateBtn.addListener(this);
    exportBtn.addListener(this);

    addAndMakeVisible(instrumentSelector);
    addAndMakeVisible(tuningSelector);
    addAndMakeVisible(trackSelector);
    addAndMakeVisible(generateBtn);
    addAndMakeVisible(exportBtn);
    
    progressBar.setPercentageDisplay(true);
    addChildComponent(progressBar); // only visible when converting

    converter.addListener(this);

    populateTrackList();
}

TablaturePanel::~TablaturePanel()
{
    converter.removeListener(this);
    instrumentSelector.removeListener(this);
    tuningSelector.removeListener(this);
    generateBtn.removeListener(this);
    exportBtn.removeListener(this);
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
    else if (comboBoxThatHasChanged == &tuningSelector)
    {
        int id = tuningSelector.getSelectedId();
        if (id == 1) generator.setTuning(Transcription::Tuning::Standard);
        else if (id == 2) generator.setTuning(Transcription::Tuning::DropD);
        else if (id == 3) generator.setTuning(Transcription::Tuning::HalfStepDown);
        else if (id == 4) generator.setTuning(Transcription::Tuning::OpenG);
        
        // Re-generate if we have tabs
        if (!currentTabs.empty()) {
            transcribeSelectedTrack();
        }
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
    else if (button == &exportBtn)
    {
        exportToAscii();
    }
}

void TablaturePanel::transcribeSelectedTrack() {
    int trackIndex = trackSelector.getSelectedId() - 1;
    if (trackIndex < 0 || trackIndex >= audioEngine.getNumTracks()) return;

    auto& track = audioEngine.getTrackInfo(trackIndex);
    
    if (track.type == OrpheusTrackInfo::Type::Midi) {
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
    else if (track.type == OrpheusTrackInfo::Type::Audio) {
        juce::File audioFile;
        for (auto* clip : track.clips) {
            if (auto* audioClip = dynamic_cast<AudioClip*>(clip)) {
                audioFile = audioClip->sourceFile;
                break;
            }
        }
        
        if (audioFile.existsAsFile()) {
            progressBar.setVisible(true);
            converter.setMode(AudioToMidiConverter::Mode::Polyphonic);
            converter.convert(audioFile);
        }
    }
}

void TablaturePanel::exportToAscii()
{
    if (currentTabs.empty()) return;

    juce::StringArray lines;
    int numStrings = generator.getNumStrings();
    for (int i = 0; i < numStrings; ++i) {
        lines.add(generator.getStringName(i) + "|--");
    }
    
    float currentBeat = -1.0f;
    for (const auto& note : currentTabs) {
        if (note.beatPosition > currentBeat + 0.1f) {
            // move forward
            for (int i = 0; i < numStrings; ++i) lines.getReference(i) += "-";
            currentBeat = note.beatPosition;
        }
        
        // PAD dashes
        for (int i = 0; i < numStrings; ++i) {
            if (i == note.stringIndex) lines.getReference(i) += juce::String(note.fret) + "-";
            else lines.getReference(i) += (note.fret >= 10 ? "--" : "-");
        }
    }
    
    juce::String fullText = lines.joinIntoString("\n");
    juce::SystemClipboard::copyTextToClipboard(fullText);
    
    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, 
        "Export Successful", "ASCII Tablature has been copied to your clipboard!\n\n" + fullText);
}

void TablaturePanel::conversionProgress(float p) {
    currentProgress = p;
    // progressBar automatically repaints when its associated double changes.
}

void TablaturePanel::conversionComplete(const AudioToMidiResult& result) {
    progressBar.setVisible(false);
    
    std::vector<std::pair<int, float>> extractedNotes;
    if (result.midiFile.getNumTracks() > 0) {
        auto* track = result.midiFile.getTrack(0);
        for (int i = 0; i < track->getNumEvents(); ++i) {
            auto& e = track->getEventPointer(i)->message;
            if (e.isNoteOn()) {
                // In basic pitch, time is often embedded in events. We use the raw tick for now or approximate
                extractedNotes.push_back({e.getNoteNumber(), (float)e.getTimeStamp()});
            }
        }
    }
    
    currentTabs = generator.generateTabs(extractedNotes);
    repaint();
}

void TablaturePanel::conversionFailed(const juce::String& error) {
    progressBar.setVisible(false);
    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Conversion Failed", error);
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
    
    exportBtn.setBounds(header.removeFromRight(120).reduced(5));
    generateBtn.setBounds(header.removeFromRight(150).reduced(5));
    trackSelector.setBounds(header.removeFromRight(150).reduced(5));
    tuningSelector.setBounds(header.removeFromRight(120).reduced(5));
    instrumentSelector.setBounds(header.removeFromRight(120).reduced(5));
    
    if (progressBar.isVisible()) {
        progressBar.setBounds(getLocalBounds().withSizeKeepingCentre(300, 20));
    }
}

