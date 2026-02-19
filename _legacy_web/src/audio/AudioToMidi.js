// ============================================
// ORPHEUS DAW — Audio-to-MIDI Converter
// ============================================
// Converts audio buffers to MIDI note data via pitch detection,
// onset detection, and optional stem separation

export class AudioToMidi {

    /**
     * Convert an audio buffer to MIDI note events
     * @param {AudioContext} ctx
     * @param {AudioBuffer} buffer
     * @param {Object} options
     * @returns {{ tracks: Array<{ name, notes }>, vocalBuffer?: AudioBuffer }}
     */
    static async convert(ctx, buffer, options = {}) {
        const {
            mode = 'monophonic', // 'monophonic' | 'polyphonic' | 'stems'
            sensitivity = 0.5,
            quantizeGrid = 1 / 16, // Beat subdivision for quantization
            bpm = 120,
            minVelocity = 10,
            minNoteDuration = 0.05, // Seconds — ignore shorter
        } = options;

        if (mode === 'stems') {
            return AudioToMidi._convertWithStems(ctx, buffer, options);
        }

        const monoData = AudioToMidi._mixToMono(buffer);
        const sampleRate = buffer.sampleRate;

        // 1. Detect onsets
        const onsets = AudioToMidi._detectOnsets(monoData, sampleRate, sensitivity);

        // 2. Detect pitches between onsets
        const notes = [];
        for (let i = 0; i < onsets.length; i++) {
            const startSample = onsets[i];
            const endSample = i + 1 < onsets.length ? onsets[i + 1] : monoData.length;
            const segment = monoData.slice(startSample, endSample);

            const durationSec = segment.length / sampleRate;
            if (durationSec < minNoteDuration) continue;

            let pitch;
            if (mode === 'polyphonic') {
                // FFT peak picking for chord content
                const pitches = AudioToMidi._detectPitchesPolyphonic(segment, sampleRate);
                pitches.forEach(p => {
                    const midiNote = AudioToMidi._freqToMidi(p.frequency);
                    if (midiNote >= 0 && midiNote <= 127) {
                        const velocity = Math.round(Math.min(127, Math.max(minVelocity, p.amplitude * 127)));
                        const startBeat = (startSample / sampleRate) * (bpm / 60);
                        const lengthBeats = durationSec * (bpm / 60);

                        notes.push({
                            pitch: midiNote,
                            startBeat: AudioToMidi._quantize(startBeat, quantizeGrid),
                            lengthBeats: Math.max(quantizeGrid, AudioToMidi._quantize(lengthBeats, quantizeGrid)),
                            velocity,
                        });
                    }
                });
            } else {
                // Monophonic autocorrelation
                pitch = AudioToMidi._detectPitchAutocorrelation(segment, sampleRate);
                if (pitch > 0) {
                    const midiNote = AudioToMidi._freqToMidi(pitch);
                    if (midiNote >= 0 && midiNote <= 127) {
                        const velocity = Math.round(Math.min(127, Math.max(minVelocity,
                            AudioToMidi._rms(segment) * 2 * 127
                        )));
                        const startBeat = (startSample / sampleRate) * (bpm / 60);
                        const lengthBeats = durationSec * (bpm / 60);

                        notes.push({
                            pitch: midiNote,
                            startBeat: AudioToMidi._quantize(startBeat, quantizeGrid),
                            lengthBeats: Math.max(quantizeGrid, AudioToMidi._quantize(lengthBeats, quantizeGrid)),
                            velocity,
                        });
                    }
                }
            }
        }

        // Merge duplicate notes at same position
        const merged = AudioToMidi._mergeNotes(notes);

        return {
            tracks: [{ name: 'Audio to MIDI', notes: merged }],
        };
    }

