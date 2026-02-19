// ============================================
// ORPHEUS DAW — Transient Detection Engine
// ============================================
// Onset detection using energy-based threshold + spectral flux
// Works on Web Audio API AudioBuffer objects

export class TransientDetector {
    /**
     * Detect transients in an AudioBuffer
     * @param {AudioBuffer} buffer - Web Audio AudioBuffer
     * @param {Object} options
     * @returns {Array<number>} Array of transient positions in seconds
     */
    static detect(buffer, options = {}) {
        const {
            sensitivity = 0.5,     // 0-1, higher = more transients detected
            minInterval = 0.05,    // Minimum seconds between transients
            windowSize = 512,      // Analysis window size
            hopSize = 256,         // Hop between windows
        } = options;

        const channelData = buffer.getChannelData(0); // Use first channel
        const sampleRate = buffer.sampleRate;
        const numFrames = Math.floor((channelData.length - windowSize) / hopSize);

        if (numFrames < 2) return [];

        // Compute short-time energy for each frame
        const energy = new Float32Array(numFrames);
        for (let i = 0; i < numFrames; i++) {
            let sum = 0;
            const start = i * hopSize;
            for (let j = 0; j < windowSize; j++) {
                const sample = channelData[start + j];
                sum += sample * sample;
            }
            energy[i] = sum / windowSize;
        }

        // Compute spectral flux (difference between consecutive frames)
        const flux = new Float32Array(numFrames);
        for (let i = 1; i < numFrames; i++) {
            const diff = energy[i] - energy[i - 1];
            flux[i] = Math.max(0, diff); // Only positive flux (onset)
        }

        // Adaptive threshold: median + sensitivity * std_dev
        const sortedFlux = [...flux].filter(v => v > 0).sort((a, b) => a - b);
        if (sortedFlux.length === 0) return [];

        const median = sortedFlux[Math.floor(sortedFlux.length / 2)];
        const mean = sortedFlux.reduce((a, b) => a + b, 0) / sortedFlux.length;
        const variance = sortedFlux.reduce((a, b) => a + (b - mean) ** 2, 0) / sortedFlux.length;
        const stdDev = Math.sqrt(variance);

        // Threshold inversely proportional to sensitivity
        const thresholdMultiplier = 3 - (sensitivity * 2.5); // sensitivity 0→3x, sensitivity 1→0.5x
        const threshold = median + stdDev * thresholdMultiplier;

        // Peak picking with minimum interval constraint
        const minFrameInterval = Math.ceil(minInterval * sampleRate / hopSize);
        const transients = [];
        let lastPeak = -minFrameInterval;

        for (let i = 1; i < numFrames - 1; i++) {
            if (
                flux[i] > threshold &&
                flux[i] > flux[i - 1] &&
                flux[i] >= flux[i + 1] &&
                (i - lastPeak) >= minFrameInterval
            ) {
                const timeSec = (i * hopSize) / sampleRate;
                transients.push(timeSec);
                lastPeak = i;
            }
        }

        return transients;
    }

    /**
     * Convert transient times (seconds) to beat positions
     * @param {Array<number>} transientTimes - Transient positions in seconds
     * @param {number} bpm - Beats per minute
     * @returns {Array<number>} Transient positions in beats
     */
    static timesToBeats(transientTimes, bpm) {
        const beatsPerSecond = bpm / 60;
        return transientTimes.map(t => t * beatsPerSecond);
    }

    /**
     * Quantize transient positions to a grid
     * @param {Array<number>} beats - Transient beat positions
     * @param {number} gridSize - Grid size in beats (e.g., 0.25 = sixteenth note)
     * @returns {Array<number>} Quantized beat positions
     */
    static quantizeToGrid(beats, gridSize = 0.25) {
        return beats.map(b => Math.round(b / gridSize) * gridSize);
    }

    /**
     * Create slice markers from transients for a clip
     * @param {AudioBuffer} buffer 
     * @param {number} bpm 
     * @param {Object} options 
     * @returns {Array<number>} Beat positions relative to clip start
     */
    static getSliceMarkers(buffer, bpm, options = {}) {
        const times = TransientDetector.detect(buffer, options);
        const beats = TransientDetector.timesToBeats(times, bpm);
        return options.quantize
            ? TransientDetector.quantizeToGrid(beats, options.quantize)
            : beats;
    }
}
