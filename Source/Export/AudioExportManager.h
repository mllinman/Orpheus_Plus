#pragma once
#include <JuceHeader.h>
#include "../Audio/AudioEngine.h"

class AudioExportManager
{
public:
    AudioExportManager(AudioEngine& engine);
    ~AudioExportManager();

    enum class ExportMode
    {
        MasterMix,
        SelectedTracks,
        AutoStems,
        AISeparation
    };

    struct ExportSettings
    {
        juce::String formatExtension { ".wav" };
        int sampleRate { 48000 };
        int bitDepth { 24 };
        int quality { 192 }; // For mp3/ogg kbps
        ExportMode mode { ExportMode::MasterMix };
        bool spotifyPreset { false };
        std::vector<int> selectedTracks;
    };

    // Registers all format writers (WAV, FLAC, OGG, AIFF, and native MP3/AAC)
    void registerFormats();

    // The main entry point for exporting.
    // outputFileOrDir determines if it is a single file or a directory for stems.
    void performExport(const juce::File& outputFileOrDir, const ExportSettings& settings, class AppState* appState = nullptr);

private:
    AudioEngine& audioEngine;
    juce::AudioFormatManager formatManager;

    void exportMasterMix(const juce::File& outputFile, const ExportSettings& settings);
    void exportSelectedTracks(const juce::File& outputFile, const ExportSettings& settings);
    void exportAutoStems(const juce::File& outputDirectory, const ExportSettings& settings);
    void exportAISeparation(const juce::File& outputDirectory, const ExportSettings& settings, class AppState* appState);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioExportManager)
};
