#include "DistributionPrepPanel.h"
#include "../MainComponent.h"
#include "../Audio/AudioEngine.h"
#include "../Util/OrpheusLogger.h"
#include "OrpheusLookAndFeel.h"

DistributionPrepPanel::DistributionPrepPanel(AudioEngine& e, AppState& s, MainComponent* mc)
    : audioEngine(e), appState(s), mainComponent(mc)
{
    //── Platform selector ────────────────────────────────────────────────────
    platformCombo.addItem("Spotify",     1);
    platformCombo.addItem("Apple Music", 2);
    platformCombo.addItem("YouTube",     3);
    platformCombo.addItem("Generic",     4);
    platformCombo.setSelectedId(1, juce::dontSendNotification);
    platformCombo.onChange = [this]() { updateSpecDisplay(getSelectedPlatform()); };
    addAndMakeVisible(platformCombo);

    platformLabel.setText("Platform", juce::dontSendNotification);
    platformLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(platformLabel);

    //── Metadata fields ──────────────────────────────────────────────────────
    titleField.setTextToShowWhenEmpty("Track Title", juce::Colours::grey);
    addAndMakeVisible(titleField);
    titleLabel.setText("Title", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(titleLabel);

    artistField.setTextToShowWhenEmpty("Artist Name", juce::Colours::grey);
    addAndMakeVisible(artistField);
    artistLabel.setText("Artist", juce::dontSendNotification);
    artistLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(artistLabel);

    stripAllToggle.setToggleState(true, juce::dontSendNotification);
    addAndMakeVisible(stripAllToggle);

    //── Spec display labels ──────────────────────────────────────────────────
    auto setupSpecLabel = [this](juce::Label& label) {
        label.setJustificationType(juce::Justification::centred);
        label.setFont(juce::Font(13.0f));
        addAndMakeVisible(label);
    };

    setupSpecLabel(lufsDisplay);
    setupSpecLabel(truePeakDisplay);
    setupSpecLabel(sampleRateDisplay);
    setupSpecLabel(bitDepthDisplay);

    updateSpecDisplay(DistributionPrepProcessor::Platform::Spotify);

    //── Buttons ──────────────────────────────────────────────────────────────
    prepareButton.onClick = [this]()
    {
        if (isProcessing || !lastDroppedFile.existsAsFile()) return;

        isProcessing = true;
        statusMessage = "Processing...";
        repaint();

        juce::Thread::launch([this]()
        {
            auto platform = getSelectedPlatform();
            juce::String title = titleField.getText();
            juce::String artist = artistField.getText();

            juce::File outputFile = lastDroppedFile.getParentDirectory().getChildFile(
                lastDroppedFile.getFileNameWithoutExtension() + "_dist_ready.wav");

            bool success = processor.processForDistribution(
                lastDroppedFile, outputFile, platform,
                stripAllToggle.getToggleState() ? "" : title,
                stripAllToggle.getToggleState() ? "" : artist,
                &appState, &audioEngine);

            // If not stripping all, write clean metadata after
            if (success && !stripAllToggle.getToggleState() && (title.isNotEmpty() || artist.isNotEmpty()))
            {
                DistributionPrepProcessor::writeCleanMetadata(outputFile, title, artist);
            }

            juce::MessageManager::callAsync([this, success, outputFile]()
            {
                isProcessing = false;
                statusMessage = success
                    ? "Done! Saved: " + outputFile.getFileName()
                    : "Error during processing.";
                repaint();
            });
        });
    };
    addAndMakeVisible(prepareButton);

    analyzeButton.onClick = [this]()
    {
        if (!lastDroppedFile.existsAsFile()) return;

        statusMessage = "Analyzing...";
        repaint();

        juce::Thread::launch([this]()
        {
            float lufs = DistributionPrepProcessor::measureIntegratedLUFS(lastDroppedFile);
            float tp = DistributionPrepProcessor::measureTruePeak(lastDroppedFile);

            juce::MessageManager::callAsync([this, lufs, tp]()
            {
                statusMessage = "LUFS: " + juce::String(lufs, 1) + " | True Peak: " + juce::String(tp, 1) + " dBTP";
                repaint();
            });
        });
    };
    addAndMakeVisible(analyzeButton);

    startTimerHz(30);
}

DistributionPrepPanel::~DistributionPrepPanel()
{
    stopTimer();
}

void DistributionPrepPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff121212));

    auto bounds = getLocalBounds().reduced(16);

    // Glassmorphic card background
    g.setColour(juce::Colour(0x20ffffff));
    g.fillRoundedRectangle(bounds.toFloat(), 12.0f);
    g.setColour(juce::Colour(0x30ffffff));
    g.drawRoundedRectangle(bounds.toFloat(), 12.0f, 1.0f);

    // Header
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(20.0f).boldened());
    g.drawText(juce::CharPointer_UTF8("\xe2\x9a\xa1 Distribution Prep"),
               bounds.removeFromTop(36), juce::Justification::centred);

    // Subtitle
    g.setFont(12.0f);
    g.setColour(juce::Colours::lightgrey);
    g.drawText("Strip metadata \u2022 Normalize loudness \u2022 Platform compliance",
               bounds.removeFromTop(20), juce::Justification::centred);

    bounds.removeFromTop(8);

    // Spec cards
    auto specArea = bounds.removeFromTop(60);
    int cardWidth = specArea.getWidth() / 4;

    auto paintSpecCard = [&](juce::Rectangle<int> area, const juce::String& label, const juce::String& value)
    {
        auto card = area.reduced(4);
        g.setColour(juce::Colour(0x18ffffff));
        g.fillRoundedRectangle(card.toFloat(), 8.0f);

        g.setColour(juce::Colours::grey);
        g.setFont(10.0f);
        g.drawText(label, card.removeFromTop(20), juce::Justification::centred);

        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(14.0f).boldened());
        g.drawText(value, card, juce::Justification::centred);
    };

    auto spec = DistributionPrepProcessor::getSpec(getSelectedPlatform());
    paintSpecCard(specArea.removeFromLeft(cardWidth), "TARGET LUFS", juce::String(spec.targetLUFS, 1));
    paintSpecCard(specArea.removeFromLeft(cardWidth), "TRUE PEAK", juce::String(spec.truePeakLimit, 1) + " dBTP");
    paintSpecCard(specArea.removeFromLeft(cardWidth), "SAMPLE RATE", juce::String(spec.sampleRate / 1000.0, 1) + " kHz");
    paintSpecCard(specArea.removeFromLeft(cardWidth), "BIT DEPTH", juce::String(spec.bitDepth) + "-bit");

    // Drop zone
    auto dropZone = getLocalBounds().reduced(16);
    dropZone = dropZone.removeFromBottom(80).reduced(20, 0);

    g.setColour(isDragHover ? juce::Colour(0x40a29bfe) : juce::Colour(0x15ffffff));
    g.fillRoundedRectangle(dropZone.toFloat(), 8.0f);

    // Dashed border
    g.setColour(isDragHover ? juce::Colour(0xffa29bfe) : juce::Colour(0x40ffffff));
    float dashLengths[] = { 6.0f, 4.0f };
    g.drawDashedLine(juce::Line<float>((float)dropZone.getX(), (float)dropZone.getY(),
                                        (float)dropZone.getRight(), (float)dropZone.getY()),
                     dashLengths, 2, 1.0f);
    g.drawDashedLine(juce::Line<float>((float)dropZone.getX(), (float)dropZone.getBottom(),
                                        (float)dropZone.getRight(), (float)dropZone.getBottom()),
                     dashLengths, 2, 1.0f);

    g.setColour(juce::Colours::lightgrey);
    g.setFont(13.0f);

    if (isProcessing)
    {
        g.setColour(juce::Colour(0xffa29bfe));
        g.drawText(juce::CharPointer_UTF8("\xe2\x8f\xb3 Processing... Please wait"), dropZone, juce::Justification::centred);
    }
    else if (lastDroppedFile.existsAsFile())
    {
        g.drawText(juce::CharPointer_UTF8("\xf0\x9f\x93\x81 ") + lastDroppedFile.getFileName(), dropZone.removeFromTop(30), juce::Justification::centred);
        if (statusMessage.isNotEmpty())
        {
            g.setColour(juce::Colour(0xff4ecdc4));
            g.setFont(11.0f);
            g.drawText(statusMessage, dropZone, juce::Justification::centred);
        }
    }
    else
    {
        g.drawText("Drag & Drop a .wav file here", dropZone, juce::Justification::centred);
    }
}

