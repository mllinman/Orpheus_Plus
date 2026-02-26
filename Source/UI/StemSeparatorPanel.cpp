#include "StemSeparatorPanel.h"

StemSeparatorPanel::StemSeparatorPanel(AudioEngine& e, AppState& s)
    : audioEngine(e), appState(s)
{
    // Model selector
    modelLabel.setFont(juce::Font(9.0f).boldened());
    modelLabel.setColour(juce::Label::textColourId, OrpheusLookAndFeel::textMuted());
    modelCombo.addItem("Demucs 4", 1);
    modelCombo.addItem("Demucs 4 HQ", 2);
    modelCombo.addItem("Spleeter 2-Stem", 3);
    modelCombo.addItem("Spleeter 4-Stem", 4);
    modelCombo.addItem("Spleeter 5-Stem", 5);
    modelCombo.addItem("Open-Unmix (UMX)", 6);
    modelCombo.addItem("UVR (MDX-Net)", 7);
    modelCombo.setSelectedId(1, juce::dontSendNotification);
    addAndMakeVisible(modelLabel);
    addAndMakeVisible(modelCombo);

    // Browse
    browseButton.setButtonText("Browse...");
    browseButton.setColour(juce::TextButton::buttonColourId, OrpheusLookAndFeel::bgElevated());
    browseButton.onClick = [this] {
        auto chooser = std::make_shared<juce::FileChooser>("Select audio file...",
            juce::File{}, "*.wav;*.mp3;*.flac;*.aiff;*.ogg");
        chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this, chooser](const juce::FileChooser&) {
                auto file = chooser->getResult();
                if (file.existsAsFile()) {
                    selectedFile = file;
                    repaint();
                }
            });
    };
    addAndMakeVisible(browseButton);

    // Quality
    qualityLabel.setFont(juce::Font(9.0f).boldened());
    qualityLabel.setColour(juce::Label::textColourId, OrpheusLookAndFeel::textMuted());
    highQualityToggle.setClickingTogglesState(true);
    addAndMakeVisible(qualityLabel);
    addAndMakeVisible(highQualityToggle);

    // Action buttons
    separateButton.setButtonText("Start Separation");
    separateButton.setColour(juce::TextButton::buttonColourId, OrpheusLookAndFeel::accentPrimary());
    separateButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    separateButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    separateButton.onClick = [this] {
        if (selectedFile.existsAsFile())
            startSeparation(selectedFile);
    };
    cancelButton.setButtonText("Cancel");
    cancelButton.setColour(juce::TextButton::buttonColourId, OrpheusLookAndFeel::accentDanger());
    cancelButton.setVisible(false);
    addAndMakeVisible(separateButton);
    addAndMakeVisible(cancelButton);

    // Stem result cards
    for (int i = 0; i < 6; ++i) {
        stemCards[i].soloButton.setColour(juce::TextButton::buttonColourId, OrpheusLookAndFeel::bgDark());
        stemCards[i].muteButton.setColour(juce::TextButton::buttonColourId, OrpheusLookAndFeel::bgDark());
        stemCards[i].volumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        stemCards[i].volumeSlider.setRange(0.0, 1.0, 0.01);
        stemCards[i].volumeSlider.setValue(1.0, juce::dontSendNotification);
        stemCards[i].volumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        stemCards[i].addToTimeline.setColour(juce::TextButton::buttonColourId, OrpheusLookAndFeel::accentPrimary().darker(0.3f));
        addAndMakeVisible(stemCards[i].soloButton);
        addAndMakeVisible(stemCards[i].muteButton);
        addAndMakeVisible(stemCards[i].volumeSlider);
        addAndMakeVisible(stemCards[i].addToTimeline);
        // Hide until results are available
        stemCards[i].soloButton.setVisible(false);
        stemCards[i].muteButton.setVisible(false);
        stemCards[i].volumeSlider.setVisible(false);
        stemCards[i].addToTimeline.setVisible(false);
    }

    stemSeparator.addListener(this);
    startTimerHz(15);
}

StemSeparatorPanel::~StemSeparatorPanel() { 
    stemSeparator.removeListener(this);
    stopTimer(); 
}

void StemSeparatorPanel::startSeparation(const juce::File& file)
{
    isProcessing = true;
    hasResult = false;
    currentProgress = 0.0f;
    errorMessage.clear();
    separateButton.setVisible(false);
    cancelButton.setVisible(true);

    switch (modelCombo.getSelectedId())
    {
        case 1: stemSeparator.setModel(StemSeparator::Model::Demucs4); break;
        case 2: stemSeparator.setModel(StemSeparator::Model::Demucs4HQ); break;
        case 3: stemSeparator.setModel(StemSeparator::Model::Spleeter2); break;
        case 4: stemSeparator.setModel(StemSeparator::Model::Spleeter4); break;
        case 5: stemSeparator.setModel(StemSeparator::Model::Spleeter5); break;
        case 6: stemSeparator.setModel(StemSeparator::Model::OpenUnmix); break;
        case 7: stemSeparator.setModel(StemSeparator::Model::UVR_MDXNet); break;
    }

    stemSeparator.separate(file, appState, nullptr);
    repaint();
}

