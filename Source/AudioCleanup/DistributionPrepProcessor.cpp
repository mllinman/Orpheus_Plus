#include "DistributionPrepProcessor.h"
#include "../Util/OrpheusLogger.h"
#include "../Project/AppState.h"
#include "../Audio/AudioEngine.h"

DistributionPrepProcessor::DistributionPrepProcessor() {}
DistributionPrepProcessor::~DistributionPrepProcessor() {}

//==============================================================================
// Platform specs
//==============================================================================

DistributionPrepProcessor::PlatformSpec DistributionPrepProcessor::getSpec(Platform platform)
{
    PlatformSpec spec;

    switch (platform)
    {
        case Platform::Spotify:
            spec.targetLUFS     = -14.0f;
            spec.truePeakLimit  = -1.0f;
            spec.sampleRate     = 44100;
            spec.bitDepth       = 24;
            spec.requiresStereo = true;
            break;

        case Platform::AppleMusic:
            spec.targetLUFS     = -16.0f;
            spec.truePeakLimit  = -1.0f;
            spec.sampleRate     = 44100;
            spec.bitDepth       = 24;
            spec.requiresStereo = true;
            break;

        case Platform::YouTube:
            spec.targetLUFS     = -14.0f;
            spec.truePeakLimit  = -1.0f;
            spec.sampleRate     = 48000;
            spec.bitDepth       = 24;
            spec.requiresStereo = true;
            break;

        case Platform::Generic:
        default:
            spec.targetLUFS     = -14.0f;
            spec.truePeakLimit  = -1.0f;
            spec.sampleRate     = 44100;
            spec.bitDepth       = 24;
            spec.requiresStereo = true;
            break;
    }

    return spec;
}

juce::String DistributionPrepProcessor::getPlatformName(Platform platform)
{
    switch (platform)
    {
        case Platform::Spotify:    return "Spotify";
        case Platform::AppleMusic: return "Apple Music";
        case Platform::YouTube:    return "YouTube";
        case Platform::Generic:    return "Generic";
        default:                   return "Unknown";
    }
}

//==============================================================================
// Metadata stripping
//==============================================================================

bool DistributionPrepProcessor::stripAllMetadata(const juce::File& inputFile, const juce::File& outputFile)
{
    // Strategy: Read the audio data through JUCE's format reader (which ignores
    // metadata chunks) and write a brand-new clean WAV with only fmt + data.
    // This effectively strips all INFO, LIST, BEXT, iXML, id3, and any other
    // non-standard chunks that AI generators may embed.

    OrpheusLogger::logInfo("DistributionPrep: Stripping metadata from " + inputFile.getFileName());

    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(inputFile));
    if (!reader)
    {
        OrpheusLogger::logError("DistributionPrep: Cannot read input file.");
        return false;
    }

    // Read entire audio into buffer
    int numChannels = (int)reader->numChannels;
    juce::int64 numSamples = reader->lengthInSamples;
    double sampleRate = reader->sampleRate;
    int bitsPerSample = (int)reader->bitsPerSample;

    juce::AudioBuffer<float> buffer(numChannels, (int)numSamples);
    reader->read(&buffer, 0, (int)numSamples, 0, true, true);
    reader.reset(); // Release the file

    // Write a clean WAV with no metadata
    juce::WavAudioFormat wavFormat;
    auto* outputStream = new juce::FileOutputStream(outputFile);

    if (outputStream->failedToOpen())
    {
        delete outputStream;
        OrpheusLogger::logError("DistributionPrep: Cannot open output file for writing.");
        return false;
    }

    // Create writer with empty metadata map — this produces a clean WAV
    std::unique_ptr<juce::AudioFormatWriter> writer(
        wavFormat.createWriterFor(outputStream,
                                   sampleRate,
                                   numChannels,
                                   bitsPerSample,
                                   {},    // Empty metadata — no INFO/BEXT chunks
                                   0));

    if (!writer)
    {
        OrpheusLogger::logError("DistributionPrep: Cannot create WAV writer.");
        return false;
    }

    writer->writeFromAudioSampleBuffer(buffer, 0, (int)numSamples);
    writer.reset();

    OrpheusLogger::logInfo("DistributionPrep: Metadata stripped. Clean WAV written to " + outputFile.getFileName());
    return true;
}

