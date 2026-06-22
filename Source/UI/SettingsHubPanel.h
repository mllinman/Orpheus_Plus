#pragma once
#include <JuceHeader.h>
#include "../Audio/AudioEngine.h"
#include "../Project/AppState.h"
#include "../Project/ProjectManager.h"
#include "OrpheusLookAndFeel.h"

//==============================================================================
// SettingsHubPanel — Comprehensive settings with sub-tabs for every app option.
//==============================================================================
class SettingsHubPanel : public juce::Component
{
public:
    SettingsHubPanel(AudioEngine& engine, AppState& state, ProjectManager& pm,
                     juce::ApplicationCommandManager& cmdMgr);
    ~SettingsHubPanel() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    AudioEngine& audioEngine;
    AppState& appState;
    ProjectManager& projectManager;
    juce::ApplicationCommandManager& commandManager;

    juce::TabbedComponent tabs { juce::TabbedButtonBar::TabsAtLeft };

    //── Sub-panel: Audio/MIDI ────────────────────────────────────────────
    struct AudioMidiPage : public juce::Component
    {
        explicit AudioMidiPage(AudioEngine& e);
        void resized() override;
        std::unique_ptr<juce::AudioDeviceSelectorComponent> deviceSelector;
    };

    //── Sub-panel: Project ───────────────────────────────────────────────
    struct ProjectPage : public juce::Component
    {
        explicit ProjectPage(ProjectManager& pm);
        void resized() override;
        void paint(juce::Graphics& g) override;

        ProjectManager& pm;
        juce::Label titleLabel, dirLabel;
        juce::TextEditor dirEditor;
        juce::TextButton browseBtn { "Browse..." };
        juce::ToggleButton copyAudioToggle { "Copy audio files to project folder" };
        juce::ToggleButton autoSaveToggle { "Auto-save every 5 minutes" };
        juce::Slider autoSaveIntervalSlider;
        std::unique_ptr<juce::FileChooser> chooser;
    };

    //── Sub-panel: VST Plugins ───────────────────────────────────────────
    struct PluginPage : public juce::Component
    {
        explicit PluginPage(AudioEngine& e);
        void resized() override;
        void paint(juce::Graphics& g) override;
        void refreshPaths();

        AudioEngine& engine;
        juce::TextButton scanBtn { "Scan Plugins Now" };
        juce::TextButton addPathBtn { "Add VST Folder..." };
        juce::TextButton removePathBtn { "Remove Selected" };
        juce::ToggleButton scanOnStartup { "Scan for new plugins on startup" };
        juce::ListBox pathList;

        struct PathModel : public juce::ListBoxModel {
            juce::StringArray paths;
            int getNumRows() override { return paths.size(); }
            void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool sel) override;
        } pathModel;

        std::unique_ptr<juce::FileChooser> chooser;
    };

    //── Sub-panel: Shortcuts ─────────────────────────────────────────────
    struct ShortcutsPage : public juce::Component
    {
        explicit ShortcutsPage(juce::ApplicationCommandManager& cm);
        void resized() override;
        juce::KeyMappingEditorComponent keyEditor;
    };

    //── Sub-panel: Appearance ────────────────────────────────────────────
    struct AppearancePage : public juce::Component
    {
        AppearancePage();
        void resized() override;
        void paint(juce::Graphics& g) override;

        juce::Label themeLabel, accentLabel, fontLabel, scaleLabel;
        juce::ComboBox themeCombo, fontCombo;
        juce::Slider scaleSlider;
        juce::Slider accentHueSlider;
    };

    //── Sub-panel: AI / Processing ───────────────────────────────────────
    struct AIPage : public juce::Component
    {
        AIPage();
        void resized() override;
        void paint(juce::Graphics& g) override;

        juce::Label deviceLabel, qualityLabel, strengthLabel;
        juce::ComboBox deviceCombo, qualityCombo;
        juce::Slider strengthSlider;
    };

    //── Sub-panel: Export Defaults ────────────────────────────────────────
    struct ExportPage : public juce::Component
    {
        ExportPage();
        void resized() override;
        void paint(juce::Graphics& g) override;

        juce::Label formatLabel, bitLabel, rateLabel;
        juce::ComboBox formatCombo, bitDepthCombo, sampleRateCombo;
        juce::ToggleButton normalizeToggle { "Normalize to -1 dBTP" };
        juce::ToggleButton ditherToggle { "Apply dithering on export" };
    };

    //── Sub-panel: About ─────────────────────────────────────────────────
    struct AboutPage : public juce::Component
    {
        void paint(juce::Graphics& g) override;
    };

    // Owned sub-panels
    std::unique_ptr<AudioMidiPage>  audioMidiPage;
    std::unique_ptr<ProjectPage>    projectPage;
    std::unique_ptr<PluginPage>     pluginPage;
    std::unique_ptr<ShortcutsPage>  shortcutsPage;
    std::unique_ptr<AppearancePage> appearancePage;
    std::unique_ptr<AIPage>         aiPage;
    std::unique_ptr<ExportPage>     exportPage;
    std::unique_ptr<AboutPage>      aboutPage;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsHubPanel)
};