void StemSeparatorPanel::stemSeparationProgress(float progress)
{
    currentProgress = progress;
}

void StemSeparatorPanel::stemSeparationComplete(const StemSeparationResult& result)
{
    lastResult = result;
    hasResult = true;
    isProcessing = false;
    separateButton.setVisible(true);
    cancelButton.setVisible(false);

    // Show stem cards
    for (int i = 0; i < 6; ++i) {
        stemCards[i].soloButton.setVisible(true);
        stemCards[i].muteButton.setVisible(true);
        stemCards[i].volumeSlider.setVisible(true);
        stemCards[i].addToTimeline.setVisible(true);
    }
}

void StemSeparatorPanel::stemSeparationFailed(const juce::String& error)
{
    errorMessage = error;
    isProcessing = false;
    separateButton.setVisible(true);
    cancelButton.setVisible(false);
}

bool StemSeparatorPanel::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (auto& f : files)
        if (f.endsWithIgnoreCase(".wav") || f.endsWithIgnoreCase(".mp3") ||
            f.endsWithIgnoreCase(".flac") || f.endsWithIgnoreCase(".aiff"))
            return true;
    return false;
}

void StemSeparatorPanel::filesDropped(const juce::StringArray& files, int, int)
{
    dragHover = false;
    for (auto& f : files) {
        juce::File file(f);
        if (file.existsAsFile()) {
            selectedFile = file;
            repaint();
            break;
        }
    }
}

void StemSeparatorPanel::timerCallback() { if (isProcessing) repaint(); }

void StemSeparatorPanel::paintDropZone(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    auto col = dragHover ? OrpheusLookAndFeel::accentPrimary().withAlpha(0.3f)
                         : OrpheusLookAndFeel::bgDark();
    g.setColour(col);
    g.fillRoundedRectangle(bounds.toFloat(), 8.0f);

    // Dashed border
    g.setColour(dragHover ? OrpheusLookAndFeel::accentPrimary()
                          : OrpheusLookAndFeel::borderDefault());
    float dashLengths[] = { 6.0f, 4.0f };
    g.drawDashedLine(juce::Line<float>((float)bounds.getX() + 8, (float)bounds.getY(),
                                        (float)bounds.getRight() - 8, (float)bounds.getY()),
                     dashLengths, 2, 1.5f);
    g.drawDashedLine(juce::Line<float>((float)bounds.getX() + 8, (float)bounds.getBottom(),
                                        (float)bounds.getRight() - 8, (float)bounds.getBottom()),
                     dashLengths, 2, 1.5f);

    if (selectedFile.existsAsFile()) {
        g.setColour(OrpheusLookAndFeel::textPrimary());
        g.setFont(juce::Font(13.0f).boldened());
        g.drawText(selectedFile.getFileName(), bounds, juce::Justification::centred);
        g.setColour(OrpheusLookAndFeel::textMuted());
        g.setFont(juce::Font(10.0f));
        g.drawText("Drop a new file to replace", bounds.withTrimmedTop(24), juce::Justification::centred);
    } else {
        g.setColour(OrpheusLookAndFeel::textSecondary());
        g.setFont(juce::Font(14.0f));
        g.drawText("Drop audio file here", bounds.withTrimmedBottom(16), juce::Justification::centred);
        g.setColour(OrpheusLookAndFeel::textMuted());
        g.setFont(juce::Font(10.0f));
        g.drawText("or use Browse button below", bounds.withTrimmedTop(16), juce::Justification::centred);
    }
}

void StemSeparatorPanel::paintProgressRing(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    auto center = bounds.getCentre().toFloat();
    float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.35f;

    // Background ring
    g.setColour(OrpheusLookAndFeel::bgDark());
    juce::Path bgRing;
    bgRing.addCentredArc(center.x, center.y, radius, radius, 0.0f,
                          0.0f, juce::MathConstants<float>::twoPi, true);
    g.strokePath(bgRing, juce::PathStrokeType(6.0f));

    // Progress arc
    float angle = currentProgress * juce::MathConstants<float>::twoPi;
    g.setColour(OrpheusLookAndFeel::accentPrimary());
    juce::Path progressArc;
    progressArc.addCentredArc(center.x, center.y, radius, radius, 0.0f,
                               -juce::MathConstants<float>::halfPi,
                               -juce::MathConstants<float>::halfPi + angle, true);
    g.strokePath(progressArc, juce::PathStrokeType(6.0f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));

    // Percentage text
    g.setColour(OrpheusLookAndFeel::textPrimary());
    g.setFont(juce::Font(20.0f).boldened());
    g.drawText(juce::String((int)(currentProgress * 100)) + "%",
               bounds, juce::Justification::centred);
}

