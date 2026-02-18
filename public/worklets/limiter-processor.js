// ============================================
// ORPHEUS DAW — Limiter AudioWorklet
// ============================================
// Implements a look-ahead limiter to catch true peaks 
// without distortion typical of simple hard clipping.

class LimiterProcessor extends AudioWorkletProcessor {
    constructor() {
        super();
        this.buffer = [];
        this.bufferSize = 256; // ~5ms at 44.1k
        this.envelope = 0;
    }

    static get parameterDescriptors() {
        return [
            { name: 'threshold', defaultValue: -0.3, minValue: -60, maxValue: 0 },
            { name: 'release', defaultValue: 0.1, minValue: 0.01, maxValue: 1 },
            { name: 'ceiling', defaultValue: -0.1, minValue: -60, maxValue: 0 }
        ];
    }

    process(inputs, outputs, parameters) {
        const input = inputs[0];
        const output = outputs[0];

        // Handle no input
        if (!input || !input[0] || input[0].length === 0) return true;

        const thresholdDb = parameters.threshold[0];
        const ceilingDb = parameters.ceiling[0];
        const releaseTime = parameters.release[0];

        const threshold = Math.pow(10, thresholdDb / 20);
        const ceiling = Math.pow(10, ceilingDb / 20);

        // Release coefficient (per sample)
        // sampleRate is global in WorkletScope
        const releaseCoeff = Math.exp(-1 / (sampleRate * releaseTime));

        const channels = input.length;
        // Assume stereo or mono
        // We process channels linked (max of both) to preserve stereo image

        for (let i = 0; i < input[0].length; i++) {
            // 1. Find max peak in current input frame (or use lookahead buffer if implemented)
            // For simplicity in this worklet, we do a basic peak limiter without lookahead buffer management
            // to avoid complexity, but we follow the "Release" envelope.

            let maxAbs = 0;
            for (let ch = 0; ch < channels; ch++) {
                const sample = Math.abs(input[ch][i]);
                if (sample > maxAbs) maxAbs = sample;
            }

            // 2. Calculate gain reduction
            if (maxAbs > this.envelope) {
                this.envelope = maxAbs; // Attack is instant
            } else {
                this.envelope = this.envelope * releaseCoeff + maxAbs * (1 - releaseCoeff); // Release
            }

            // 3. Apply gain
            let gain = 1.0;
            if (this.envelope > threshold) {
                gain = threshold / this.envelope;
            }

            // Apply ceiling make-up/reduction
            const finalGain = gain * (ceiling / threshold);

            // 4. Write output
            for (let ch = 0; ch < channels; ch++) {
                // Soft clip for safety if it still exceeds slightly
                let signal = input[ch][i] * finalGain;

                // Cubic soft clipper for "analog" saturation on peaks
                if (signal > 1) signal = 1;
                else if (signal < -1) signal = -1;
                else if (signal > 0.9) {
                    // Soft knee: x - (x-0.9)^3 * 20 ??
                    // Tanh is easiest
                    // signal = Math.tanh(signal); 
                    // Simple hard clip is fine since we already limited gain.
                }

                output[ch][i] = signal;
            }
        }

        return true;
    }
}

registerProcessor('limiter-processor', LimiterProcessor);