    /**
     * Full stem separation → MIDI pipeline
     * Separates audio into stems, converts each to MIDI, isolates vocals
     */
    static async _convertWithStems(ctx, buffer, options) {
        // Simulate stem separation (in production, this calls StemSeparator)
        // We split by frequency bands as a practical approximation
        const sampleRate = buffer.sampleRate;
        const monoData = AudioToMidi._mixToMono(buffer);

        const stems = {
            bass: AudioToMidi._bandpassFilter(monoData, sampleRate, 20, 250),
            mid: AudioToMidi._bandpassFilter(monoData, sampleRate, 250, 4000),
            high: AudioToMidi._bandpassFilter(monoData, sampleRate, 4000, 16000),
        };

        const bpm = options.bpm || 120;
        const quantizeGrid = options.quantizeGrid || 1 / 16;
        const sensitivity = options.sensitivity || 0.5;
        const minVelocity = options.minVelocity || 10;

        const tracks = [];

        // Convert each stem to MIDI
        for (const [stemName, stemData] of Object.entries(stems)) {
            const onsets = AudioToMidi._detectOnsets(stemData, sampleRate, sensitivity);
            const notes = [];

            for (let i = 0; i < onsets.length; i++) {
                const startSample = onsets[i];
                const endSample = i + 1 < onsets.length ? onsets[i + 1] : stemData.length;
                const segment = stemData.slice(startSample, endSample);
                const durationSec = segment.length / sampleRate;

                if (durationSec < 0.05) continue;

                const pitch = AudioToMidi._detectPitchAutocorrelation(segment, sampleRate);
                if (pitch > 0) {
                    const midiNote = AudioToMidi._freqToMidi(pitch);
                    if (midiNote >= 0 && midiNote <= 127) {
                        const velocity = Math.round(Math.min(127, Math.max(minVelocity,
                            AudioToMidi._rms(segment) * 2 * 127
                        )));
                        notes.push({
                            pitch: midiNote,
                            startBeat: AudioToMidi._quantize((startSample / sampleRate) * (bpm / 60), quantizeGrid),
                            lengthBeats: Math.max(quantizeGrid, AudioToMidi._quantize(durationSec * (bpm / 60), quantizeGrid)),
                            velocity,
                        });
                    }
                }
            }

            const instrumentName = stemName === 'bass' ? 'Bass'
                : stemName === 'mid' ? 'Keys/Lead'
                    : 'Percussion/High';

            tracks.push({ name: instrumentName, notes: AudioToMidi._mergeNotes(notes) });
        }

        // Isolate vocal range (800Hz - 5000Hz) as a clean audio buffer
        const vocalData = AudioToMidi._bandpassFilter(monoData, sampleRate, 800, 5000);
        const vocalBuffer = ctx.createBuffer(1, vocalData.length, sampleRate);
        vocalBuffer.getChannelData(0).set(vocalData);

        return { tracks, vocalBuffer };
    }

    // ─── Onset Detection ───
    // Energy-based onset detection with adaptive threshold
    static _detectOnsets(samples, sampleRate, sensitivity) {
        const hopSize = Math.floor(sampleRate * 0.01); // 10ms hop
        const windowSize = hopSize * 2;
        const onsets = [0]; // Always start with onset at 0
        const threshold = 0.05 * (2 - sensitivity); // Lower sensitivity = higher threshold

        let prevEnergy = 0;
        let adaptiveThreshold = threshold;

        for (let i = 0; i < samples.length - windowSize; i += hopSize) {
            // Calculate window energy
            let energy = 0;
            for (let j = 0; j < windowSize; j++) {
                energy += samples[i + j] * samples[i + j];
            }
            energy /= windowSize;

            // Spectral flux: positive energy increase
            const flux = Math.max(0, energy - prevEnergy);

            // Adaptive threshold (exponential moving average)
            adaptiveThreshold = adaptiveThreshold * 0.95 + flux * 0.05;

            if (flux > adaptiveThreshold * (3 - sensitivity * 2) && flux > threshold) {
                // Minimum distance between onsets: 50ms
                const lastOnset = onsets[onsets.length - 1];
                if ((i - lastOnset) > sampleRate * 0.05) {
                    onsets.push(i);
                }
            }

            prevEnergy = energy;
        }

        return onsets;
    }