void StemSeparatorPanel::paint(juce::Graphics& g)
{
    g.fillAll(OrpheusLookAndFeel::bgPanel());

    // Header
    auto header = getLocalBounds().removeFromTop(40);
    g.setGradientFill(juce::ColourGradient(
        OrpheusLookAndFeel::bgSurface(), 0, 0,
        OrpheusLookAndFeel::bgPanel(), 0, 40.0f, false));
    g.fillRect(header);
    g.setColour(OrpheusLookAndFeel::textPrimary());
    g.setFont(juce::Font(14.0f).boldened());
    g.drawText("STEM SEPARATOR", header.reduced(16, 0), juce::Justification::centredLeft);

    auto area = getLocalBounds().reduced(16).withTrimmedTop(40);

    // Drop zone
    auto dropArea = area.removeFromTop(80);
    paintDropZone(g, dropArea);

    area.removeFromTop(12);

    // Progress ring when processing
    if (isProcessing) {
        auto progressArea = area.removeFromTop(160);
        paintProgressRing(g, progressArea);
        g.setColour(OrpheusLookAndFeel::textSecondary());
        g.setFont(juce::Font(11.0f));
        g.drawText("Separating stems...", progressArea.withTrimmedTop(120),
                   juce::Justification::centredTop);
    }

    // Error message
    if (errorMessage.isNotEmpty()) {
        g.setColour(OrpheusLookAndFeel::accentDanger());
        g.setFont(juce::Font(11.0f));
        g.drawText(errorMessage, area.removeFromTop(30), juce::Justification::centred);
    }

    // Stem result cards
    if (hasResult) {
        area.removeFromTop(8);
        g.setColour(OrpheusLookAndFeel::textMuted());
        g.setFont(juce::Font(10.0f).boldened());
        g.drawText("SEPARATED STEMS", area.removeFromTop(18).reduced(4, 0), juce::Justification::centredLeft);

        const juce::File stems[6] = { lastResult.vocals, lastResult.drums, lastResult.bass,
                                       lastResult.guitar, lastResult.piano, lastResult.other };
        int cardH = 40;
        for (int i = 0; i < 6; ++i) {
            auto cardBounds = area.removeFromTop(cardH);
            paintStemCard(g, cardBounds, stemNames[i], stems[i], i);
            area.removeFromTop(4);
        }
    }
}

void StemSeparatorPanel::paintStemCard(juce::Graphics& g, juce::Rectangle<int> bounds,
                                        const juce::String& name, const juce::File& file, int)
{
    g.setColour(OrpheusLookAndFeel::bgElevated());
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);

    g.setColour(OrpheusLookAndFeel::textPrimary());
    g.setFont(juce::Font(11.0f).boldened());
    g.drawText(name, bounds.withWidth(80).reduced(8, 0), juce::Justification::centredLeft);

    if (file.existsAsFile()) {
        g.setColour(OrpheusLookAndFeel::accentSuccess().withAlpha(0.7f));
        g.fillEllipse((float)(bounds.getX() + 70), (float)(bounds.getCentreY() - 3), 6.0f, 6.0f);
    }
}

void StemSeparatorPanel::resized()
{
    auto area = getLocalBounds().reduced(16).withTrimmedTop(40);
    area.removeFromTop(100); // drop zone + gap

    // Model + quality row
    auto ctrlRow = area.removeFromTop(32);
    modelLabel.setBounds(ctrlRow.removeFromLeft(50));
    modelCombo.setBounds(ctrlRow.removeFromLeft(160).reduced(2));
    qualityLabel.setBounds(ctrlRow.removeFromLeft(60));
    highQualityToggle.setBounds(ctrlRow.removeFromLeft(80).reduced(2));
    area.removeFromTop(4);

    // Browse + Separate row
    auto btnRow = area.removeFromTop(32);
    browseButton.setBounds(btnRow.removeFromLeft(btnRow.getWidth() / 2 - 4).reduced(2));
    separateButton.setBounds(btnRow.reduced(2));
    cancelButton.setBounds(separateButton.getBounds());
    area.removeFromTop(8);

    // Stem cards
    if (hasResult) {
        area.removeFromTop(26); // header
        int cardH = 40;
        for (int i = 0; i < 6; ++i) {
            auto card = area.removeFromTop(cardH);
            auto right = card.removeFromRight(card.getWidth() - 86);
            stemCards[i].soloButton.setBounds(right.removeFromLeft(24).reduced(2));
            stemCards[i].muteButton.setBounds(right.removeFromLeft(24).reduced(2));
            stemCards[i].volumeSlider.setBounds(right.removeFromLeft(100).reduced(2));
            stemCards[i].addToTimeline.setBounds(right.reduced(2));
            area.removeFromTop(4);
        }
    }
}
