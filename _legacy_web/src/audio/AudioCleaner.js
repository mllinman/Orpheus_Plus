// ============================================
// ORPHEUS DAW — Audio Cleanup Engine
// ============================================
// Professional audio cleanup: pop/click removal, decrackle,
// artifact smoothing, humanization, DC offset removal

export class AudioCleaner {

    /**
     * Full cleanup pipeline — runs selected processors on an AudioBuffer
     * @param {AudioContext} ctx
     * @param {AudioBuffer} buffer - Source buffer to clean
     * @param {Object} options
     * @returns {{ cleaned: AudioBuffer, stats: Object }}
     */
    static async clean(ctx, buffer, options = {}) {
        const {
            popRemoval = true,
            decrackle = true,
            artifactSmoothing = true,
            humanize = false,
            dcOffset = true,
            crossfadeRepair = true,
            strength = 0.5,  // 0-1: light to aggressive
        } = options;

        const stats = { popsRemoved: 0, cracklesFixed: 0, artifactsSmoothed: 0, dcCorrected: false };

        // Work on a copy
        const cleaned = ctx.createBuffer(
            buffer.numberOfChannels,
            buffer.length,
            buffer.sampleRate
        );

        for (let ch = 0; ch < buffer.numberOfChannels; ch++) {
            const src = buffer.getChannelData(ch);
            const dst = cleaned.getChannelData(ch);
            dst.set(src); // copy

            // Pipeline order matters: DC → Pops → Crackles → Artifacts → Crossfade → Humanize
            if (dcOffset) {
                const corrected = AudioCleaner._removeDCOffset(dst);
                if (corrected) stats.dcCorrected = true;
            }

            if (popRemoval) {
                stats.popsRemoved += AudioCleaner._removePopClicks(dst, strength, buffer.sampleRate);
            }

            if (decrackle) {
                stats.cracklesFixed += AudioCleaner._decrackle(dst, strength, buffer.sampleRate);
            }

            if (artifactSmoothing) {
                stats.artifactsSmoothed += AudioCleaner._smoothArtifacts(dst, strength, buffer.sampleRate);
            }

            if (crossfadeRepair) {
                AudioCleaner._repairCrossfades(dst, buffer.sampleRate);
            }

            if (humanize) {
                AudioCleaner._humanize(dst, strength, buffer.sampleRate);
            }
        }

        return { cleaned, stats };
    }

    // ─── DC Offset Removal ───
    // Subtracts mean sample value to center waveform at zero
    static _removeDCOffset(samples) {
        let sum = 0;
        for (let i = 0; i < samples.length; i++) sum += samples[i];
        const mean = sum / samples.length;

        if (Math.abs(mean) < 0.0001) return false; // Already centered

        for (let i = 0; i < samples.length; i++) {
            samples[i] -= mean;
        }
        return true;
    }

    // ─── Pop / Click Removal ───
    // Detects amplitude spikes > 3σ from local mean, replaces with interpolated values
    static _removePopClicks(samples, strength, sampleRate) {
        const windowSize = Math.floor(sampleRate * 0.002); // 2ms analysis window
        const threshold = 3.0 - strength * 1.5; // More aggressive = lower threshold
        let popsFixed = 0;

        // Calculate running statistics
        for (let i = windowSize; i < samples.length - windowSize; i++) {
            // Local mean and std dev
            let localSum = 0, localSumSq = 0;
            for (let j = i - windowSize; j < i + windowSize; j++) {
                localSum += samples[j];
                localSumSq += samples[j] * samples[j];
            }
            const count = windowSize * 2;
            const localMean = localSum / count;
            const localStd = Math.sqrt(Math.max(0, localSumSq / count - localMean * localMean));

            // Detect spike
            if (localStd > 0.0001 && Math.abs(samples[i] - localMean) > threshold * localStd) {
                // Interpolate: weighted average of neighbors
                const repairWindow = Math.min(windowSize, 32);
                const left = i > repairWindow ? samples[i - repairWindow] : samples[i];
                const right = i < samples.length - repairWindow ? samples[i + repairWindow] : samples[i];

                // Smooth interpolation across the spike
                for (let k = -repairWindow / 2; k <= repairWindow / 2; k++) {
                    const idx = i + k;
                    if (idx >= 0 && idx < samples.length) {
                        const t = (k + repairWindow / 2) / repairWindow;
                        // Cosine interpolation for natural transition
                        const blend = 0.5 * (1 - Math.cos(t * Math.PI));
                        samples[idx] = left * (1 - blend) + right * blend;
                    }
                }
                popsFixed++;
                i += repairWindow; // Skip repaired region
            }
        }

        return popsFixed;
    }