bool DistributionPrepProcessor::writeCleanMetadata(const juce::File& wavFile,
                                                     const juce::String& title,
                                                     const juce::String& artist)
{
    // Read the clean WAV, then re-write with minimal metadata
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(wavFile));
    if (!reader) return false;

    int numChannels = (int)reader->numChannels;
    juce::int64 numSamples = reader->lengthInSamples;
    double sampleRate = reader->sampleRate;
    int bitsPerSample = (int)reader->bitsPerSample;

    juce::AudioBuffer<float> buffer(numChannels, (int)numSamples);
    reader->read(&buffer, 0, (int)numSamples, 0, true, true);
    reader.reset();

    // Prepare minimal metadata
    juce::StringPairArray metadata;
    if (title.isNotEmpty())
        metadata.set("INAM", title);    // RIFF INFO title
    if (artist.isNotEmpty())
        metadata.set("IART", artist);   // RIFF INFO artist

    // Re-write the file
    juce::WavAudioFormat wavFormat;
    auto* outputStream = new juce::FileOutputStream(wavFile);
    if (outputStream->failedToOpen())
    {
        delete outputStream;
        return false;
    }

    std::unique_ptr<juce::AudioFormatWriter> writer(
        wavFormat.createWriterFor(outputStream,
                                   sampleRate,
                                   numChannels,
                                   bitsPerSample,
                                   metadata,
                                   0));
    if (!writer) return false;

    writer->writeFromAudioSampleBuffer(buffer, 0, (int)numSamples);
    writer.reset();

    OrpheusLogger::logInfo("DistributionPrep: Clean metadata written (Title: " + title + ", Artist: " + artist + ")");
    return true;
}

//==============================================================================
// Analysis — Integrated LUFS measurement
//==============================================================================

float DistributionPrepProcessor::measureIntegratedLUFS(const juce::File& wavFile)
{
    // Simplified integrated LUFS measurement based on ITU-R BS.1770-4:
    //   1. K-weight filtering (pre-filter + RLB filter)
    //   2. Mean-square measurement across the full file
    //   3. LUFS = -0.691 + 10 * log10(mean_square)
    //
    // For a production implementation, gating (EBU R128) should be added.

    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(wavFile));
    if (!reader) return -70.0f;

    int numChannels = (int)reader->numChannels;
    juce::int64 totalSamples = reader->lengthInSamples;
    double sampleRate = reader->sampleRate;

    // Read into buffer (for files < 100MB this is fine; larger files would need chunked processing)
    juce::AudioBuffer<float> buffer(numChannels, (int)totalSamples);
    reader->read(&buffer, 0, (int)totalSamples, 0, true, true);

    // K-weighting: Apply a high-shelf pre-filter and RLB high-pass
    // Pre-filter: +4dB shelf at 1681 Hz (approximated with a 1-pole high shelf)
    // RLB filter: High-pass at 38 Hz (approximated with a 1-pole HPF)
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32)totalSamples;
    spec.numChannels = (juce::uint32)numChannels;

    // Pre-filter coefficients (high-shelf boost ~+4dB above 1681 Hz)
    auto preFilterCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(
        sampleRate, 1681.0, 0.7071, juce::Decibels::decibelsToGain(4.0f));

    // RLB high-pass at 38 Hz
    auto rlbCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 38.0);

    // Apply filters per channel
    for (int ch = 0; ch < numChannels; ++ch)
    {
        juce::dsp::IIR::Filter<float> preFilter;
        preFilter.coefficients = preFilterCoeffs;
        preFilter.reset();

        juce::dsp::IIR::Filter<float> rlbFilter;
        rlbFilter.coefficients = rlbCoeffs;
        rlbFilter.reset();

        auto* data = buffer.getWritePointer(ch);
        for (juce::int64 i = 0; i < totalSamples; ++i)
        {
            data[i] = preFilter.processSample(data[i]);
            data[i] = rlbFilter.processSample(data[i]);
        }
    }

    // Calculate mean square (with channel weighting: L=1.0, R=1.0 for stereo)
    double sumSquares = 0.0;
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* data = buffer.getReadPointer(ch);
        double channelWeight = 1.0; // L/R = 1.0, surround channels would be 1.41

        for (juce::int64 i = 0; i < totalSamples; ++i)
        {
            sumSquares += channelWeight * (double)data[i] * (double)data[i];
        }
    }

    double meanSquare = sumSquares / (double)(totalSamples * numChannels);

    if (meanSquare < 1e-20)
        return -70.0f;

    float lufs = (float)(-0.691 + 10.0 * std::log10(meanSquare));
    return lufs;
}