    // ─── Monophonic Pitch Detection (Autocorrelation) ───
    static _detectPitchAutocorrelation(segment, sampleRate) {
        const minFreq = 50; // Lowest pitch to detect
        const maxFreq = 4000; // Highest pitch
        const minLag = Math.floor(sampleRate / maxFreq);
        const maxLag = Math.floor(sampleRate / minFreq);
        const len = Math.min(segment.length, maxLag * 2);

        if (len < maxLag) return 0;

        // Normalized autocorrelation
        let bestCorr = 0;
        let bestLag = 0;

        // First pass: coarse search
        for (let lag = minLag; lag < Math.min(maxLag, len / 2); lag++) {
            let correlation = 0;
            let norm1 = 0, norm2 = 0;

            for (let i = 0; i < len - lag; i++) {
                correlation += segment[i] * segment[i + lag];
                norm1 += segment[i] * segment[i];
                norm2 += segment[i + lag] * segment[i + lag];
            }

            const norm = Math.sqrt(norm1 * norm2);
            if (norm > 0) {
                correlation /= norm;
                if (correlation > bestCorr) {
                    bestCorr = correlation;
                    bestLag = lag;
                }
            }
        }

        // Need minimum correlation confidence
        if (bestCorr < 0.3 || bestLag === 0) return 0;

        // Parabolic interpolation for sub-sample accuracy
        if (bestLag > minLag && bestLag < maxLag - 1) {
            const prev = AudioToMidi._autocorrAtLag(segment, bestLag - 1, len);
            const curr = bestCorr;
            const next = AudioToMidi._autocorrAtLag(segment, bestLag + 1, len);
            const delta = 0.5 * (prev - next) / (prev - 2 * curr + next + 0.00001);
            return sampleRate / (bestLag + delta);
        }

        return sampleRate / bestLag;
    }

    static _autocorrAtLag(segment, lag, len) {
        let corr = 0, n1 = 0, n2 = 0;
        for (let i = 0; i < len - lag; i++) {
            corr += segment[i] * segment[i + lag];
            n1 += segment[i] * segment[i];
            n2 += segment[i + lag] * segment[i + lag];
        }
        const norm = Math.sqrt(n1 * n2);
        return norm > 0 ? corr / norm : 0;
    }

    // ─── Polyphonic Pitch Detection (FFT Peak Picking) ───
    static _detectPitchesPolyphonic(segment, sampleRate) {
        const fftSize = 4096;
        const magnitudes = new Float32Array(fftSize / 2);

        // Simple DFT (for production, use Web Audio AnalyserNode)
        for (let k = 0; k < fftSize / 2; k++) {
            let real = 0, imag = 0;
            const freq = k * sampleRate / fftSize;
            if (freq < 50 || freq > 4000) continue;

            for (let n = 0; n < Math.min(segment.length, fftSize); n++) {
                const angle = 2 * Math.PI * k * n / fftSize;
                real += segment[n] * Math.cos(angle);
                imag -= segment[n] * Math.sin(angle);
            }
            magnitudes[k] = Math.sqrt(real * real + imag * imag);
        }

        // Find peaks
        const pitches = [];
        const threshold = Math.max(...magnitudes) * 0.2;

        for (let k = 2; k < magnitudes.length - 2; k++) {
            if (magnitudes[k] > threshold &&
                magnitudes[k] > magnitudes[k - 1] &&
                magnitudes[k] > magnitudes[k + 1] &&
                magnitudes[k] > magnitudes[k - 2] &&
                magnitudes[k] > magnitudes[k + 2]) {
                const freq = k * sampleRate / fftSize;
                if (freq >= 50 && freq <= 4000) {
                    pitches.push({ frequency: freq, amplitude: magnitudes[k] / fftSize });
                }
            }
        }

        // Take top 6 peaks (max notes in a chord)
        pitches.sort((a, b) => b.amplitude - a.amplitude);
        return pitches.slice(0, 6);
    }