    // ─── Decrackle ───
    // Median filter on short windows to smooth micro-discontinuities
    static _decrackle(samples, strength, sampleRate) {
        const filterSize = Math.max(3, Math.floor(strength * 7) | 1); // Odd number 3-7
        const crackleThreshold = 0.01 + (1 - strength) * 0.05;
        let cracklesFixed = 0;

        // Detect and fix micro-discontinuities
        const temp = new Float32Array(samples.length);
        temp.set(samples);

        const halfFilter = Math.floor(filterSize / 2);
        const windowBuf = new Float32Array(filterSize);

        for (let i = halfFilter; i < samples.length - halfFilter; i++) {
            // Check for sudden derivative change (crackle signature)
            const derivBefore = Math.abs(temp[i] - temp[i - 1]);
            const derivAfter = Math.abs(temp[i + 1] - temp[i]);
            const derivRatio = Math.max(derivBefore, derivAfter) / (Math.min(derivBefore, derivAfter) + 0.00001);

            if (derivRatio > 3 + (1 - strength) * 5) {
                // Apply median filter to this region
                for (let j = 0; j < filterSize; j++) {
                    windowBuf[j] = temp[i - halfFilter + j];
                }
                // Sort for median
                windowBuf.sort();
                samples[i] = windowBuf[halfFilter];
                cracklesFixed++;
            }
        }

        return cracklesFixed;
    }

    // ─── Artifact Smoothing ───
    // FFT-based spectral smoothing to remove digital artifacts
    static _smoothArtifacts(samples, strength, sampleRate) {
        const fftSize = 2048;
        const hopSize = fftSize / 4;
        const smoothingFactor = 0.3 + strength * 0.5; // How aggressively to smooth
        let artifactsDetected = 0;

        // Process in overlapping windows
        for (let start = 0; start < samples.length - fftSize; start += hopSize) {
            // Extract window
            const window = new Float32Array(fftSize);
            for (let i = 0; i < fftSize; i++) {
                // Hann window
                const hannValue = 0.5 * (1 - Math.cos(2 * Math.PI * i / fftSize));
                window[i] = samples[start + i] * hannValue;
            }

            // Simple spectral analysis: check for anomalous spikes
            let maxVal = 0, sumVal = 0;
            for (let i = 0; i < fftSize; i++) {
                const absVal = Math.abs(window[i]);
                maxVal = Math.max(maxVal, absVal);
                sumVal += absVal;
            }
            const avgVal = sumVal / fftSize;

            // If there's a big spike relative to average, smooth it
            if (maxVal > avgVal * (5 - strength * 3) && avgVal > 0.001) {
                artifactsDetected++;
                for (let i = 0; i < fftSize; i++) {
                    if (Math.abs(window[i]) > avgVal * (3 - strength * 1.5)) {
                        // Smooth toward local average
                        const localAvg = (
                            (i > 0 ? samples[start + i - 1] : 0) +
                            samples[start + i] +
                            (i < fftSize - 1 ? samples[start + i + 1] : 0)
                        ) / 3;
                        samples[start + i] = samples[start + i] * (1 - smoothingFactor) + localAvg * smoothingFactor;
                    }
                }
            }
        }

        return artifactsDetected;
    }

