// ============================================
// ORPHEUS DAW — Track Extender
// ============================================
// Intelligent track/song extension via pattern detection,
// loop duplication with crossfade blending, and smart fill

export class TrackExtender {

    /**
     * Extend a clip to a target length using pattern detection
     * @param {AudioContext} ctx
     * @param {AudioBuffer} buffer - Source audio to extend
     * @param {Object} options
     * @returns {AudioBuffer} Extended buffer
     */
    static async extend(ctx, buffer, options = {}) {
        const {
            targetLengthSeconds = null, // Extend to this duration
            targetBars = null,          // Or extend by this many bars
            bpm = 120,
            beatsPerBar = 4,
            crossfadeLength = 0.05,     // Crossfade in seconds
            variation = 0.1,            // Randomization amount (0-1)
        } = options;

        const sampleRate = buffer.sampleRate;
        const sourceDuration = buffer.duration;

        // Determine target length
        let targetDuration;
        if (targetLengthSeconds) {
            targetDuration = targetLengthSeconds;
        } else if (targetBars) {
            const barDuration = (beatsPerBar * 60) / bpm;
            targetDuration = sourceDuration + targetBars * barDuration;
        } else {
            // Default: double the length
            targetDuration = sourceDuration * 2;
        }

        if (targetDuration <= sourceDuration) {
            // Just trim — return a sliced copy
            return TrackExtender._trimBuffer(ctx, buffer, targetDuration);
        }

        // Detect best loop point in the source
        const monoData = TrackExtender._mixToMono(buffer);
        const loopInfo = TrackExtender._detectLoopPoint(monoData, sampleRate, bpm, beatsPerBar);

        // Create extended buffer
        const targetSamples = Math.ceil(targetDuration * sampleRate);
        const extended = ctx.createBuffer(buffer.numberOfChannels, targetSamples, sampleRate);

        for (let ch = 0; ch < buffer.numberOfChannels; ch++) {
            const src = buffer.getChannelData(ch);
            const dst = extended.getChannelData(ch);

            // Copy original first
            const copyLen = Math.min(src.length, dst.length);
            for (let i = 0; i < copyLen; i++) {
                dst[i] = src[i];
            }

            // Extend by looping from detected loop point
            if (copyLen < targetSamples) {
                const loopStart = loopInfo.startSample;
                const loopEnd = loopInfo.endSample;
                const loopLength = loopEnd - loopStart;

                if (loopLength > 0) {
                    let writePos = copyLen;
                    let iteration = 0;

                    while (writePos < targetSamples) {
                        iteration++;
                        for (let i = 0; i < loopLength && writePos < targetSamples; i++, writePos++) {
                            let sample = src[loopStart + i];

                            // Apply slight variation each iteration
                            if (variation > 0) {
                                const variationAmount = variation * 0.02 * Math.sin(iteration * 0.7 + i * 0.001);
                                sample *= (1 + variationAmount);
                            }

                            // Crossfade at loop boundaries
                            const crossfadeSamples = Math.floor(crossfadeLength * sampleRate);
                            if (i < crossfadeSamples) {
                                // Fade in from previous content
                                const t = i / crossfadeSamples;
                                const fadeIn = 0.5 * (1 - Math.cos(t * Math.PI));
                                sample = dst[writePos - 1] * (1 - fadeIn) + sample * fadeIn;
                            }

                            dst[writePos] = sample;
                        }
                    }
                }
            }
        }

        return extended;
    }

