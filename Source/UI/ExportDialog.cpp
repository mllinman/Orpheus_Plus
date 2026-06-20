#include "ExportDialog.h"

ExportDialog::ExportDialog(AudioExportManager& exportManager)
    : manager(exportManager)
{
    setOpaque(false);
    setComponentEffect(&shadow);
    shadow.setShadowProperties({juce::Colours::black.withAlpha(0.7f), 15, {0, 5}});

    titleLabel.setFont(juce::Font(22.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, OrpheusLookAndFeel::textPrimary());
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    auto setupLabel = [this](juce::Label& lbl) {
        lbl.setFont(juce::Font(14.0f));
        lbl.setColour(juce::Label::textColourId, OrpheusLookAndFeel::textMuted());
        lbl.setJustificationType(juce::Justification::centredRight);
        addAndMakeVisible(lbl);
    };

    setupLabel(formatLabel);
    setupLabel(sampleRateLabel);
    setupLabel(bitDepthLabel);
    setupLabel(modeLabel);
    setupLabel(presetLabel);
    setupLabel(lufsLabel);
    setupLabel(truePeakLabel);

    addAndMakeVisible(formatBox);
    formatBox.addItemList({ "WAV (.wav)", "MP3 (.mp3)", "AAC (.m4a)", "OGG (.ogg)", "FLAC (.flac)", "ALAC (.m4a)", "AIFF (.aiff)" }, 1);
    formatBox.setSelectedId(1, juce::dontSendNotification);

    addAndMakeVisible(sampleRateBox);
    sampleRateBox.addItemList({ "44100 Hz", "48000 Hz", "88200 Hz", "96000 Hz", "192000 Hz" }, 1);
    sampleRateBox.setSelectedId(2, juce::dontSendNotification);

    addAndMakeVisible(bitDepthBox);
    bitDepthBox.addItemList({ "16-Bit", "24-Bit", "32-Bit Float" }, 1);
    bitDepthBox.setSelectedId(2, juce::dontSendNotification);

    addAndMakeVisible(modeBox);
    modeBox.addItemList({ "Master Mix", "Auto-Stems", "AI Separation Extraction" }, 1);
    modeBox.setSelectedId(1, juce::dontSendNotification);

    addAndMakeVisible(presetBox);
    presetBox.addItemList({ "None (Unrestricted)", "Spotify Standard (-14 LUFS, -1.0 TP)", "Apple Music (-16 LUFS, -1.0 TP)", "CD Mastering (-9 LUFS, -0.1 TP)", "Custom target..." }, 1);
    presetBox.setSelectedId(1, juce::dontSendNotification);
    presetBox.onChange = [this]() { updatePreset(presetBox.getSelectedId()); };

    addAndMakeVisible(lufsSlider);
    lufsSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    lufsSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    lufsSlider.setRange(-24.0, 0.0, 0.1);
    lufsSlider.setValue(-14.0);
    lufsSlider.setEnabled(false);

    addAndMakeVisible(truePeakSlider);
    truePeakSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    truePeakSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    truePeakSlider.setRange(-3.0, 0.0, 0.1);
    truePeakSlider.setValue(-1.0);
    truePeakSlider.setEnabled(false);

    addAndMakeVisible(ditherToggle);
    ditherToggle.setToggleState(true, juce::dontSendNotification);
    
    addAndMakeVisible(offlineToggle);
    offlineToggle.setToggleState(true, juce::dontSendNotification);

    addAndMakeVisible(exportButton);
    exportButton.setColour(juce::TextButton::buttonColourId, OrpheusLookAndFeel::accentPrimary());
    exportButton.onClick = [this]() { triggerExport(); };

    addAndMakeVisible(cancelButton);
    cancelButton.setColour(juce::TextButton::buttonColourId, OrpheusLookAndFeel::bgDark());
    cancelButton.onClick = [this]() { if (onCancel) onCancel(); };

    setSize(500, 560);
}

ExportDialog::~ExportDialog() {}

void ExportDialog::updatePreset(int presetId)
{
    bool isCustom = (presetId == 5);
    lufsSlider.setEnabled(isCustom);
    truePeakSlider.setEnabled(isCustom);

    if (presetId == 2) { // Spotify
        lufsSlider.setValue(-14.0, juce::dontSendNotification);
        truePeakSlider.setValue(-1.0, juce::dontSendNotification);
    } else if (presetId == 3) { // Apple
        lufsSlider.setValue(-16.0, juce::dontSendNotification);
        truePeakSlider.setValue(-1.0, juce::dontSendNotification);
    } else if (presetId == 4) { // CD
        lufsSlider.setValue(-9.0, juce::dontSendNotification);
        truePeakSlider.setValue(-0.1, juce::dontSendNotification);
    }
}

void ExportDialog::triggerExport()
{
    // Configure settings
    AudioExportManager::ExportSettings settings;
    
    juce::String ext = ".wav";
    switch(formatBox.getSelectedId()) {
        case 1: ext = ".wav"; break;
        case 2: ext = ".mp3"; break;
        case 3: ext = ".m4a"; break; // AAC
        case 4: ext = ".ogg"; break;
        case 5: ext = ".flac"; break;
        case 6: ext = ".m4a"; break; // ALAC
        case 7: ext = ".aiff"; break;
    }
    settings.formatExtension = ext;
    
    switch(sampleRateBox.getSelectedId()) {
        case 1: settings.sampleRate = 44100; break;
        case 2: settings.sampleRate = 48000; break;
        case 3: settings.sampleRate = 88200; break;
        case 4: settings.sampleRate = 96000; break;
        case 5: settings.sampleRate = 192000; break;
    }

    switch(bitDepthBox.getSelectedId()) {
        case 1: settings.bitDepth = 16; break;
        case 2: settings.bitDepth = 24; break;
        case 3: settings.bitDepth = 32; break; // float
    }

    settings.enforceStandard = (presetBox.getSelectedId() > 1); // If a standard is picked, we'll map this broadly internally
    
    settings.mode = static_cast<AudioExportManager::ExportMode>(modeBox.getSelectedId() - 1);

    auto chooserFlags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles;
    chooser = std::make_unique<juce::FileChooser>("Select export destination...", juce::File::getSpecialLocation(juce::File::userMusicDirectory), "*" + ext);
    chooser->launchAsync(chooserFlags, [this, settings](const juce::FileChooser& fc)
    {
        juce::File target = fc.getResult();
        if (target != juce::File{})
        {
            manager.performExport(target, settings, nullptr);
            if (onExportStarted) onExportStarted();
        }
    });
}

void ExportDialog::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    if (auto* lnf = dynamic_cast<OrpheusLookAndFeel*>(&getLookAndFeel())) {
        lnf->drawGlassBackground(g, bounds, 12.0f);
    } else {
        g.setColour(juce::Colours::black.withAlpha(0.8f));
        g.fillRoundedRectangle(bounds, 12.0f);
    }
    
    // Draw an accent glow top line
    g.setGradientFill(juce::ColourGradient(OrpheusLookAndFeel::accentPrimary().withAlpha(0.6f), bounds.getWidth() * 0.5f, 0.0f,
                                           OrpheusLookAndFeel::accentPrimary().withAlpha(0.0f), bounds.getWidth() * 0.5f, 60.0f, false));
    g.fillRect(bounds.withHeight(60.0f));
}