    // ─── Crossfade Error Repair ───
    // Detect zero-crossing discontinuities and apply micro-crossfades
    static _repairCrossfades(samples, sampleRate) {
        const jumpThreshold = 0.1; // Minimum amplitude jump to consider a discontinuity
        const fadeLength = Math.floor(sampleRate * 0.005); // 5ms crossfade

        for (let i = 1; i < samples.length - fadeLength; i++) {
            const jump = Math.abs(samples[i] - samples[i - 1]);
            if (jump > jumpThreshold) {
                // Check if it's a sudden phase discontinuity (not just a normal transient)
                const prevSlope = i > 1 ? Math.abs(samples[i - 1] - samples[i - 2]) : 0;
                const nextSlope = i < samples.length - 2 ? Math.abs(samples[i + 1] - samples[i]) : 0;
                const avgSlope = (prevSlope + nextSlope) / 2;

                // A true discontinuity has a jump much larger than surrounding slopes
                if (jump > avgSlope * 5 && avgSlope > 0) {
                    // Apply micro-crossfade
                    const leftVal = samples[i - 1];
                    const rightVal = samples[i];
                    const halfFade = Math.min(fadeLength / 2, Math.floor(i / 2));

                    for (let k = 0; k < halfFade * 2; k++) {
                        const idx = i - halfFade + k;
                        if (idx >= 0 && idx < samples.length) {
                            const t = k / (halfFade * 2);
                            const blend = 0.5 * (1 - Math.cos(t * Math.PI)); // Cosine crossfade
                            const fromLeft = leftVal * (1 - t) + samples[idx] * t;
                            const fromRight = rightVal * t + samples[idx] * (1 - t);
                            samples[idx] = fromLeft * (1 - blend) + fromRight * blend;
                        }
                    }
                }
            }
        }
    }

    // ─── Humanization ───
    // Add subtle micro-variations to overly-perfect/quantized audio
    static _humanize(samples, strength, sampleRate) {
        const pitchVariation = strength * 0.003; // Subtle pitch drift
        const timingJitter = Math.floor(sampleRate * 0.001 * strength); // ±1ms max
        const volumeVariation = strength * 0.02; // ±2% volume swing

        // Apply slow-moving random modulations
        const modulationRate = sampleRate / 20; // Change every 50ms
        let phase = 0;
        let currentPitchMod = 0;
        let currentVolMod = 1;

        for (let i = 0; i < samples.length; i++) {
            if (i % modulationRate === 0) {
                // New random modulation targets (smooth random walk)
                currentPitchMod += (Math.random() - 0.5) * pitchVariation;
                currentPitchMod *= 0.95; // Decay toward zero
                currentVolMod = 1 + (Math.random() - 0.5) * volumeVariation;
            }

            // Apply volume variation
            samples[i] *= currentVolMod;

            // Apply subtle sample-level jitter (simulates analog wow/flutter)
            if (timingJitter > 0 && Math.random() < 0.001 * strength) {
                const jitter = Math.floor((Math.random() - 0.5) * timingJitter);
                const src = Math.max(0, Math.min(samples.length - 1, i + jitter));
                samples[i] = samples[i] * 0.7 + samples[src] * 0.3;
            }
        }
    }

    // ─── Utility: Get cleanup preview stats without modifying ───
    static analyze(buffer) {
        const stats = { channels: buffer.numberOfChannels, duration: buffer.duration, sampleRate: buffer.sampleRate };

        for (let ch = 0; ch < buffer.numberOfChannels; ch++) {
            const data = buffer.getChannelData(ch);
            let peakAmplitude = 0, dcOffset = 0, zeroCrossings = 0;

            for (let i = 0; i < data.length; i++) {
                peakAmplitude = Math.max(peakAmplitude, Math.abs(data[i]));
                dcOffset += data[i];
                if (i > 0 && (data[i] >= 0) !== (data[i - 1] >= 0)) zeroCrossings++;
            }

            stats[`ch${ch}`] = {
                peak: peakAmplitude,
                peakDb: 20 * Math.log10(Math.max(peakAmplitude, 0.00001)),
                dcOffset: dcOffset / data.length,
                zeroCrossings,
                zeroCrossingRate: zeroCrossings / buffer.duration,
            };
        }

        return stats;
    }
}