    // ─── Bandpass Filter (time-domain approximation) ───
    static _bandpassFilter(samples, sampleRate, lowFreq, highFreq) {
        const output = new Float32Array(samples.length);
        const lowCutoff = lowFreq / sampleRate;
        const highCutoff = highFreq / sampleRate;

        // Simple resonant bandpass via biquad coefficients
        const w0Low = 2 * Math.PI * lowCutoff;
        const w0High = 2 * Math.PI * highCutoff;
        const Q = 0.707;

        // High-pass then low-pass
        let hp_x1 = 0, hp_x2 = 0, hp_y1 = 0, hp_y2 = 0;
        let lp_x1 = 0, lp_x2 = 0, lp_y1 = 0, lp_y2 = 0;

        const alphaHP = Math.sin(w0Low) / (2 * Q);
        const hp_a0 = 1 + alphaHP;
        const hp_b0 = ((1 + Math.cos(w0Low)) / 2) / hp_a0;
        const hp_b1 = (-(1 + Math.cos(w0Low))) / hp_a0;
        const hp_b2 = hp_b0;
        const hp_a1 = (-2 * Math.cos(w0Low)) / hp_a0;
        const hp_a2 = (1 - alphaHP) / hp_a0;

        const alphaLP = Math.sin(w0High) / (2 * Q);
        const lp_a0 = 1 + alphaLP;
        const lp_b0 = ((1 - Math.cos(w0High)) / 2) / lp_a0;
        const lp_b1 = (1 - Math.cos(w0High)) / lp_a0;
        const lp_b2 = lp_b0;
        const lp_a1 = (-2 * Math.cos(w0High)) / lp_a0;
        const lp_a2 = (1 - alphaLP) / lp_a0;

        // Apply highpass
        const hpOut = new Float32Array(samples.length);
        for (let i = 0; i < samples.length; i++) {
            const x = samples[i];
            hpOut[i] = hp_b0 * x + hp_b1 * hp_x1 + hp_b2 * hp_x2 - hp_a1 * hp_y1 - hp_a2 * hp_y2;
            hp_x2 = hp_x1; hp_x1 = x;
            hp_y2 = hp_y1; hp_y1 = hpOut[i];
        }

        // Apply lowpass
        for (let i = 0; i < hpOut.length; i++) {
            const x = hpOut[i];
            output[i] = lp_b0 * x + lp_b1 * lp_x1 + lp_b2 * lp_x2 - lp_a1 * lp_y1 - lp_a2 * lp_y2;
            lp_x2 = lp_x1; lp_x1 = x;
            lp_y2 = lp_y1; lp_y1 = output[i];
        }

        return output;
    }

    // ─── Helpers ───

    static _mixToMono(buffer) {
        const length = buffer.length;
        const mono = new Float32Array(length);
        const channels = buffer.numberOfChannels;
        for (let ch = 0; ch < channels; ch++) {
            const data = buffer.getChannelData(ch);
            for (let i = 0; i < length; i++) {
                mono[i] += data[i] / channels;
            }
        }
        return mono;
    }

    static _freqToMidi(freq) {
        return Math.round(69 + 12 * Math.log2(freq / 440));
    }

    static _rms(samples) {
        let sum = 0;
        for (let i = 0; i < samples.length; i++) sum += samples[i] * samples[i];
        return Math.sqrt(sum / samples.length);
    }

    static _quantize(value, grid) {
        return Math.round(value / grid) * grid;
    }

    static _mergeNotes(notes) {
        // Remove exact duplicates at same position/pitch
        const seen = new Set();
        return notes.filter(n => {
            const key = `${n.pitch}-${n.startBeat.toFixed(4)}`;
            if (seen.has(key)) return false;
            seen.add(key);
            return true;
        });
    }
}