void ExportDialog::resized()
{
    auto area = getLocalBounds().reduced(30);
    titleLabel.setBounds(area.removeFromTop(40));
    area.removeFromTop(10);

    int rowH = 32;
    int labelW = 140;
    int gap = 15;

    auto makeRow = [&](juce::Label& lbl, juce::Component& comp) {
        auto row = area.removeFromTop(rowH);
        lbl.setBounds(row.removeFromLeft(labelW));
        row.removeFromLeft(gap);
        comp.setBounds(row);
        area.removeFromTop(10);
    };

    makeRow(modeLabel, modeBox);
    makeRow(formatLabel, formatBox);
    makeRow(sampleRateLabel, sampleRateBox);
    makeRow(bitDepthLabel, bitDepthBox);
    
    
    area.removeFromTop(10);
    
    makeRow(presetLabel, presetBox);
    makeRow(lufsLabel, lufsSlider);
    makeRow(truePeakLabel, truePeakSlider);

    area.removeFromTop(10);
    auto toggleRow = area.removeFromTop(rowH);
    toggleRow.removeFromLeft(labelW + gap);
    ditherToggle.setBounds(toggleRow.removeFromLeft(150));
    offlineToggle.setBounds(toggleRow);

    auto btnArea = getLocalBounds().removeFromBottom(80).reduced(20, 0);
    exportButton.setBounds(btnArea.removeFromRight(140).withSizeKeepingCentre(140, 40));
    btnArea.removeFromRight(15);
    cancelButton.setBounds(btnArea.removeFromRight(100).withSizeKeepingCentre(100, 40));
}
