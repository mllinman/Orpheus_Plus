#include "UserManualPanel.h"

UserManualPanel::UserManualPanel()
{
    populateManualContent();

    topicsList.setModel(this);
    topicsList.setColour(juce::ListBox::backgroundColourId, OrpheusLookAndFeel::bgDark());
    topicsList.setColour(juce::ListBox::outlineColourId, OrpheusLookAndFeel::borderSubtle());
    addAndMakeVisible(topicsList);

    contentEditor.setMultiLine(true);
    contentEditor.setReadOnly(true);
    contentEditor.setScrollbarsShown(true);
    contentEditor.setCaretVisible(false);
    contentEditor.setColour(juce::TextEditor::backgroundColourId, OrpheusLookAndFeel::bgPanel());
    contentEditor.setColour(juce::TextEditor::textColourId, OrpheusLookAndFeel::textPrimary());
    contentEditor.setColour(juce::TextEditor::outlineColourId, OrpheusLookAndFeel::borderSubtle());
    contentEditor.applyFontToAllText(juce::Font(15.0f));
    addAndMakeVisible(contentEditor);

    if (!topics.empty())
    {
        topicsList.selectRow(0);
        loadTopic(0);
    }
}

UserManualPanel::~UserManualPanel()
{
    topicsList.setModel(nullptr);
}

void UserManualPanel::populateManualContent()
{
    topics.push_back({
        "1. Welcome to Orpheus Plus",
        "Orpheus Plus is an advanced AI-powered Digital Audio Workstation (DAW) tailored for vocal production, stem separation, and audio restoration.\n\n"
        "Key Features:\n"
        "- AI Stem Separation: Instantly extract vocals, drums, bass, and other elements from a mixed audio file.\n"
        "- Advanced Pitch Correction: Real-time, transparent AutoTune with scale locking and retune speed control.\n"
        "- Vocal Automation (Timbre Tuning): Record dynamic 'vocality' automation over time, adjusting articulation, pace, resonance, and more.\n"
        "- Audio Cleanup: One-knob neural noise reduction to instantly clean up noisy recordings.\n"
        "- Voice Cloning (Timbre Transfer): Apply the vocal characteristics of a trained model to your performance in real-time.\n"
    });

    topics.push_back({
        "2. Interface Overview",
        "The interface is built using a modern, dockable panel system.\n\n"
        "Central Area:\n"
        "The main tabbed view contains the Timeline (for arrangement), Piano Roll (for MIDI editing), Mastering, Stem Separation, Audio Cleanup, AutoTune, and the User Manual.\n\n"
        "Sidebars:\n"
        "The left sidebar contains track headers. The bottom docks can hold the Mixer, Plugin Workspace, Vocal Automation, or other dockable panels.\n\n"
        "To customize your layout, you can drag tabs out into floating windows or dock them to the edges of the screen."
    });

    topics.push_back({
        "3. AutoTune & Pitch Correction",
        "The AutoTune panel provides real-time pitch correction for vocal tracks.\n\n"
        "How to use:\n"
        "1. Select a vocal track.\n"
        "2. Open the 'AutoTune' tab.\n"
        "3. Set the 'Key' and 'Scale' to match your song. This forces the pitch correction to snap to valid notes.\n"
        "4. Adjust 'Retune Speed': A fast speed (0ms) yields a robotic, hard-tuned effect, while a slower speed yields a natural, transparent correction.\n\n"
        "The panel features a live piano roll visualization showing the detected input pitch (gray) against the corrected output pitch (blue)."
    });

    topics.push_back({
        "4. Vocal Automation (Vocality & Timbre)",
        "The Vocal Automation panel gives you unprecedented control over the 'performance' of a vocal track.\n\n"
        "How to use:\n"
        "1. Play back your track.\n"
        "2. Move any of the Primary or Expression knobs (Pitch, Volume, Tone, Pace, Rhythm, Articulation, Resonance, Inflection, Emphasis, Projection).\n"
        "3. Any adjustments you make while the transport is running are automatically recorded as high-resolution automation data mapped to the playhead.\n\n"
        "Smoothing & Clearing:\n"
        "Use the 'Smooth Automation' button to feather out rigid edits using a moving-average algorithm. Use 'Clear Automation' to remove all automation points for the selected track."
    });

    topics.push_back({
        "5. AI Stem Separation",
        "Extract individual stems from a flattened mix.\n\n"
        "How to use:\n"
        "1. Open the 'Stem Sep' tab.\n"
        "2. Drag and drop an audio file onto the drop zone, or use the 'Load Audio File' button.\n"
        "3. Orpheus Plus will analyze the file and render four draggable stems (Vocals, Drums, Bass, Other).\n"
        "4. You can drag the generated stems directly into the timeline as new audio tracks, or click 'Export Stems' to save them to disk."
    });

    topics.push_back({
        "6. Audio Cleanup (Noise Reduction)",
        "Instantly clean up background noise, hum, or room echo using the Audio Cleanup neural network.\n\n"
        "How to use:\n"
        "1. Open the 'Cleanup' tab.\n"
        "2. Select a noisy audio track.\n"
        "3. Adjust the main rotary dial (Amount). The more you dial it up, the more noise is suppressed.\n"
        "4. The spectrogram visualization updates in real time to show the difference between the dry signal and the cleaned signal."
    });

    topics.push_back({
        "7. Voice Cloning (Timbre Transfer)",
        "Transform your voice into someone else's using ONNX-powered voice conversion models.\n\n"
        "How to use:\n"
        "1. Open the 'Voice Cloning' panel (accessible from the dockable menus).\n"
        "2. Load an RVC or Voice Conversion .onnx model using the 'Load Model' button.\n"
        "3. (Optional) Provide a Speaker Embedding if using a multi-speaker model.\n"
        "4. Adjust the 'Timbre Mix' slider to blend between your natural voice and the cloned voice.\n"
        "5. Speak or sing into your microphone. The AI performs real-time inference to map your pitch and pacing onto the target vocal tone."
    });
}

