#include "TrackElementClassifier.h"
#include <algorithm>
#include <numeric>

//==============================================================================
// Public API
//==============================================================================

TrackElementClassifier::ClassificationResult
TrackElementClassifier::classify(const juce::String& trackName,
                                  const juce::AudioBuffer<float>& buffer,
                                  double sampleRate)
{
    if (buffer.getNumSamples() == 0)
    {
        ClassificationResult r;
        r.type = InstrumentType::Other;
        r.confidence = 0.0f;
        r.description = "Empty buffer — no classification possible.";
        return r;
    }

    const float* data = buffer.getReadPointer(0);
    int numSamples = buffer.getNumSamples();

    // ── Extract features ──
    ClassificationResult features;
    features.spectralCentroid     = computeSpectralCentroid(data, numSamples, sampleRate);
    features.transientDensity     = computeTransientDensity(data, numSamples, sampleRate);
    features.harmonicToNoiseRatio = computeHarmonicToNoiseRatio(data, numSamples, sampleRate);
    features.rmsLevel             = computeRMS(data, numSamples);
    features.peakLevel            = computePeak(data, numSamples);
    features.crestFactor          = (features.rmsLevel > 0.0001f)
                                      ? features.peakLevel / features.rmsLevel : 0.0f;
    computeBandEnergies(data, numSamples, sampleRate,
                        features.lowBandEnergy, features.midBandEnergy, features.highBandEnergy);

    // ── Classify from spectral features ──
    auto spectralResult = classifyFromFeatures(features);

    // ── Classify from track name ──
    auto namedResult = classifyFromName(trackName);

    // ── Merge ──
    return mergeResults(spectralResult, namedResult);
}

std::vector<TrackElementClassifier::ClassificationResult>
TrackElementClassifier::classifyAll(
    const std::vector<std::pair<juce::String, juce::AudioBuffer<float>>>& tracks,
    double sampleRate)
{
    std::vector<ClassificationResult> results;
    results.reserve(tracks.size());
    for (auto& [name, buffer] : tracks)
        results.push_back(classify(name, buffer, sampleRate));
    return results;
}

//==============================================================================
// Feature Extraction
//==============================================================================

float TrackElementClassifier::computeSpectralCentroid(const float* data, int numSamples, double sampleRate)
{
    // Average spectral centroid over overlapping FFT frames
    double centroidSum = 0.0;
    int frameCount = 0;
    int hopSize = fftSize / 2;

    for (int offset = 0; offset + fftSize <= numSamples; offset += hopSize)
    {
        // Fill FFT buffer with Hann window
        for (int i = 0; i < fftSize; ++i)
        {
            float window = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * i / (float)(fftSize - 1)));
            fftData[(size_t)(i * 2)]     = data[offset + i] * window;
            fftData[(size_t)(i * 2 + 1)] = 0.0f;
        }

        fft.performFrequencyOnlyForwardTransform(fftData.data());

        // Compute centroid: sum(f_k * |X_k|) / sum(|X_k|)
        double weightedSum = 0.0;
        double magnitudeSum = 0.0;
        int numBins = fftSize / 2;

        for (int k = 1; k < numBins; ++k)
        {
            float magnitude = fftData[(size_t)k];
            double freq = (double)k * sampleRate / (double)fftSize;
            weightedSum += freq * magnitude;
            magnitudeSum += magnitude;
        }

        if (magnitudeSum > 0.0001)
        {
            centroidSum += weightedSum / magnitudeSum;
            frameCount++;
        }
    }

    return (frameCount > 0) ? (float)(centroidSum / frameCount) : 0.0f;
}