    /**
     * Detect the best loop point in audio via autocorrelation of energy envelope
     */
    static _detectLoopPoint(monoData, sampleRate, bpm, beatsPerBar) {
        const barDuration = (beatsPerBar * 60) / bpm;
        const barSamples = Math.floor(barDuration * sampleRate);
        const totalBars = Math.floor(monoData.length / barSamples);

        if (totalBars < 2) {
            // Audio too short — loop entire thing
            return { startSample: 0, endSample: monoData.length, confidence: 0.5 };
        }

        // Calculate energy profile per bar
        const barEnergies = [];
        for (let bar = 0; bar < totalBars; bar++) {
            let energy = 0;
            const offset = bar * barSamples;
            for (let i = 0; i < barSamples && offset + i < monoData.length; i++) {
                energy += monoData[offset + i] * monoData[offset + i];
            }
            barEnergies.push(energy / barSamples);
        }

        // Find repeating pattern via autocorrelation of bar energies
        let bestPeriod = 1;
        let bestCorrelation = 0;

        for (let period = 1; period <= Math.floor(totalBars / 2); period++) {
            let correlation = 0;
            let count = 0;

            for (let i = 0; i < totalBars - period; i++) {
                const diff = Math.abs(barEnergies[i] - barEnergies[i + period]);
                correlation += 1 / (1 + diff * 1000);
                count++;
            }

            if (count > 0) {
                correlation /= count;
                if (correlation > bestCorrelation) {
                    bestCorrelation = correlation;
                    bestPeriod = period;
                }
            }
        }

        // Find the best matching bar pair for the loop start
        let bestMatchBar = Math.max(0, totalBars - bestPeriod);
        let bestMatchScore = 0;

        for (let bar = 0; bar < totalBars - bestPeriod; bar++) {
            // Cross-correlate the actual audio at bar boundaries
            const offset1 = bar * barSamples;
            const offset2 = (bar + bestPeriod) * barSamples;
            let score = 0;
            const checkLen = Math.min(barSamples, 4096);

            for (let i = 0; i < checkLen; i++) {
                if (offset1 + i < monoData.length && offset2 + i < monoData.length) {
                    score += monoData[offset1 + i] * monoData[offset2 + i];
                }
            }

            if (score > bestMatchScore) {
                bestMatchScore = score;
                bestMatchBar = bar;
            }
        }

        return {
            startSample: bestMatchBar * barSamples,
            endSample: (bestMatchBar + bestPeriod) * barSamples,
            periodBars: bestPeriod,
            confidence: bestCorrelation,
        };
    }

    /**
     * Trim a buffer to a target duration
     */
    static _trimBuffer(ctx, buffer, targetSeconds) {
        const targetSamples = Math.min(
            Math.ceil(targetSeconds * buffer.sampleRate),
            buffer.length
        );
        const trimmed = ctx.createBuffer(
            buffer.numberOfChannels,
            targetSamples,
            buffer.sampleRate
        );

        for (let ch = 0; ch < buffer.numberOfChannels; ch++) {
            const src = buffer.getChannelData(ch);
            const dst = trimmed.getChannelData(ch);
            for (let i = 0; i < targetSamples; i++) {
                dst[i] = src[i];
            }
        }

        // Apply fade-out at the end (50ms)
        const fadeLen = Math.min(Math.floor(0.05 * buffer.sampleRate), targetSamples);
        for (let ch = 0; ch < trimmed.numberOfChannels; ch++) {
            const data = trimmed.getChannelData(ch);
            for (let i = 0; i < fadeLen; i++) {
                const t = i / fadeLen;
                data[targetSamples - fadeLen + i] *= 0.5 * (1 + Math.cos(t * Math.PI));
            }
        }

        return trimmed;
    }

    /**
     * Time-stretch a buffer (change duration without changing pitch)
     * Uses simple overlap-add (PSOLA-light)
     */
    static timeStretch(ctx, buffer, factor) {
        if (Math.abs(factor - 1) < 0.01) return buffer; // No change needed

        const sampleRate = buffer.sampleRate;
        const newLength = Math.ceil(buffer.length * factor);
        const stretched = ctx.createBuffer(buffer.numberOfChannels, newLength, sampleRate);

        const windowSize = 2048;
        const hopIn = Math.floor(windowSize / 4);
        const hopOut = Math.floor(hopIn * factor);

        for (let ch = 0; ch < buffer.numberOfChannels; ch++) {
            const src = buffer.getChannelData(ch);
            const dst = stretched.getChannelData(ch);

            // Overlap-add
            for (let outPos = 0, inPos = 0;
                outPos < newLength - windowSize && inPos < src.length - windowSize;
                outPos += hopOut, inPos += hopIn) {

                for (let i = 0; i < windowSize; i++) {
                    // Hann window
                    const w = 0.5 * (1 - Math.cos(2 * Math.PI * i / windowSize));
                    if (outPos + i < newLength) {
                        dst[outPos + i] += src[inPos + i] * w;
                    }
                }
            }

            // Normalize to prevent clipping
            let maxVal = 0;
            for (let i = 0; i < newLength; i++) maxVal = Math.max(maxVal, Math.abs(dst[i]));
            if (maxVal > 1) {
                for (let i = 0; i < newLength; i++) dst[i] /= maxVal;
            }
        }

        return stretched;
    }

    static _mixToMono(buffer) {
        const length = buffer.length;
        const mono = new Float32Array(length);
        for (let ch = 0; ch < buffer.numberOfChannels; ch++) {
            const data = buffer.getChannelData(ch);
            for (let i = 0; i < length; i++) mono[i] += data[i] / buffer.numberOfChannels;
        }
        return mono;
    }
}