void UserManualPanel::paint(juce::Graphics& g)
{
    g.fillAll(OrpheusLookAndFeel::bgPanel());

    auto header = getLocalBounds().removeFromTop(44);
    g.setGradientFill(juce::ColourGradient(
        OrpheusLookAndFeel::accentPrimary().withAlpha(0.15f), 0, 0,
        OrpheusLookAndFeel::bgPanel(), 0, 44.0f, false));
    g.fillRect(header);

    g.setColour(OrpheusLookAndFeel::textPrimary());
    g.setFont(juce::Font(15.0f).boldened());
    g.drawText("  USER MANUAL & INFO", header.reduced(16, 0), juce::Justification::centredLeft);

    g.setColour(OrpheusLookAndFeel::borderSubtle());
    g.drawHorizontalLine(44, 0, getWidth());
}

void UserManualPanel::resized()
{
    auto area = getLocalBounds().withTrimmedTop(44);
    topicsList.setBounds(area.removeFromLeft(250).reduced(8));
    contentEditor.setBounds(area.reduced(8));
}

int UserManualPanel::getNumRows()
{
    return (int)topics.size();
}

void UserManualPanel::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    if (rowNumber >= 0 && rowNumber < (int)topics.size())
    {
        if (rowIsSelected)
        {
            g.fillAll(OrpheusLookAndFeel::accentPrimary().withAlpha(0.3f));
            g.setColour(OrpheusLookAndFeel::accentPrimary());
            g.drawRect(0, 0, width, height, 1);
        }

        g.setColour(rowIsSelected ? juce::Colours::white : OrpheusLookAndFeel::textPrimary());
        g.setFont(14.0f);
        g.drawText(topics[(size_t)rowNumber].title, 8, 0, width - 16, height, juce::Justification::centredLeft, true);
    }
}

void UserManualPanel::listBoxItemClicked(int row, const juce::MouseEvent&)
{
    loadTopic(row);
}

void UserManualPanel::loadTopic(int index)
{
    if (index >= 0 && index < (int)topics.size())
    {
        // Add a nice header
        juce::String content = topics[(size_t)index].title.toUpperCase() + "\n";
        content += "=========================================================\n\n";
        content += topics[(size_t)index].content;
        
        contentEditor.setText(content);
        contentEditor.setCaretPosition(0);
    }
}