void DistributionPrepPanel::resized()
{
    auto bounds = getLocalBounds().reduced(16);
    bounds.removeFromTop(56);  // Header + subtitle
    bounds.removeFromTop(8);
    bounds.removeFromTop(60);  // Spec cards (painted manually)
    bounds.removeFromTop(8);

    // Platform selector row
    auto platformRow = bounds.removeFromTop(30);
    platformLabel.setBounds(platformRow.removeFromLeft(70));
    platformCombo.setBounds(platformRow.removeFromLeft(160));
    platformRow.removeFromLeft(20);
    stripAllToggle.setBounds(platformRow.removeFromLeft(180));

    bounds.removeFromTop(8);

    // Metadata fields row
    auto metaRow1 = bounds.removeFromTop(28);
    titleLabel.setBounds(metaRow1.removeFromLeft(50));
    metaRow1.removeFromLeft(4);
    titleField.setBounds(metaRow1.removeFromLeft(200));
    metaRow1.removeFromLeft(16);
    artistLabel.setBounds(metaRow1.removeFromLeft(50));
    metaRow1.removeFromLeft(4);
    artistField.setBounds(metaRow1.removeFromLeft(200));

    bounds.removeFromTop(12);

    // Buttons row
    auto btnRow = bounds.removeFromTop(36);
    int btnWidth = 200;
    int totalBtnWidth = btnWidth * 2 + 16;
    auto centeredBtns = btnRow.withSizeKeepingCentre(totalBtnWidth, 36);
    analyzeButton.setBounds(centeredBtns.removeFromLeft(btnWidth));
    centeredBtns.removeFromLeft(16);
    prepareButton.setBounds(centeredBtns.removeFromLeft(btnWidth));
}

