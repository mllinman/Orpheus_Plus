// ============================================
// ORPHEUS DAW — MIDI LFO / Envelope Generator
// ============================================
// Generates LFO waveforms that can modulate any MIDI CC or parameter

export class MidiLFO {
    static SHAPES = {
        sine: 'sine',
        triangle: 'triangle',
        sawtooth: 'sawtooth',
        square: 'square',
        random: 'random',
        sampleHold: 'sampleHold',
    };

    /**
     * Generate LFO automation points
     * @param {Object} config
     * @returns {Array<{beat: number, value: number}>} Points with value 0-1
     */
    static generate(config = {}) {
        const {
            shape = 'sine',
            rate = 1,          // cycles per bar (4 beats)
            depth = 1,         // 0-1 modulation depth
            offset = 0.5,      // center value (0-1)
            phase = 0,         // phase offset in degrees
            bars = 4,          // duration in bars
            resolution = 16,   // points per bar
        } = config;

        const totalPoints = bars * resolution;
        const points = [];
        const phaseRad = (phase / 360) * Math.PI * 2;
        let lastRandom = Math.random();

        for (let i = 0; i < totalPoints; i++) {
            const beat = (i / resolution) * 4; // convert to beats
            const t = (i / resolution) * rate; // cycle position
            const angle = t * Math.PI * 2 + phaseRad;

            let raw;
            switch (shape) {
                case 'sine':
                    raw = Math.sin(angle);
                    break;
                case 'triangle':
                    raw = (2 / Math.PI) * Math.asin(Math.sin(angle));
                    break;
                case 'sawtooth':
                    raw = 2 * ((t + phase / 360) % 1) - 1;
                    break;
                case 'square':
                    raw = Math.sin(angle) >= 0 ? 1 : -1;
                    break;
                case 'random':
                    raw = Math.random() * 2 - 1;
                    break;
                case 'sampleHold':
                    if (i % resolution === 0) lastRandom = Math.random() * 2 - 1;
                    raw = lastRandom;
                    break;
                default:
                    raw = Math.sin(angle);
            }

            const value = Math.max(0, Math.min(1, offset + raw * depth * 0.5));
            points.push({ beat, value });
        }

        return points;
    }

    /**
     * Apply LFO to a parameter range
     * @param {Array<{beat, value}>} lfoPoints 
     * @param {number} min - Minimum parameter value
     * @param {number} max - Maximum parameter value
     * @returns {Array<{beat, value}>} Scaled points
     */
    static applyToRange(lfoPoints, min, max) {
        return lfoPoints.map(p => ({
            beat: p.beat,
            value: min + p.value * (max - min),
        }));
    }

    /**
     * Generate envelope (ADSR)
     * @param {Object} config 
     * @returns {Array<{beat, value}>}
     */
    static generateEnvelope(config = {}) {
        const {
            attack = 0.1,   // beats
            decay = 0.2,    // beats
            sustain = 0.7,  // level 0-1
            release = 0.5,  // beats
            hold = 2,       // beats at sustain level
        } = config;

        const points = [];
        points.push({ beat: 0, value: 0 });
        points.push({ beat: attack, value: 1 });
        points.push({ beat: attack + decay, value: sustain });
        points.push({ beat: attack + decay + hold, value: sustain });
        points.push({ beat: attack + decay + hold + release, value: 0 });
        return points;
    }
}