float TrackElementClassifier::computeTransientDensity(const float* data, int numSamples, double sampleRate)
{
    // Onset detection via spectral flux
    // Compare energy of successive short windows
    int windowSize = (int)(sampleRate * 0.01);  // 10ms windows
    if (windowSize < 1) windowSize = 1;

    std::vector<float> energyFrames;
    for (int i = 0; i + windowSize <= numSamples; i += windowSize)
    {
        float energy = 0.0f;
        for (int j = 0; j < windowSize; ++j)
            energy += data[i + j] * data[i + j];
        energyFrames.push_back(energy / (float)windowSize);
    }

    // Count positive flux peaks above threshold
    if (energyFrames.size() < 3) return 0.0f;

    // Compute spectral flux
    std::vector<float> flux;
    for (size_t i = 1; i < energyFrames.size(); ++i)
    {
        float diff = energyFrames[i] - energyFrames[i - 1];
        flux.push_back(juce::jmax(0.0f, diff));  // Half-wave rectified
    }

    // Adaptive threshold: mean + 1.5 * stddev
    float fluxMean = 0.0f;
    for (float f : flux) fluxMean += f;
    fluxMean /= (float)flux.size();

    float fluxVar = 0.0f;
    for (float f : flux) fluxVar += (f - fluxMean) * (f - fluxMean);
    fluxVar /= (float)flux.size();
    float fluxStdDev = std::sqrt(fluxVar);

    float threshold = fluxMean + 1.5f * fluxStdDev;

    int onsetCount = 0;
    bool wasBelowThreshold = true;
    for (float f : flux)
    {
        if (f > threshold && wasBelowThreshold)
        {
            onsetCount++;
            wasBelowThreshold = false;
        }
        else if (f <= threshold)
        {
            wasBelowThreshold = true;
        }
    }

    double durationSeconds = (double)numSamples / sampleRate;
    return (durationSeconds > 0.0) ? (float)(onsetCount / durationSeconds) : 0.0f;
}

float TrackElementClassifier::computeHarmonicToNoiseRatio(const float* data, int numSamples, double sampleRate)
{
    // Simplified HNR via autocorrelation
    // High HNR = strongly harmonic (vocals, guitar, piano)
    // Low HNR  = noisy/transient (drums, percussion)
    int maxLag = (int)(sampleRate / 60.0);   // Lowest pitch ~60 Hz
    int minLag = (int)(sampleRate / 1000.0); // Highest pitch ~1000 Hz
    if (maxLag >= numSamples) maxLag = numSamples - 1;
    if (minLag < 1) minLag = 1;

    // Only analyze a portion to save CPU
    int analysisLen = juce::jmin(numSamples, (int)(sampleRate * 0.5)); // Max 500ms

    // Autocorrelation at lag 0 (total energy)
    double r0 = 0.0;
    for (int i = 0; i < analysisLen; ++i)
        r0 += (double)data[i] * data[i];

    if (r0 < 0.00001) return 0.0f;

    // Find peak autocorrelation in the pitch range
    double maxR = 0.0;
    for (int lag = minLag; lag <= juce::jmin(maxLag, analysisLen - 1); ++lag)
    {
        double r = 0.0;
        int end = analysisLen - lag;
        for (int i = 0; i < end; ++i)
            r += (double)data[i] * data[i + lag];
        r /= (double)end; // Normalize by frame length

        if (r > maxR) maxR = r;
    }

    // Normalize r0 too
    double r0Norm = r0 / (double)analysisLen;

    // HNR = 10 * log10(maxR / (r0Norm - maxR))
    double noise = r0Norm - maxR;
    if (noise < 0.000001) return 30.0f; // Very harmonic
    double hnr = 10.0 * std::log10(maxR / noise);
    return (float)juce::jlimit(-10.0, 30.0, hnr);
}