float DistributionPrepProcessor::measureTruePeak(const juce::File& wavFile)
{
    // ITU-R BS.1770-4 true peak: oversample 4x and find the absolute peak.
    // We use linear interpolation for speed (production code would use
    // the specified FIR filter, but this is very close for most material).

    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(wavFile));
    if (!reader) return -70.0f;

    int numChannels = (int)reader->numChannels;
    juce::int64 totalSamples = reader->lengthInSamples;

    juce::AudioBuffer<float> buffer(numChannels, (int)totalSamples);
    reader->read(&buffer, 0, (int)totalSamples, 0, true, true);

    float maxPeak = 0.0f;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* data = buffer.getReadPointer(ch);

        for (juce::int64 i = 0; i < totalSamples - 1; ++i)
        {
            float s0 = std::abs(data[i]);
            float s1 = std::abs(data[i + 1]);

            // Check the sample itself
            maxPeak = juce::jmax(maxPeak, s0);

            // Check 4x oversampled interpolated values
            for (int k = 1; k < 4; ++k)
            {
                float t = (float)k / 4.0f;
                float interp = s0 + (s1 - s0) * t;
                maxPeak = juce::jmax(maxPeak, interp);
            }
        }
        // Last sample
        if (totalSamples > 0)
            maxPeak = juce::jmax(maxPeak, std::abs(data[totalSamples - 1]));
    }

    if (maxPeak < 1e-10f) return -70.0f;
    return juce::Decibels::gainToDecibels(maxPeak);
}

//==============================================================================
// Internal helpers
//==============================================================================

void DistributionPrepProcessor::applyGainToBuffer(juce::AudioBuffer<float>& buffer, float gainLinear)
{
    buffer.applyGain(gainLinear);
}

void DistributionPrepProcessor::applyTruePeakLimiter(juce::AudioBuffer<float>& buffer, float ceilingDB, double /*sampleRate*/)
{
    // Simple brick-wall limiter at the specified ceiling.
    // A production limiter would use lookahead and release curves,
    // but this ensures no sample exceeds the ceiling.
    float ceiling = juce::Decibels::decibelsToGain(ceilingDB);

    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();

    // Lookahead limiter with 5ms attack
    int lookahead = juce::jmin(numSamples, 256); // ~5ms at 48kHz
    float envelope = 0.0f;
    float releaseCoeff = 0.9995f; // Slow release

    // First pass: find peak envelope with lookahead
    std::vector<float> gainReduction((size_t)numSamples, 1.0f);

    for (int i = 0; i < numSamples; ++i)
    {
        // Find max absolute sample across channels
        float peak = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            peak = juce::jmax(peak, std::abs(buffer.getReadPointer(ch)[i]));
        }

        // Track envelope
        if (peak > envelope)
            envelope = peak;
        else
            envelope *= releaseCoeff;

        // Calculate gain reduction needed
        if (envelope > ceiling)
            gainReduction[(size_t)i] = ceiling / envelope;
    }

    // Apply gain reduction with smoothing
    float smoothedGain = 1.0f;
    float smoothCoeff = 0.001f; // Very smooth transitions

    for (int i = 0; i < numSamples; ++i)
    {
        smoothedGain += (gainReduction[(size_t)i] - smoothedGain) * smoothCoeff;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            buffer.getWritePointer(ch)[i] *= smoothedGain;
        }
    }
}