void DistributionPrepPanel::timerCallback()
{
    repaint();
}

bool DistributionPrepPanel::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (auto& file : files)
    {
        if (file.endsWithIgnoreCase(".wav") || file.endsWithIgnoreCase(".aiff") ||
            file.endsWithIgnoreCase(".flac") || file.endsWithIgnoreCase(".mp3"))
            return true;
    }
    return false;
}

void DistributionPrepPanel::filesDropped(const juce::StringArray& files, int /*x*/, int /*y*/)
{
    isDragHover = false;

    if (isProcessing) return;

    for (auto& path : files)
    {
        juce::File file(path);
        if (file.existsAsFile() && (file.hasFileExtension("wav") || file.hasFileExtension("aiff") ||
                                     file.hasFileExtension("flac") || file.hasFileExtension("mp3")))
        {
            lastDroppedFile = file;
            statusMessage = "";
            repaint();
            break;
        }
    }
}

void DistributionPrepPanel::updateSpecDisplay(DistributionPrepProcessor::Platform platform)
{
    auto spec = DistributionPrepProcessor::getSpec(platform);
    lufsDisplay.setText("LUFS: " + juce::String(spec.targetLUFS, 1), juce::dontSendNotification);
    truePeakDisplay.setText("TP: " + juce::String(spec.truePeakLimit, 1) + " dBTP", juce::dontSendNotification);
    sampleRateDisplay.setText(juce::String(spec.sampleRate / 1000.0, 1) + " kHz", juce::dontSendNotification);
    bitDepthDisplay.setText(juce::String(spec.bitDepth) + "-bit", juce::dontSendNotification);
    repaint();
}

DistributionPrepProcessor::Platform DistributionPrepPanel::getSelectedPlatform() const
{
    switch (platformCombo.getSelectedId())
    {
        case 1: return DistributionPrepProcessor::Platform::Spotify;
        case 2: return DistributionPrepProcessor::Platform::AppleMusic;
        case 3: return DistributionPrepProcessor::Platform::YouTube;
        case 4: return DistributionPrepProcessor::Platform::Generic;
        default: return DistributionPrepProcessor::Platform::Spotify;
    }
}
