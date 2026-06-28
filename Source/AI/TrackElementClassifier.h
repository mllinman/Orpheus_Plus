#pragma once
#include <JuceHeader.h>
#include <vector>
#include <cmath>
#include <array>

/**
 * TrackElementClassifier
 *
 * Identifies the instrument role of an audio track using multi-band
 * spectral analysis, transient density, harmonic-to-noise ratio (HNR),
 * and optional track-name hinting.
 *
 * This is the core intelligence behind the Smart Track Fixer —
 * all downstream EQ, dynamics, and stereo decisions branch from the
 * InstrumentType returned here.
 */
class TrackElementClassifier
{
public:
    //──────────────────────────────────────────────────────────────────────
    // Instrument taxonomy
    //──────────────────────────────────────────────────────────────────────
    enum class InstrumentType
    {
        Vocals,
        Drums,
        Bass,
        ElectricGuitar,
        AcousticGuitar,
        Synth,
        Piano,
        Percussion,
        Other
    };

    static juce::String instrumentTypeToString(InstrumentType t)
    {
        switch (t)
        {
            case InstrumentType::Vocals:          return "Vocals";
            case InstrumentType::Drums:           return "Drums";
            case InstrumentType::Bass:            return "Bass";
            case InstrumentType::ElectricGuitar:   return "Electric Guitar";
            case InstrumentType::AcousticGuitar:   return "Acoustic Guitar";
            case InstrumentType::Synth:           return "Synth";
            case InstrumentType::Piano:           return "Piano";
            case InstrumentType::Percussion:      return "Percussion";
            case InstrumentType::Other:           return "Other";
        }
        return "Unknown";
    }

    static juce::Colour instrumentColour(InstrumentType t)
    {
        switch (t)
        {
            case InstrumentType::Vocals:          return juce::Colour(0xffE06C75);  // coral red
            case InstrumentType::Drums:           return juce::Colour(0xffD19A66);  // warm amber
            case InstrumentType::Bass:            return juce::Colour(0xff61AFEF);  // cool blue
            case InstrumentType::ElectricGuitar:   return juce::Colour(0xffC678DD);  // purple
            case InstrumentType::AcousticGuitar:   return juce::Colour(0xffE5C07B);  // gold
            case InstrumentType::Synth:           return juce::Colour(0xff56B6C2);  // teal
            case InstrumentType::Piano:           return juce::Colour(0xff98C379);  // green
            case InstrumentType::Percussion:      return juce::Colour(0xffBE5046);  // deep red
            case InstrumentType::Other:           return juce::Colour(0xff5C6370);  // muted grey
        }
        return juce::Colours::grey;
    }

    //──────────────────────────────────────────────────────────────────────
    // Classification result
    //──────────────────────────────────────────────────────────────────────
    struct ClassificationResult
    {
        InstrumentType type          = InstrumentType::Other;
        float          confidence    = 0.0f;   // 0.0 – 1.0
        juce::String   subType;                // e.g. "Kick", "Snare", "Lead Vocal", "Pad"
        juce::String   description;            // Human-readable summary

        // Raw spectral features (useful for downstream processing)
        float spectralCentroid       = 0.0f;   // Hz
        float transientDensity       = 0.0f;   // Transients per second
        float harmonicToNoiseRatio   = 0.0f;   // dB
        float rmsLevel               = 0.0f;   // Linear
        float peakLevel              = 0.0f;   // Linear
        float crestFactor            = 0.0f;   // peak / rms
        float lowBandEnergy          = 0.0f;   // 0-1 proportion of energy below 300 Hz
        float midBandEnergy          = 0.0f;   // 0-1 proportion of energy 300 Hz–4 kHz
        float highBandEnergy         = 0.0f;   // 0-1 proportion of energy above 4 kHz
    };

    //──────────────────────────────────────────────────────────────────────
    // API
    //──────────────────────────────────────────────────────────────────────
    TrackElementClassifier() = default;
    ~TrackElementClassifier() = default;

    /**
     * Classify a single track from its audio buffer.
     * @param trackName  The user-visible name (used for name-based hinting)
     * @param buffer     The audio data (mono or stereo; channel 0 is analyzed)
     * @param sampleRate The sample rate of the audio
     */
    ClassificationResult classify(const juce::String& trackName,
                                  const juce::AudioBuffer<float>& buffer,
                                  double sampleRate);

    /**
     * Classify all tracks in a batch.
     * Returns results in the same order as the input vector.
     */
    std::vector<ClassificationResult> classifyAll(
        const std::vector<std::pair<juce::String, juce::AudioBuffer<float>>>& tracks,
        double sampleRate);

private:
    //── Feature extraction ──────────────────────────────────────────────
    float computeSpectralCentroid(const float* data, int numSamples, double sampleRate);
    float computeTransientDensity(const float* data, int numSamples, double sampleRate);
    float computeHarmonicToNoiseRatio(const float* data, int numSamples, double sampleRate);
    void  computeBandEnergies(const float* data, int numSamples, double sampleRate,
                              float& lowBand, float& midBand, float& highBand);
    float computeRMS(const float* data, int numSamples);
    float computePeak(const float* data, int numSamples);

    //── Classification logic ────────────────────────────────────────────
    ClassificationResult classifyFromFeatures(const ClassificationResult& features);
    ClassificationResult classifyFromName(const juce::String& trackName);
    ClassificationResult mergeResults(const ClassificationResult& spectral,
                                       const ClassificationResult& named);

    //── FFT workspace ───────────────────────────────────────────────────
    static constexpr int fftOrder = 11;                      // 2048-point FFT
    static constexpr int fftSize  = 1 << fftOrder;           // 2048
    juce::dsp::FFT fft { fftOrder };
    std::array<float, fftSize * 2> fftData {};
};
