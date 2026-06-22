#include "SettingsHubPanel.h"
#include "../Audio/PluginManager.h"

//==============================================================================
// SettingsHubPanel
//==============================================================================
SettingsHubPanel::SettingsHubPanel(AudioEngine& engine, AppState& state,
                                   ProjectManager& pm, juce::ApplicationCommandManager& cmdMgr)
    : audioEngine(engine), appState(state), projectManager(pm), commandManager(cmdMgr)
{
    auto tabBg = juce::Colour(0xff12121e);

    audioMidiPage  = std::make_unique<AudioMidiPage>(engine);
    projectPage    = std::make_unique<ProjectPage>(pm);
    pluginPage     = std::make_unique<PluginPage>(engine);
    shortcutsPage  = std::make_unique<ShortcutsPage>(cmdMgr);
    appearancePage = std::make_unique<AppearancePage>();
    aiPage         = std::make_unique<AIPage>();
    exportPage     = std::make_unique<ExportPage>();
    aboutPage      = std::make_unique<AboutPage>();

    tabs.setTabBarDepth(120);
    tabs.addTab("Audio/MIDI",    tabBg, audioMidiPage.get(),  false);
    tabs.addTab("Project",       tabBg, projectPage.get(),    false);
    tabs.addTab("VST Plugins",   tabBg, pluginPage.get(),     false);
    tabs.addTab("Shortcuts",     tabBg, shortcutsPage.get(),  false);
    tabs.addTab("Appearance",    tabBg, appearancePage.get(),  false);
    tabs.addTab("AI/Processing", tabBg, aiPage.get(),         false);
    tabs.addTab("Export",        tabBg, exportPage.get(),     false);
    tabs.addTab("About",         tabBg, aboutPage.get(),      false);

    addAndMakeVisible(tabs);
}

SettingsHubPanel::~SettingsHubPanel() {}

void SettingsHubPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0d0d1a));
}

void SettingsHubPanel::resized()
{
    tabs.setBounds(getLocalBounds());
}

//==============================================================================
// Audio/MIDI Page
//==============================================================================
SettingsHubPanel::AudioMidiPage::AudioMidiPage(AudioEngine& e)
{
    deviceSelector = std::make_unique<juce::AudioDeviceSelectorComponent>(
        e.getDeviceManager(), 0, 256, 0, 256, true, true, true, false);
    addAndMakeVisible(deviceSelector.get());
}

void SettingsHubPanel::AudioMidiPage::resized()
{
    deviceSelector->setBounds(getLocalBounds().reduced(8));
}

//==============================================================================
// Project Page
//==============================================================================
SettingsHubPanel::ProjectPage::ProjectPage(ProjectManager& p) : pm(p)
{
    titleLabel.setText("PROJECT SETTINGS", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(16.0f).boldened());
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(titleLabel);

    dirLabel.setText("Default Directory:", juce::dontSendNotification);
    dirLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(dirLabel);

    dirEditor.setText(pm.getDefaultProjectDirectory().getFullPathName());
    addAndMakeVisible(dirEditor);

    browseBtn.onClick = [this]() {
        chooser = std::make_unique<juce::FileChooser>("Select Default Project Directory",
            juce::File(dirEditor.getText()));
        chooser->launchAsync(
            juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
            [this](const juce::FileChooser& fc) {
                auto result = fc.getResult();
                if (result.isDirectory()) {
                    dirEditor.setText(result.getFullPathName());
                    this->pm.setDefaultProjectDirectory(result);
                }
                chooser.reset();
            });
    };
    addAndMakeVisible(browseBtn);

    addAndMakeVisible(copyAudioToggle);
    addAndMakeVisible(autoSaveToggle);

    autoSaveIntervalSlider.setRange(1, 30, 1);
    autoSaveIntervalSlider.setValue(5);
    autoSaveIntervalSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 20);
    addAndMakeVisible(autoSaveIntervalSlider);
}

void SettingsHubPanel::ProjectPage::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0d0d1a));
}

void SettingsHubPanel::ProjectPage::resized()
{
    auto b = getLocalBounds().reduced(16);
    titleLabel.setBounds(b.removeFromTop(30));
    b.removeFromTop(12);
    dirLabel.setBounds(b.removeFromTop(22));
    auto dirRow = b.removeFromTop(28);
    dirEditor.setBounds(dirRow.removeFromLeft(dirRow.getWidth() - 100));
    dirRow.removeFromLeft(4);
    browseBtn.setBounds(dirRow);
    b.removeFromTop(12);
    copyAudioToggle.setBounds(b.removeFromTop(24));
    b.removeFromTop(8);
    autoSaveToggle.setBounds(b.removeFromTop(24));
    b.removeFromTop(4);
    autoSaveIntervalSlider.setBounds(b.removeFromTop(28).withTrimmedLeft(24));
}