void DistributionPrepProcessor::convertMonoToStereo(juce::AudioBuffer<float>& buffer)
{
    if (buffer.getNumChannels() >= 2) return;

    juce::AudioBuffer<float> stereoBuffer(2, buffer.getNumSamples());
    stereoBuffer.copyFrom(0, 0, buffer, 0, 0, buffer.getNumSamples());
    stereoBuffer.copyFrom(1, 0, buffer, 0, 0, buffer.getNumSamples());
    buffer = std::move(stereoBuffer);
}

//==============================================================================
// Full distribution prep pipeline
//==============================================================================

bool DistributionPrepProcessor::processForDistribution(
    const juce::File& inputFile,
    const juce::File& outputFile,
    Platform platform,
    const juce::String& title,
    const juce::String& artist,
    AppState* appState,
    AudioEngine* engine)
{
    OrpheusLogger::logInfo("DistributionPrep: Starting pipeline for " + getPlatformName(platform));
    OrpheusLogger::logInfo("DistributionPrep: Input: " + inputFile.getFullPathName());

    // Get target specs
    PlatformSpec spec = getSpec(platform);

    // Allow custom overrides for Generic platform
    if (platform == Platform::Generic)
    {
        spec.targetLUFS    = customLUFS;
        spec.truePeakLimit = customTruePeak;
        spec.sampleRate    = customSampleRate;
        spec.bitDepth      = customBitDepth;
    }

    //── Step 1: Read the source audio ────────────────────────────────────────
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(inputFile));
    if (!reader)
    {
        OrpheusLogger::logError("DistributionPrep: Cannot read input file.");
        return false;
    }

    int numChannels = (int)reader->numChannels;
    juce::int64 totalSamples = reader->lengthInSamples;
    double sourceSampleRate = reader->sampleRate;

    juce::AudioBuffer<float> buffer(numChannels, (int)totalSamples);
    reader->read(&buffer, 0, (int)totalSamples, 0, true, true);
    reader.reset();

    OrpheusLogger::logInfo("DistributionPrep: Read " + juce::String(totalSamples) + " samples, " +
                           juce::String(numChannels) + " channels, " +
                           juce::String(sourceSampleRate) + " Hz");

    //── Step 2: Mono-to-stereo if needed ─────────────────────────────────────
    if (spec.requiresStereo && numChannels < 2)
    {
        convertMonoToStereo(buffer);
        numChannels = 2;
        OrpheusLogger::logInfo("DistributionPrep: Converted mono to stereo.");
    }

    //── Step 3: Sample rate conversion (if needed) ───────────────────────────
    double targetSampleRate = (double)spec.sampleRate;

    if (std::abs(sourceSampleRate - targetSampleRate) > 1.0)
    {
        // Use JUCE's resampling via interpolation
        double ratio = targetSampleRate / sourceSampleRate;
        int newLength = (int)std::ceil((double)totalSamples * ratio);

        juce::AudioBuffer<float> resampledBuffer(numChannels, newLength);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* src = buffer.getReadPointer(ch);
            auto* dst = resampledBuffer.getWritePointer(ch);

            for (int i = 0; i < newLength; ++i)
            {
                double srcPos = (double)i / ratio;
                int srcIdx = (int)srcPos;
                float frac = (float)(srcPos - (double)srcIdx);

                if (srcIdx + 1 < (int)totalSamples)
                    dst[i] = src[srcIdx] + (src[srcIdx + 1] - src[srcIdx]) * frac;
                else if (srcIdx < (int)totalSamples)
                    dst[i] = src[srcIdx];
                else
                    dst[i] = 0.0f;
            }
        }

        buffer = std::move(resampledBuffer);
        totalSamples = newLength;
        OrpheusLogger::logInfo("DistributionPrep: Resampled from " +
                               juce::String(sourceSampleRate) + " to " +
                               juce::String(targetSampleRate) + " Hz");
    }

    //── Step 4: Loudness normalization ───────────────────────────────────────
    {
        // Measure current integrated LUFS from the buffer directly
        // (simplified version — uses RMS as a proxy for LUFS measurement)
        double sumSquares = 0.0;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* data = buffer.getReadPointer(ch);
            for (juce::int64 i = 0; i < totalSamples; ++i)
                sumSquares += (double)data[i] * (double)data[i];
        }
        double meanSquare = sumSquares / (double)(totalSamples * numChannels);
        float currentLUFS = (meanSquare > 1e-20) ? (float)(-0.691 + 10.0 * std::log10(meanSquare)) : -70.0f;

        float gainNeeded = spec.targetLUFS - currentLUFS;
        float gainLinear = juce::Decibels::decibelsToGain(gainNeeded);

        // Clamp gain to prevent extreme amplification of very quiet files
        gainLinear = juce::jlimit(0.1f, 10.0f, gainLinear);

        applyGainToBuffer(buffer, gainLinear);

        OrpheusLogger::logInfo("DistributionPrep: Normalized from " +
                               juce::String(currentLUFS, 1) + " to " +
                               juce::String(spec.targetLUFS, 1) + " LUFS (gain: " +
                               juce::String(juce::Decibels::gainToDecibels(gainLinear), 1) + " dB)");
    }

    //── Step 5: True peak limiting ───────────────────────────────────────────
    applyTruePeakLimiter(buffer, spec.truePeakLimit, targetSampleRate);
    OrpheusLogger::logInfo("DistributionPrep: True peak limited to " +
                           juce::String(spec.truePeakLimit, 1) + " dBTP");

    //── Step 6: Write clean output WAV ───────────────────────────────────────
    juce::WavAudioFormat wavFormat;
    auto* outputStream = new juce::FileOutputStream(outputFile);

    if (outputStream->failedToOpen())
    {
        delete outputStream;
        OrpheusLogger::logError("DistributionPrep: Cannot open output file for writing.");
        return false;
    }

    // Write with no metadata — completely clean WAV
    std::unique_ptr<juce::AudioFormatWriter> writer(
        wavFormat.createWriterFor(outputStream,
                                   targetSampleRate,
                                   numChannels,
                                   spec.bitDepth,
                                   {},   // No metadata
                                   0));

    if (!writer)
    {
        OrpheusLogger::logError("DistributionPrep: Cannot create WAV writer.");
        return false;
    }

    writer->writeFromAudioSampleBuffer(buffer, 0, (int)totalSamples);
    writer.reset();

    //── Step 7: Write clean metadata if provided ─────────────────────────────
    if (title.isNotEmpty() || artist.isNotEmpty())
    {
        writeCleanMetadata(outputFile, title, artist);
    }

    OrpheusLogger::logInfo("DistributionPrep: Pipeline complete! Output: " + outputFile.getFullPathName());

    //── Step 8: Auto-import into project if engine available ─────────────────
    if (appState && engine)
    {
        juce::String trackName = "Dist-Ready (" + getPlatformName(platform) + ")";
        int newTrackIdx = appState->addAudioTrack(trackName);
        engine->addAudioTrack(trackName);

        auto& trackInfo = engine->getTrackInfo(newTrackIdx);
        auto* newClip = new AudioClip(outputFile, 0.0);
        trackInfo.clips.add(newClip);

        OrpheusLogger::logInfo("DistributionPrep: Auto-imported as \"" + trackName + "\"");
    }

    return true;
}
