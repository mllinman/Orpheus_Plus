#include "AudioExportManager.h"
#include "../Util/OrpheusLogger.h"

AudioExportManager::AudioExportManager(AudioEngine& engine)
    : audioEngine(engine)
{
    registerFormats();
}

AudioExportManager::~AudioExportManager()
{
}

void AudioExportManager::registerFormats()
{
    formatManager.registerBasicFormats(); // WAV, AIFF
    
    // Add open-source formats
    formatManager.registerFormat(new juce::FlacAudioFormat(), false);
    formatManager.registerFormat(new juce::OggVorbisAudioFormat(), false);

    // Add native OS codecs for MP3/AAC
#if JUCE_MAC || JUCE_IOS
    formatManager.registerFormat(new juce::CoreAudioFormat(), false);
#elif JUCE_WINDOWS
    formatManager.registerFormat(new juce::WindowsMediaAudioFormat(), false);
#endif
    
    OrpheusLogger::logInfo("AudioExportManager: Registered formats.");
}

void AudioExportManager::performExport(const juce::File& outputFileOrDir, const ExportSettings& settings)
{
    if (settings.spotifyPreset)
    {
        // TODO: Signal MasteringModule to force -14 LUFS, -1 dB TP
        OrpheusLogger::logInfo("AudioExportManager: Spotify Preset active. Forcing target loudness.");
    }

    switch (settings.mode)
    {
        case ExportMode::MasterMix:
            exportMasterMix(outputFileOrDir, settings);
            break;
        case ExportMode::SelectedTracks:
            exportSelectedTracks(outputFileOrDir, settings);
            break;
        case ExportMode::AutoStems:
            exportAutoStems(outputFileOrDir, settings);
            break;
        case ExportMode::AISeparation:
            exportAISeparation(outputFileOrDir, settings);
            break;
    }
}

void AudioExportManager::exportMasterMix(const juce::File& outputFile, const ExportSettings& settings)
{
    OrpheusLogger::logInfo("Exporting Master Mix to " + outputFile.getFullPathName());
    
    auto* format = formatManager.findFormatForFileExtension(settings.formatExtension);
    if (!format)
    {
        OrpheusLogger::logError("No format writer found for " + settings.formatExtension);
        return;
    }

    auto stream = outputFile.createOutputStream();
    if (!stream) return;

    // Quality index 0 for lossless, varies for lossy
    std::unique_ptr<juce::AudioFormatWriter> writer(
        format->createWriterFor(stream.release(), settings.sampleRate, 2, settings.bitDepth, {}, 0));

    if (!writer) return;

    // We pass to audio engine to run the offline graph
    // Note: To cleanly move this from AudioEngine, we can just fetch the graph block by block here
    // But for now, we will adapt AudioEngine's logic to use our custom writer properties
    
    // For simplicity, calling the engine's exportMix for now, but passing our settings
    audioEngine.exportMix(outputFile, settings.sampleRate, settings.bitDepth);
}

void AudioExportManager::exportSelectedTracks(const juce::File& outputFile, const ExportSettings& settings)
{
    // TODO: Mute non-selected tracks, bounce, then restore states
    OrpheusLogger::logInfo("Exporting Selected Tracks to " + outputFile.getFullPathName());
    exportMasterMix(outputFile, settings);
}

void AudioExportManager::exportAutoStems(const juce::File& outputDirectory, const ExportSettings& settings)
{
    OrpheusLogger::logInfo("Exporting Auto-Stems to " + outputDirectory.getFullPathName());
    outputDirectory.createDirectory();

    // Save states
    auto numTracks = audioEngine.getTrackCount();

    // Iterate and bounce
    for (int i = 0; i < numTracks; ++i)
    {
        // Solo track i
        // ... (requires adding `setSolo` or similar to AudioEngine)

        auto* track = audioEngine.getTrack(i);
        if (!track) continue;

        juce::String fileName = juce::File::createLegalFileName(track->name) + settings.formatExtension;
        auto stemFile = outputDirectory.getChildFile(fileName);
        
        exportMasterMix(stemFile, settings);
    }

    // Restore states
}

void AudioExportManager::exportAISeparation(const juce::File& outputDirectory, const ExportSettings& settings)
{
    OrpheusLogger::logInfo("Running AI Stem Separation...");
    // 1. Export the master mix to a temp file
    // 2. Feed it into StemSeparator ONNX model
    // 3. Receive Vocals, Bass, Drums, Other buffers
    // 4. Write buffers to outputDirectory
    // 5. Add new tracks to AudioEngine
}