//==============================================================================
// VST Plugins Page
//==============================================================================
SettingsHubPanel::PluginPage::PluginPage(AudioEngine& e) : engine(e)
{
    scanBtn.onClick = [this]() { engine.getPluginManager().scanForPlugins(); };
    addAndMakeVisible(scanBtn);

    addPathBtn.onClick = [this]() {
        if (chooser) return; // Prevent double-open
        chooser = std::make_unique<juce::FileChooser>(
            "Select VST Plugin Directory",
            juce::File::getSpecialLocation(juce::File::userHomeDirectory));
        chooser->launchAsync(
            juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
            [this](const juce::FileChooser& fc) {
                auto result = fc.getResult();
                if (result.isDirectory()) {
                    engine.getPluginManager().addSearchPath(result);
                    refreshPaths();
                }
                chooser.reset(); // Release the dialog
            });
    };
    addAndMakeVisible(addPathBtn);

    removePathBtn.onClick = [this]() {
        int sel = pathList.getSelectedRow();
        if (sel >= 0 && sel < pathModel.paths.size()) {
            engine.getPluginManager().removeSearchPath(juce::File(pathModel.paths[sel]));
            refreshPaths();
        }
    };
    addAndMakeVisible(removePathBtn);

    addAndMakeVisible(scanOnStartup);
    scanOnStartup.setToggleState(true, juce::dontSendNotification);

    pathList.setModel(&pathModel);
    pathList.setRowHeight(20);
    pathList.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff0a0a16));
    addAndMakeVisible(pathList);

    refreshPaths();
}

void SettingsHubPanel::PluginPage::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0d0d1a));
    auto b = getLocalBounds().reduced(16);
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(16.0f).boldened());
    g.drawText("VST PLUGIN MANAGEMENT", b.removeFromTop(30), juce::Justification::centredLeft);
}

void SettingsHubPanel::PluginPage::resized()
{
    auto b = getLocalBounds().reduced(16);
    b.removeFromTop(36); // Title
    auto btnRow = b.removeFromTop(30);
    scanBtn.setBounds(btnRow.removeFromLeft(140).reduced(0, 2));
    btnRow.removeFromLeft(8);
    scanOnStartup.setBounds(btnRow.removeFromLeft(220));
    b.removeFromTop(12);

    auto pathTitle = b.removeFromTop(20);
    // "Custom Search Paths" label handled in paint if needed

    auto pathBtnRow = b.removeFromTop(28);
    addPathBtn.setBounds(pathBtnRow.removeFromLeft(160).reduced(0, 2));
    pathBtnRow.removeFromLeft(4);
    removePathBtn.setBounds(pathBtnRow.removeFromLeft(140).reduced(0, 2));

    b.removeFromTop(4);
    pathList.setBounds(b);
}

void SettingsHubPanel::PluginPage::refreshPaths()
{
    pathModel.paths.clear();
    auto& customPaths = engine.getPluginManager().getCustomSearchPaths();
    for (int i = 0; i < customPaths.getNumPaths(); ++i)
        pathModel.paths.add(customPaths[i].getFullPathName());
    pathList.updateContent();
}

void SettingsHubPanel::PluginPage::PathModel::paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool sel)
{
    if (!juce::isPositiveAndBelow(row, paths.size())) return;
    if (sel) {
        g.setColour(juce::Colour(0xff533483).withAlpha(0.5f));
        g.fillRect(0, 0, w, h);
    }
    g.setColour(sel ? juce::Colours::white : juce::Colours::lightgrey);
    g.setFont(juce::Font(11.0f));
    g.drawText(paths[row], 8, 0, w - 16, h, juce::Justification::centredLeft);
}

//==============================================================================
// Shortcuts Page
//==============================================================================
SettingsHubPanel::ShortcutsPage::ShortcutsPage(juce::ApplicationCommandManager& cm)
    : keyEditor(*cm.getKeyMappings(), true)
{
    addAndMakeVisible(keyEditor);
}