void TrackElementClassifier::computeBandEnergies(const float* data, int numSamples, double sampleRate,
                                                  float& lowBand, float& midBand, float& highBand)
{
    // Multi-band energy distribution via FFT
    double lowSum = 0.0, midSum = 0.0, highSum = 0.0;
    int frameCount = 0;
    int hopSize = fftSize / 2;

    int lowBinCutoff = (int)(300.0 * fftSize / sampleRate);
    int highBinCutoff = (int)(4000.0 * fftSize / sampleRate);
    int numBins = fftSize / 2;

    for (int offset = 0; offset + fftSize <= numSamples; offset += hopSize)
    {
        for (int i = 0; i < fftSize; ++i)
        {
            float window = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * i / (float)(fftSize - 1)));
            fftData[(size_t)(i * 2)]     = data[offset + i] * window;
            fftData[(size_t)(i * 2 + 1)] = 0.0f;
        }

        fft.performFrequencyOnlyForwardTransform(fftData.data());

        for (int k = 1; k < numBins; ++k)
        {
            float mag = fftData[(size_t)k] * fftData[(size_t)k]; // Power
            if (k < lowBinCutoff)
                lowSum += mag;
            else if (k < highBinCutoff)
                midSum += mag;
            else
                highSum += mag;
        }
        frameCount++;
    }

    double total = lowSum + midSum + highSum;
    if (total < 0.00001 || frameCount == 0)
    {
        lowBand = midBand = highBand = 0.33f;
        return;
    }

    lowBand  = (float)(lowSum / total);
    midBand  = (float)(midSum / total);
    highBand = (float)(highSum / total);
}

float TrackElementClassifier::computeRMS(const float* data, int numSamples)
{
    float sum = 0.0f;
    for (int i = 0; i < numSamples; ++i)
        sum += data[i] * data[i];
    return std::sqrt(sum / (float)numSamples);
}

float TrackElementClassifier::computePeak(const float* data, int numSamples)
{
    float peak = 0.0f;
    for (int i = 0; i < numSamples; ++i)
        peak = juce::jmax(peak, std::abs(data[i]));
    return peak;
}

//==============================================================================
// Classification Logic
//==============================================================================

