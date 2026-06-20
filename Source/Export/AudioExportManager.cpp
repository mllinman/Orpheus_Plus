#include "AudioExportManager.h"
#include "../Util/OrpheusLogger.h"
#include "../Mastering/MasteringModule.h"

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
    MasteringModule* mastering = audioEngine.getMasteringModule();

    if (settings.spotifyPreset)
    {
        if (mastering) mastering->forceSpotifyPreset(true);
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

    if (settings.spotifyPreset)
    {
        if (mastering) mastering->forceSpotifyPreset(false);
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
    OrpheusLogger::logInfo("Exporting Selected Tracks to " + outputFile.getFullPathName());

    auto numTracks = audioEngine.getTrackCount();
    struct TrackState { bool mute; bool solo; };
    std::vector<TrackState> originalStates(numTracks);

    // Save states and mute non-selected
    for (int i = 0; i < numTracks; ++i)
    {
        auto* track = audioEngine.getTrack(i);
        if (track)
        {
            originalStates[i] = { track->mute, track->solo };
            
            bool isSelected = std::find(settings.selectedTracks.begin(), settings.selectedTracks.end(), i) != settings.selectedTracks.end();
            if (!isSelected)
            {
                audioEngine.setTrackMute(i, true);
            }
        }
    }

    // Export
    exportMasterMix(outputFile, settings);

    // Restore states
    for (int i = 0; i < numTracks; ++i)
    {
        auto* track = audioEngine.getTrack(i);
        if (track)
        {
            audioEngine.setTrackMute(i, originalStates[i].mute);
            audioEngine.setTrackSolo(i, originalStates[i].solo);
        }
    }
}

void AudioExportManager::exportAutoStems(const juce::File& outputDirectory, const ExportSettings& settings)
{
    OrpheusLogger::logInfo("Exporting Auto-Stems to " + outputDirectory.getFullPathName());
    outputDirectory.createDirectory();

    auto numTracks = audioEngine.getTrackCount();
    struct TrackState { bool mute; bool solo; };
    std::vector<TrackState> originalStates(numTracks);

    // Save states
    for (int i = 0; i < numTracks; ++i)
    {
        auto* track = audioEngine.getTrack(i);
        if (track)
        {
            originalStates[i] = { track->mute, track->solo };
        }
    }

    // Iterate and bounce
    for (int i = 0; i < numTracks; ++i)
    {
        auto* track = audioEngine.getTrack(i);
        if (!track) continue;

        // Isolate this track by muting all others
        for (int j = 0; j < numTracks; ++j)
        {
            if (j == i)
            {
                audioEngine.setTrackMute(j, false);
                audioEngine.setTrackSolo(j, true);
            }
            else
            {
                audioEngine.setTrackMute(j, true);
                audioEngine.setTrackSolo(j, false);
            }
        }

        juce::String trackName = track->name.isEmpty() ? "Track_" + juce::String(i + 1) : track->name;
        juce::String fileName = juce::File::createLegalFileName(trackName) + settings.formatExtension;
        auto stemFile = outputDirectory.getChildFile(fileName);
        
        exportMasterMix(stemFile, settings);
    }

    // Restore states
    for (int i = 0; i < numTracks; ++i)
    {
        auto* track = audioEngine.getTrack(i);
        if (track)
        {
            audioEngine.setTrackMute(i, originalStates[i].mute);
            audioEngine.setTrackSolo(i, originalStates[i].solo);
        }
    }
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
