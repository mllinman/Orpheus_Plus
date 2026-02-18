// ============================================
// ORPHEUS DAW — Sidechain Compressor
// ============================================
// Web Audio API based sidechain compression with key input routing

export class SidechainCompressor {
    constructor(audioContext) {
        this.ctx = audioContext;

        // Main signal chain
        this.input = this.ctx.createGain();
        this.output = this.ctx.createGain();

        // Sidechain input
        this.sidechainInput = this.ctx.createGain();

        // Analyzer for sidechain signal level
        this.analyzer = this.ctx.createAnalyser();
        this.analyzer.fftSize = 256;
        this.sidechainInput.connect(this.analyzer);

        // Compressor (native)
        this.compressor = this.ctx.createDynamicsCompressor();
        this.input.connect(this.compressor);
        this.compressor.connect(this.output);

        // Settings
        this._threshold = -20;
        this._ratio = 4;
        this._attack = 0.003;
        this._release = 0.25;
        this._depth = 1;

        this.applySettings();

        // Sidechain processing loop
        this._active = true;
        this._dataArray = new Uint8Array(this.analyzer.frequencyBinCount);
        this._process();
    }

    applySettings() {
        this.compressor.threshold.value = this._threshold;
        this.compressor.ratio.value = this._ratio;
        this.compressor.attack.value = this._attack;
        this.compressor.release.value = this._release;
        this.compressor.knee.value = 6;
    }

    set threshold(v) { this._threshold = v; this.applySettings(); }
    set ratio(v) { this._ratio = v; this.applySettings(); }
    set attack(v) { this._attack = v / 1000; this.applySettings(); } // ms to seconds
    set release(v) { this._release = v / 1000; this.applySettings(); } // ms to seconds
    set depth(v) { this._depth = Math.max(0, Math.min(1, v)); }

    get threshold() { return this._threshold; }
    get ratio() { return this._ratio; }
    get attack() { return this._attack * 1000; }
    get release() { return this._release * 1000; }
    get depth() { return this._depth; }

    _process() {
        if (!this._active) return;

        this.analyzer.getByteTimeDomainData(this._dataArray);

        // Calculate RMS of sidechain signal
        let sum = 0;
        for (let i = 0; i < this._dataArray.length; i++) {
            const v = (this._dataArray[i] - 128) / 128;
            sum += v * v;
        }
        const rms = Math.sqrt(sum / this._dataArray.length);

        // Convert to dB
        const db = 20 * Math.log10(Math.max(rms, 0.0001));

        // Apply gain reduction based on sidechain level
        if (db > this._threshold) {
            const excessDb = db - this._threshold;
            const gainReduction = excessDb * (1 - 1 / this._ratio);
            const targetGain = Math.pow(10, -gainReduction * this._depth / 20);

            // Smooth gain changes
            const now = this.ctx.currentTime;
            this.output.gain.setTargetAtTime(targetGain, now, this._attack);
        } else {
            const now = this.ctx.currentTime;
            this.output.gain.setTargetAtTime(1, now, this._release);
        }

        requestAnimationFrame(() => this._process());
    }

    /**
     * Get the current gain reduction in dB (for metering)
     */
    getGainReduction() {
        return 20 * Math.log10(Math.max(this.output.gain.value, 0.0001));
    }

    destroy() {
        this._active = false;
        this.input.disconnect();
        this.output.disconnect();
        this.sidechainInput.disconnect();
        this.compressor.disconnect();
    }
}