TrackElementClassifier::ClassificationResult
TrackElementClassifier::classifyFromFeatures(const ClassificationResult& f)
{
    ClassificationResult result = f; // Copy all spectral features

    // Decision tree based on extracted features
    // Priority order: Drums/Percussion → Bass → Vocals → Guitars → Piano → Synth → Other

    // ── DRUMS: High transient density, low HNR, broad spectrum ──
    if (f.transientDensity > 4.0f && f.harmonicToNoiseRatio < 5.0f)
    {
        result.type = InstrumentType::Drums;
        result.confidence = juce::jmin(1.0f, 0.5f + f.transientDensity * 0.05f);
        result.description = "High transient density with low harmonicity — percussive content.";

        if (f.lowBandEnergy > 0.5f)
            result.subType = "Kick-heavy";
        else if (f.highBandEnergy > 0.4f)
            result.subType = "Hi-hat/Cymbal-heavy";
        else
            result.subType = "Full Kit";
        return result;
    }

    // ── PERCUSSION: Moderate transients, low HNR, less regular than drums ──
    if (f.transientDensity > 2.5f && f.harmonicToNoiseRatio < 8.0f
        && f.crestFactor > 5.0f)
    {
        result.type = InstrumentType::Percussion;
        result.confidence = 0.6f;
        result.description = "Moderate transients with low harmonicity — likely percussion.";
        result.subType = "Auxiliary Percussion";
        return result;
    }

    // ── BASS: Most energy below 300 Hz, low spectral centroid ──
    if (f.lowBandEnergy > 0.55f && f.spectralCentroid < 500.0f)
    {
        result.type = InstrumentType::Bass;
        result.confidence = juce::jmin(1.0f, 0.5f + f.lowBandEnergy);

        if (f.harmonicToNoiseRatio > 15.0f)
        {
            result.subType = "Synth Bass";
            result.description = "Strong sub-bass energy with clean harmonics — synthesized bass.";
        }
        else
        {
            result.subType = "Electric/Upright Bass";
            result.description = "Dominant low-frequency energy — bass instrument.";
        }
        return result;
    }

    // ── VOCALS: Mid-range centroid (1–4 kHz), high HNR, formant presence ──
    if (f.spectralCentroid > 800.0f && f.spectralCentroid < 4500.0f
        && f.harmonicToNoiseRatio > 12.0f
        && f.midBandEnergy > 0.4f)
    {
        result.type = InstrumentType::Vocals;
        result.confidence = juce::jmin(1.0f, 0.4f + f.harmonicToNoiseRatio * 0.03f);

        if (f.spectralCentroid > 2500.0f)
            result.subType = "Female/High Vocal";
        else
            result.subType = "Male/Low Vocal";

        result.description = "Strong mid-range harmonics with high HNR — vocal content.";
        return result;
    }

    // ── ELECTRIC GUITAR: Mid-range centroid (800–3.5 kHz), moderate HNR, distortion harmonics ──
    if (f.spectralCentroid > 700.0f && f.spectralCentroid < 3500.0f
        && f.harmonicToNoiseRatio > 5.0f && f.harmonicToNoiseRatio < 18.0f
        && f.highBandEnergy > 0.15f)
    {
        result.type = InstrumentType::ElectricGuitar;
        result.confidence = 0.6f;

        if (f.harmonicToNoiseRatio < 10.0f)
            result.subType = "Distorted/Overdriven";
        else
            result.subType = "Clean Electric";

        result.description = "Mid-range dominant with moderate harmonicity — electric guitar character.";
        return result;
    }

    // ── ACOUSTIC GUITAR: Centroid 400–2 kHz, pluck transients, clean harmonics ──
    if (f.spectralCentroid > 400.0f && f.spectralCentroid < 2200.0f
        && f.harmonicToNoiseRatio > 10.0f
        && f.transientDensity > 1.5f && f.transientDensity < 6.0f)
    {
        result.type = InstrumentType::AcousticGuitar;
        result.confidence = 0.55f;
        result.subType = "Steel/Nylon String";
        result.description = "Moderate spectral centroid with pluck transients — acoustic guitar.";
        return result;
    }

    // ── PIANO: Wide spectral spread, clear attacks, strong harmonic series ──
    if (f.harmonicToNoiseRatio > 12.0f
        && f.transientDensity > 1.0f && f.transientDensity < 8.0f
        && f.lowBandEnergy > 0.15f && f.highBandEnergy > 0.1f)
    {
        // Piano typically has a very even distribution across bands
        float bandVariance = std::abs(f.lowBandEnergy - f.midBandEnergy)
                           + std::abs(f.midBandEnergy - f.highBandEnergy);
        if (bandVariance < 0.35f)
        {
            result.type = InstrumentType::Piano;
            result.confidence = 0.55f;
            result.subType = "Acoustic Piano";
            result.description = "Wide spectral spread with clean harmonic attacks — piano.";
            return result;
        }
    }

    // ── SYNTH: Catch-all for tonal content that doesn't match above ──
    if (f.harmonicToNoiseRatio > 8.0f && f.spectralCentroid > 200.0f)
    {
        result.type = InstrumentType::Synth;
        result.confidence = 0.45f;

        if (f.spectralCentroid < 800.0f)
            result.subType = "Pad/Warm Synth";
        else if (f.spectralCentroid > 3000.0f)
            result.subType = "Lead/Bright Synth";
        else
            result.subType = "General Synth";

        result.description = "Tonal content with synthetic harmonic characteristics.";
        return result;
    }

    // ── OTHER: Unclassified ──
    result.type = InstrumentType::Other;
    result.confidence = 0.3f;
    result.description = "Could not confidently classify instrument type.";
    return result;
}

