#include "AudioExportManager.h"
#include "../Project/AppState.h"
#include "../StemSeparation/StemSeparator.h"
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

void AudioExportManager::performExport(const juce::File& outputFileOrDir, const ExportSettings& settings, class AppState* appState)
{
    MasteringModule* mastering = audioEngine.getMasteringModule();

    if (settings.enforceStandard)
    {
        if (mastering) mastering->forceLufsTarget(settings.targetLUFS, settings.targetTruePeak);
        OrpheusLogger::logInfo("AudioExportManager: Standards enforced. Target LUFS: " + juce::String(settings.targetLUFS) + " TP: " + juce::String(settings.targetTruePeak));
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
            exportAISeparation(outputFileOrDir, settings, appState);
            break;
    }

    if (settings.enforceStandard)
    {
        if (mastering) mastering->disableForceLufsTarget();
    }
}

void AudioExportManager::exportMasterMix(const juce::File& outputFile, const ExportSettings& settings)
{
    OrpheusLogger::logInfo("Exporting Master Mix to " + outputFile.getFullPathName());
    
    // Create writer
    std::unique_ptr<juce::AudioFormatWriter> writer(
        formatManager.findFormatForFileExtension(settings.formatExtension)
            ->createWriterFor(new juce::FileOutputStream(outputFile),
                              settings.sampleRate,
                              2,
                              settings.bitDepth,
                              {},
                              settings.quality));

    if (!writer)
    {
        OrpheusLogger::logError("Failed to create audio format writer for " + settings.formatExtension);
        return;
    }

    // Set up bounce bounds
    auto& timeline = audioEngine.getTimelineProcessor();
    double endTime = timeline.getTotalLengthSeconds();
    if (endTime <= 0) endTime = 10.0; // fallback
    endTime += 2.0; // tail

    int numSamples = (int)(endTime * settings.sampleRate);
    int blockSize = 512;
    juce::AudioBuffer<float> buffer(2, blockSize);
    
    auto oldState = audioEngine.getTransportState();
    audioEngine.setTransportState(AudioEngine::TransportState::Exporting);
    audioEngine.prepareToPlay(settings.sampleRate, blockSize);

    for (int i = 0; i < numSamples; i += blockSize)
    {
        int samplesToProcess = juce::jmin(blockSize, numSamples - i);
        buffer.setSize(2, samplesToProcess, false, false, true);
        buffer.clear();

        juce::MidiBuffer midiMessages;
        audioEngine.processBlock(buffer, midiMessages);

        writer->writeFromAudioSampleBuffer(buffer, 0, samplesToProcess);
        
        // Progress callback could go here
    }

    writer.reset();
    audioEngine.setTransportState(oldState);
    OrpheusLogger::logInfo("Master Mix exported successfully.");
}

void AudioExportManager::exportSelectedTracks(const juce::File& outputFile, const ExportSettings& settings)
{
    OrpheusLogger::logInfo("Exporting Selected Tracks...");
    
    auto numTracks = audioEngine.getTrackCount();
    struct TrackState { bool mute; bool solo; };
    std::vector<TrackState> originalStates(numTracks);

    // Save current states and mute unselected
    for (int i = 0; i < numTracks; ++i)
    {
        auto* track = audioEngine.getTrack(i);
        if (track)
        {
            originalStates[i] = { track->mute, track->solo };
            bool isSelected = std::find(settings.selectedTracks.begin(), settings.selectedTracks.end(), i) != settings.selectedTracks.end();
            audioEngine.setTrackMute(i, !isSelected);
            audioEngine.setTrackSolo(i, false);
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

void AudioExportManager::exportAISeparation(const juce::File& outputDirectory, const ExportSettings& settings, class AppState* appState)
{
    OrpheusLogger::logInfo("Running AI Stem Separation...");
    
    if (!appState)
    {
        OrpheusLogger::logError("No AppState provided for AI Stem Separation.");
        return;
    }

    // 1. Export the master mix to a temp file
    outputDirectory.createDirectory();
    auto tempFile = outputDirectory.getChildFile("temp_master_mix.wav");
    
    ExportSettings tempSettings = settings;
    tempSettings.formatExtension = ".wav"; // Model usually expects WAV
    exportMasterMix(tempFile, tempSettings);

    // 2. Feed it into StemSeparator ONNX model
    audioEngine.getStemSeparator().separate(tempFile, *appState, [this, outputDirectory, tempFile, settings, appState](StemSeparationResult result) {
        
        // 3. Receive Vocals, Bass, Drums, Other buffers (StemSeparator writes to its own output folder)
        // 4. Move files to outputDirectory, possibly re-encoding to requested format
        
        auto copyOrEncode = [&](const juce::File& source, const juce::String& name)
        {
            if (!source.existsAsFile()) return;
            
            auto targetFile = outputDirectory.getChildFile(name + settings.formatExtension);
            
            if (settings.formatExtension == ".wav")
            {
                source.copyFileTo(targetFile);
            }
            else
            {
                // Re-encode
                std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(source));
                if (reader)
                {
                    std::unique_ptr<juce::AudioFormatWriter> writer(
                        formatManager.findFormatForFileExtension(settings.formatExtension)
                            ->createWriterFor(new juce::FileOutputStream(targetFile),
                                              reader->sampleRate,
                                              reader->numChannels,
                                              settings.bitDepth,
                                              {},
                                              settings.quality));
                                              
                    if (writer)
                        writer->writeFromAudioReader(*reader, 0, -1);
                }
            }
            
            // 5. Add new tracks to AudioEngine
            int newTrackIdx = audioEngine.addAudioTrack(name);
            auto* track = audioEngine.getTrack(newTrackIdx);
            if (track)
            {
                appState->addAudioRegion(newTrackIdx, targetFile.getFullPathName(), 0.0, 0.0);
            }
        };

        copyOrEncode(result.vocals, "Vocals");
        copyOrEncode(result.drums, "Drums");
        copyOrEncode(result.bass, "Bass");
        copyOrEncode(result.guitar, "Electric Guitar");
        copyOrEncode(result.piano, "Synth");
        copyOrEncode(result.other, "Percussion");
        
        // Cleanup temp file
        tempFile.deleteFile();
        
        // Cleanup the temporary stems folder created by StemSeparator
        if (result.vocals.existsAsFile())
            result.vocals.getParentDirectory().deleteRecursively();

        OrpheusLogger::logInfo("AI Stem Separation completed and auto-imported!");
    });
}