void SettingsHubPanel::ShortcutsPage::resized()
{
    keyEditor.setBounds(getLocalBounds().reduced(8));
}

//==============================================================================
// Appearance Page
//==============================================================================
SettingsHubPanel::AppearancePage::AppearancePage()
{
    themeLabel.setText("Theme:", juce::dontSendNotification);
    themeLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(themeLabel);

    themeCombo.addItem("Orpheus Dark (Default)", 1);
    themeCombo.addItem("Midnight Blue", 2);
    themeCombo.addItem("Deep Purple", 3);
    themeCombo.setSelectedId(1, juce::dontSendNotification);
    addAndMakeVisible(themeCombo);

    accentLabel.setText("Accent Hue:", juce::dontSendNotification);
    accentLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(accentLabel);

    accentHueSlider.setRange(0, 360, 1);
    accentHueSlider.setValue(260);
    accentHueSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    addAndMakeVisible(accentHueSlider);

    fontLabel.setText("UI Font:", juce::dontSendNotification);
    fontLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(fontLabel);

    fontCombo.addItem("Inter", 1);
    fontCombo.addItem("System Default", 2);
    fontCombo.addItem("Roboto", 3);
    fontCombo.setSelectedId(1, juce::dontSendNotification);
    addAndMakeVisible(fontCombo);

    scaleLabel.setText("UI Scale:", juce::dontSendNotification);
    scaleLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(scaleLabel);

    scaleSlider.setRange(0.75, 2.0, 0.05);
    scaleSlider.setValue(1.0);
    scaleSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    addAndMakeVisible(scaleSlider);
}

void SettingsHubPanel::AppearancePage::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0d0d1a));
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(16.0f).boldened());
    g.drawText("APPEARANCE", getLocalBounds().reduced(16).removeFromTop(30), juce::Justification::centredLeft);
}

void SettingsHubPanel::AppearancePage::resized()
{
    auto b = getLocalBounds().reduced(16);
    b.removeFromTop(36);
    auto row = [&]() { auto r = b.removeFromTop(30); b.removeFromTop(4); return r; };

    auto r1 = row(); themeLabel.setBounds(r1.removeFromLeft(100)); themeCombo.setBounds(r1.removeFromLeft(200));
    auto r2 = row(); accentLabel.setBounds(r2.removeFromLeft(100)); accentHueSlider.setBounds(r2.removeFromLeft(250));
    auto r3 = row(); fontLabel.setBounds(r3.removeFromLeft(100)); fontCombo.setBounds(r3.removeFromLeft(200));
    auto r4 = row(); scaleLabel.setBounds(r4.removeFromLeft(100)); scaleSlider.setBounds(r4.removeFromLeft(250));
}

//==============================================================================
// AI/Processing Page
//==============================================================================
SettingsHubPanel::AIPage::AIPage()
{
    deviceLabel.setText("Inference Device:", juce::dontSendNotification);
    deviceLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(deviceLabel);

    deviceCombo.addItem("CPU", 1);
    deviceCombo.addItem("GPU (CUDA)", 2);
    deviceCombo.addItem("GPU (DirectML)", 3);
    deviceCombo.setSelectedId(1, juce::dontSendNotification);
    addAndMakeVisible(deviceCombo);

    qualityLabel.setText("Stem Sep Quality:", juce::dontSendNotification);
    qualityLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(qualityLabel);

    qualityCombo.addItem("Fast", 1);
    qualityCombo.addItem("Balanced", 2);
    qualityCombo.addItem("High Quality", 3);
    qualityCombo.setSelectedId(2, juce::dontSendNotification);
    addAndMakeVisible(qualityCombo);

    strengthLabel.setText("Humanizer Strength:", juce::dontSendNotification);
    strengthLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(strengthLabel);

    strengthSlider.setRange(0, 100, 1);
    strengthSlider.setValue(50);
    strengthSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    strengthSlider.setTextValueSuffix("%");
    addAndMakeVisible(strengthSlider);
}

void SettingsHubPanel::AIPage::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0d0d1a));
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(16.0f).boldened());
    g.drawText("AI & PROCESSING", getLocalBounds().reduced(16).removeFromTop(30), juce::Justification::centredLeft);
}