TrackElementClassifier::ClassificationResult
TrackElementClassifier::classifyFromName(const juce::String& trackName)
{
    ClassificationResult result;
    auto name = trackName.toLowerCase();

    // Vocals
    if (name.contains("vocal") || name.contains("vox") || name.contains("voice")
        || name.contains("sing") || name.contains("lead voc") || name.contains("bgv")
        || name.contains("choir") || name.contains("harmony"))
    {
        result.type = InstrumentType::Vocals;
        result.confidence = 0.85f;
        if (name.contains("lead"))       result.subType = "Lead Vocal";
        else if (name.contains("bgv") || name.contains("back") || name.contains("harmony"))
            result.subType = "Background Vocal";
        else result.subType = "Vocal";
        return result;
    }

    // Drums
    if (name.contains("drum") || name.contains("kick") || name.contains("snare")
        || name.contains("hi-hat") || name.contains("hihat") || name.contains("hh")
        || name.contains("cymbal") || name.contains("tom") || name.contains("overhead")
        || name.contains("oh") || name.contains("room"))
    {
        result.type = InstrumentType::Drums;
        result.confidence = 0.85f;
        if (name.contains("kick"))       result.subType = "Kick";
        else if (name.contains("snare")) result.subType = "Snare";
        else if (name.contains("hat"))   result.subType = "Hi-Hat";
        else                             result.subType = "Drums";
        return result;
    }

    // Bass
    if (name.contains("bass") || name.contains("sub") || name.contains("808"))
    {
        result.type = InstrumentType::Bass;
        result.confidence = 0.85f;
        if (name.contains("synth") || name.contains("808"))
            result.subType = "Synth Bass";
        else result.subType = "Bass";
        return result;
    }

    // Guitar
    if (name.contains("guitar") || name.contains("gtr") || name.contains("guit"))
    {
        if (name.contains("elec") || name.contains("dist") || name.contains("overdrive")
            || name.contains("clean elec") || name.contains("e.gtr") || name.contains("egtr"))
        {
            result.type = InstrumentType::ElectricGuitar;
            result.confidence = 0.85f;
            result.subType = "Electric Guitar";
        }
        else if (name.contains("acou") || name.contains("a.gtr") || name.contains("agtr")
                 || name.contains("nylon") || name.contains("steel"))
        {
            result.type = InstrumentType::AcousticGuitar;
            result.confidence = 0.85f;
            result.subType = "Acoustic Guitar";
        }
        else
        {
            result.type = InstrumentType::ElectricGuitar; // Default to electric
            result.confidence = 0.7f;
            result.subType = "Guitar";
        }
        return result;
    }

    // Synth
    if (name.contains("synth") || name.contains("pad") || name.contains("lead")
        || name.contains("arp") || name.contains("keys"))
    {
        result.type = InstrumentType::Synth;
        result.confidence = 0.8f;
        if (name.contains("pad"))        result.subType = "Pad";
        else if (name.contains("lead"))  result.subType = "Lead";
        else if (name.contains("arp"))   result.subType = "Arpeggio";
        else                             result.subType = "Synth";
        return result;
    }

    // Piano
    if (name.contains("piano") || name.contains("pno") || name.contains("rhodes")
        || name.contains("wurlitzer") || name.contains("ep"))
    {
        result.type = InstrumentType::Piano;
        result.confidence = 0.85f;
        if (name.contains("rhodes") || name.contains("wurlitzer") || name.contains("ep"))
            result.subType = "Electric Piano";
        else result.subType = "Piano";
        return result;
    }

    // Percussion
    if (name.contains("perc") || name.contains("shaker") || name.contains("tamb")
        || name.contains("conga") || name.contains("bongo") || name.contains("clap"))
    {
        result.type = InstrumentType::Percussion;
        result.confidence = 0.8f;
        result.subType = "Percussion";
        return result;
    }

    // No match from name
    result.type = InstrumentType::Other;
    result.confidence = 0.0f; // Zero = name provided no info
    return result;
}

TrackElementClassifier::ClassificationResult
TrackElementClassifier::mergeResults(const ClassificationResult& spectral,
                                      const ClassificationResult& named)
{
    // If name gave a high-confidence match, trust it and overlay spectral features
    if (named.confidence >= 0.7f)
    {
        ClassificationResult merged = spectral; // Keep all spectral features
        merged.type       = named.type;
        merged.subType    = named.subType;
        merged.confidence = juce::jmin(1.0f, named.confidence * 0.6f + spectral.confidence * 0.4f);
        merged.description = "Classified by track name (\"" + named.subType + "\") "
                           + "with spectral confirmation.";
        return merged;
    }

    // If name gave a moderate match, boost confidence if spectral agrees
    if (named.confidence > 0.0f && named.type == spectral.type)
    {
        ClassificationResult merged = spectral;
        merged.confidence = juce::jmin(1.0f, spectral.confidence + 0.2f);
        merged.subType = named.subType.isNotEmpty() ? named.subType : spectral.subType;
        merged.description = spectral.description + " (Name confirms classification.)";
        return merged;
    }

    // Otherwise trust spectral analysis
    return spectral;
}
