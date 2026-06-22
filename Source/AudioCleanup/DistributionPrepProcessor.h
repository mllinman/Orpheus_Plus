#pragma once
#include <JuceHeader.h>

class AppState;
class AudioEngine;

//==============================================================================
// DistributionPrepProcessor — Metadata stripping, loudness normalization,
// true peak limiting, and format compliance for streaming platforms.
//==============================================================================
class DistributionPrepProcessor
{
public:
    DistributionPrepProcessor();
    ~DistributionPrepProcessor();

    //── Platform presets ──────────────────────────────────────────────────────
    enum class Platform { Spotify, AppleMusic, YouTube, Generic };

    struct PlatformSpec
    {
        float  targetLUFS    { -14.0f };
        float  truePeakLimit { -1.0f };   // dBTP
        int    sampleRate    { 44100 };
        int    bitDepth      { 24 };
        bool   requiresStereo { true };
    };

    static PlatformSpec getSpec(Platform platform);
    static juce::String getPlatformName(Platform platform);

    //── Metadata operations ──────────────────────────────────────────────────

    // Strip all non-audio metadata from a WAV file (INFO, LIST, BEXT, iXML, id3, etc.)
    // Rewrites only the fmt + data chunks into a clean WAV.
    static bool stripAllMetadata(const juce::File& inputFile, const juce::File& outputFile);

    // Write minimal clean metadata (RIFF INFO title + artist only)
    static bool writeCleanMetadata(const juce::File& wavFile,
                                    const juce::String& title,
                                    const juce::String& artist);

    //── Analysis ─────────────────────────────────────────────────────────────

    // Measure integrated LUFS of a WAV file
    static float measureIntegratedLUFS(const juce::File& wavFile);

    // Measure true peak in dBTP
    static float measureTruePeak(const juce::File& wavFile);

    //── Full distribution prep pipeline ──────────────────────────────────────

    // Runs the full pipeline:
    //   1. Strip metadata
    //   2. Normalize loudness to platform target LUFS
    //   3. Apply true peak limiter
    //   4. Validate/convert sample rate and bit depth
    //   5. Ensure stereo
    // Returns true on success.
    bool processForDistribution(const juce::File& inputFile,
                                 const juce::File& outputFile,
                                 Platform platform,
                                 const juce::String& title = {},
                                 const juce::String& artist = {},
                                 AppState* appState = nullptr,
                                 AudioEngine* engine = nullptr);

    //── Custom overrides (for the "Generic" platform) ────────────────────────
    void setCustomTargetLUFS(float lufs)     { customLUFS = lufs; }
    void setCustomTruePeak(float dbtp)        { customTruePeak = dbtp; }
    void setCustomSampleRate(int sr)          { customSampleRate = sr; }
    void setCustomBitDepth(int bd)            { customBitDepth = bd; }

private:
    float customLUFS       { -14.0f };
    float customTruePeak   { -1.0f };
    int   customSampleRate { 44100 };
    int   customBitDepth   { 24 };

    // Internal helpers
    static void applyGainToBuffer(juce::AudioBuffer<float>& buffer, float gainLinear);
    static void applyTruePeakLimiter(juce::AudioBuffer<float>& buffer, float ceilingDB, double sampleRate);
    static void convertMonoToStereo(juce::AudioBuffer<float>& buffer);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DistributionPrepProcessor)
};