void SettingsHubPanel::AIPage::resized()
{
    auto b = getLocalBounds().reduced(16);
    b.removeFromTop(36);
    auto row = [&]() { auto r = b.removeFromTop(30); b.removeFromTop(4); return r; };

    auto r1 = row(); deviceLabel.setBounds(r1.removeFromLeft(140)); deviceCombo.setBounds(r1.removeFromLeft(200));
    auto r2 = row(); qualityLabel.setBounds(r2.removeFromLeft(140)); qualityCombo.setBounds(r2.removeFromLeft(200));
    auto r3 = row(); strengthLabel.setBounds(r3.removeFromLeft(140)); strengthSlider.setBounds(r3.removeFromLeft(250));
}

//==============================================================================
// Export Defaults Page
//==============================================================================
SettingsHubPanel::ExportPage::ExportPage()
{
    formatLabel.setText("Default Format:", juce::dontSendNotification);
    formatLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(formatLabel);

    formatCombo.addItem("WAV", 1);
    formatCombo.addItem("FLAC", 2);
    formatCombo.addItem("AIFF", 3);
    formatCombo.addItem("MP3", 4);
    formatCombo.setSelectedId(1, juce::dontSendNotification);
    addAndMakeVisible(formatCombo);

    bitLabel.setText("Bit Depth:", juce::dontSendNotification);
    bitLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(bitLabel);

    bitDepthCombo.addItem("16-bit", 1);
    bitDepthCombo.addItem("24-bit", 2);
    bitDepthCombo.addItem("32-bit float", 3);
    bitDepthCombo.setSelectedId(2, juce::dontSendNotification);
    addAndMakeVisible(bitDepthCombo);

    rateLabel.setText("Sample Rate:", juce::dontSendNotification);
    rateLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(rateLabel);

    sampleRateCombo.addItem("44100 Hz", 1);
    sampleRateCombo.addItem("48000 Hz", 2);
    sampleRateCombo.addItem("96000 Hz", 3);
    sampleRateCombo.setSelectedId(1, juce::dontSendNotification);
    addAndMakeVisible(sampleRateCombo);

    addAndMakeVisible(normalizeToggle);
    normalizeToggle.setToggleState(true, juce::dontSendNotification);
    addAndMakeVisible(ditherToggle);
}

void SettingsHubPanel::ExportPage::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0d0d1a));
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(16.0f).boldened());
    g.drawText("EXPORT DEFAULTS", getLocalBounds().reduced(16).removeFromTop(30), juce::Justification::centredLeft);
}

void SettingsHubPanel::ExportPage::resized()
{
    auto b = getLocalBounds().reduced(16);
    b.removeFromTop(36);
    auto row = [&]() { auto r = b.removeFromTop(30); b.removeFromTop(4); return r; };

    auto r1 = row(); formatLabel.setBounds(r1.removeFromLeft(120)); formatCombo.setBounds(r1.removeFromLeft(180));
    auto r2 = row(); bitLabel.setBounds(r2.removeFromLeft(120)); bitDepthCombo.setBounds(r2.removeFromLeft(180));
    auto r3 = row(); rateLabel.setBounds(r3.removeFromLeft(120)); sampleRateCombo.setBounds(r3.removeFromLeft(180));
    b.removeFromTop(8);
    normalizeToggle.setBounds(b.removeFromTop(24));
    b.removeFromTop(4);
    ditherToggle.setBounds(b.removeFromTop(24));
}

//==============================================================================
// About Page
//==============================================================================
void SettingsHubPanel::AboutPage::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0d0d1a));
    auto b = getLocalBounds().reduced(32);

    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(24.0f).boldened());
    g.drawText("Orpheus Plus", b.removeFromTop(40), juce::Justification::centredLeft);

    g.setFont(juce::Font(14.0f));
    g.setColour(juce::Colour(0xffa29bfe));
    g.drawText("Version 1.0.0", b.removeFromTop(24), juce::Justification::centredLeft);

    b.removeFromTop(16);
    g.setColour(juce::Colours::lightgrey);
    g.setFont(12.0f);

    juce::StringArray lines = {
        "Professional AI-Assisted Digital Audio Workstation",
        "",
        "Features: Stem separation, AI humanizer, voice cloning,",
        "auto-mix, distribution prep, modulation matrix,",
        "VST3/AU hosting, and more.",
        "",
        "Built with JUCE Framework + ONNX Runtime",
        "",
        juce::String(juce::CharPointer_UTF8("\xc2\xa9")) + " 2026 Orpheus Audio. All rights reserved."
    };

    for (auto& line : lines)
        g.drawText(line, b.removeFromTop(18), juce::Justification::centredLeft);
}
