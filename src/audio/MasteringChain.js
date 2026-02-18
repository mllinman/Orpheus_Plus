// ============================================
// ORPHEUS DAW — Mastering Chain
// ============================================
// Professional mastering signal chain:
// Saturation → EQ → Multiband Comp → M/S Widener → Limiter (True Peak)

export class MasteringChain {
    constructor(audioContext, destination) {
        this.ctx = audioContext;
        this.destination = destination;
        this.enabled = false;
        this.workletReady = false;

        // ── Create Nodes ──
        this.input = this.ctx.createGain();
        this.output = this.ctx.createGain();

        // 1. Input Trim
        this.inputTrim = this.ctx.createGain();

        // 2. Saturator (Analog Warmth)
        this.saturator = this._createSaturator();

        // 3. Pre-EQ
        this.preEQ = this._createPreEQ();

        // 4. Multiband Compressor
        this.multibandComp = this._createMultibandCompressor();

        // 5. M/S Stereo Widener
        this.stereoWidener = this._createStereoWidener();

        // 6. Post-EQ
        this.postEQ = this._createPostEQ();

        // 7. Limiter (Worklet-based with fallback)
        this.limiter = this._createLimiterPlaceholder();

        // 8. Output Gain
        this.outputGain = this.ctx.createGain();

        // Metering
        this.lufsAnalyser = this.ctx.createAnalyser();
        this.lufsAnalyser.fftSize = 4096;

        this._buildChain();
        this._loadWorklet(); // Async load true peak limiter

        this.currentPreset = 'Streaming';
        this.applyPreset('Streaming');
    }

    async _loadWorklet() {
        try {
            await this.ctx.audioWorklet.addModule('/worklets/limiter-processor.js');

            // Create Worklet Node
            const workletNode = new AudioWorkletNode(this.ctx, 'limiter-processor');

            // Replace placeholder compressor with worklet
            // Disconnect fallback compressor
            this.limiter.input.disconnect(this.limiter.comp);
            this.limiter.comp.disconnect(this.limiter.output);

            // Connect worklet
            this.limiter.input.connect(workletNode);
            workletNode.connect(this.limiter.output);

            // Store reference
            this.limiter.worklet = workletNode;
            this.workletReady = true;
            console.log('[MasteringChain] True Peak Limiter loaded');

            // Apply current settings
            this.limiter.setThreshold(this._pendingThreshold || -0.3);
            this.limiter.setCeiling(this._pendingCeiling || -0.1);

        } catch (e) {
            console.warn('[MasteringChain] Failed to load limiter worklet, using fallback compressor', e);
        }
    }

    _createSaturator() {
        // Soft clipping wave shaper
        const shaper = this.ctx.createWaveShaper();
        // Create curve (tanh-like)
        const samples = 44100;
        const curve = new Float32Array(samples);
        for (let i = 0; i < samples; ++i) {
            const x = (i * 2) / samples - 1;
            // Adjustable hardness could be added, but fixed soft clip is good
            curve[i] = Math.tanh(x * 1.5) / Math.tanh(1.5);
        }
        shaper.curve = curve;
        shaper.oversample = '4x';

        const drive = this.ctx.createGain();
        drive.gain.value = 1;
        const makeup = this.ctx.createGain();
        makeup.gain.value = 1;

        drive.connect(shaper);
        shaper.connect(makeup);

        return { input: drive, output: makeup, drive };
    }

    _createPreEQ() {
        const lowCut = this.ctx.createBiquadFilter();
        lowCut.type = 'highpass';
        lowCut.frequency.value = 30;

        const lowShelf = this.ctx.createBiquadFilter();
        lowShelf.type = 'lowshelf';
        lowShelf.frequency.value = 80;

        const midPeak = this.ctx.createBiquadFilter();
        midPeak.type = 'peaking';
        midPeak.frequency.value = 1000;

        const highShelf = this.ctx.createBiquadFilter();
        highShelf.type = 'highshelf';
        highShelf.frequency.value = 10000;

        lowCut.connect(lowShelf);
        lowShelf.connect(midPeak);
        midPeak.connect(highShelf);

        return {
            input: lowCut,
            output: highShelf,
            lowCut,
            lowShelf,
            midPeak,
            highShelf
        };
    }

    _createMultibandCompressor() {
        const inGain = this.ctx.createGain();
        const outGain = this.ctx.createGain();

        // 3 Bands split
        // Low Band (< 200)
        const lowLP = this.ctx.createBiquadFilter();
        lowLP.type = 'lowpass';
        lowLP.frequency.value = 200;
        const lowComp = this.ctx.createDynamicsCompressor();

        // Mid Band (200 - 4k)
        // Highpass at 200, Lowpass at 4000
        const midHP = this.ctx.createBiquadFilter();
        midHP.type = 'highpass';
        midHP.frequency.value = 200;
        const midLP = this.ctx.createBiquadFilter();
        midLP.type = 'lowpass';
        midLP.frequency.value = 4000;
        const midComp = this.ctx.createDynamicsCompressor();

        // High Band (> 4k)
        const highHP = this.ctx.createBiquadFilter();
        highHP.type = 'highpass';
        highHP.frequency.value = 4000;
        const highComp = this.ctx.createDynamicsCompressor();

        // Routing
        inGain.connect(lowLP);
        lowLP.connect(lowComp);
        lowComp.connect(outGain);

        inGain.connect(midHP);
        midHP.connect(midLP);
        midLP.connect(midComp);
        midComp.connect(outGain);

        inGain.connect(highHP);
        highHP.connect(highComp);
        highComp.connect(outGain);

        return {
            input: inGain,
            output: outGain,
            bands: {
                low: { comp: lowComp },
                mid: { comp: midComp },
                high: { comp: highComp }
            }
        };
    }

    _createStereoWidener() {
        // High quality Haas effect tuned to be subtle (<10ms)
        const input = this.ctx.createGain();
        const output = this.ctx.createGain();
        const dry = this.ctx.createGain();
        const wet = this.ctx.createGain();

        // Filter for width (only widen above 300Hz to keep bass mono)
        const hp = this.ctx.createBiquadFilter();
        hp.type = 'highpass';
        hp.frequency.value = 300;

        const delay = this.ctx.createDelay();
        delay.delayTime.value = 0.007; // 7ms

        input.connect(dry);
        input.connect(hp);
        hp.connect(delay);
        delay.connect(wet);
        dry.connect(output);
        wet.connect(output);

        return {
            input,
            output,
            dry,
            wet,
            setWidth(val) {
                // val 0-1
                wet.gain.setTargetAtTime(val * 0.4, input.context.currentTime, 0.02);
            }
        };
    }

    _createPostEQ() {
        const air = this.ctx.createBiquadFilter();
        air.type = 'highshelf';
        air.frequency.value = 12000;

        const presence = this.ctx.createBiquadFilter();
        presence.type = 'peaking';
        presence.frequency.value = 3500;
        presence.Q.value = 1.0;

        air.connect(presence);

        return {
            input: air,
            output: presence,
            air,
            presence
        };
    }

    _createLimiterPlaceholder() {
        // Standard compressor fallback until Worklet loads
        const comp = this.ctx.createDynamicsCompressor();
        comp.attack.value = 0.005;
        comp.release.value = 0.1;
        comp.ratio.value = 20;
        comp.threshold.value = -1;

        const input = this.ctx.createGain();
        const output = this.ctx.createGain();

        input.connect(comp);
        comp.connect(output);

        return {
            input,
            output,
            comp,
            setThreshold(db) {
                this._pendingThreshold = db;
                // Fallback
                try { comp.threshold.value = db; } catch (e) { }
                // Worklet
                if (this.worklet) {
                    const param = this.worklet.parameters.get('threshold');
                    if (param) param.value = db;
                }
            },
            setCeiling(db) {
                this._pendingCeiling = db;
                // Worklet
                if (this.worklet) {
                    const param = this.worklet.parameters.get('ceiling');
                    if (param) param.value = db;
                }
            }
        };
    }

    _buildChain() {
        this.input.connect(this.inputTrim);
        this.inputTrim.connect(this.saturator.input);
        this.saturator.output.connect(this.preEQ.input);
        this.preEQ.output.connect(this.multibandComp.input);
        this.multibandComp.output.connect(this.stereoWidener.input);
        this.stereoWidener.output.connect(this.postEQ.input);
        this.postEQ.output.connect(this.limiter.input);
        this.limiter.output.connect(this.outputGain);
        this.outputGain.connect(this.lufsAnalyser);
        this.outputGain.connect(this.output);
        this.output.connect(this.destination);
    }

    setInputTrim(db) {
        const linear = Math.pow(10, db / 20);
        this.inputTrim.gain.setTargetAtTime(linear, this.ctx.currentTime, 0.02);
    }

    setOutputGain(db) {
        const linear = Math.pow(10, db / 20);
        this.outputGain.gain.setTargetAtTime(linear, this.ctx.currentTime, 0.02);
    }

    getLUFS() {
        const data = new Float32Array(this.lufsAnalyser.fftSize);
        this.lufsAnalyser.getFloatTimeDomainData(data);
        let sum = 0;
        for (let i = 0; i < data.length; i++) sum += data[i] * data[i];
        const rms = Math.sqrt(sum / data.length);
        const lufs = 20 * Math.log10(rms + 1e-9) - 0.691;
        return Math.max(-100, lufs);
    }

    applyPreset(name) {
        this.currentPreset = name;
        const preset = MASTERING_PRESETS[name];
        if (!preset) return;

        // Pre-EQ
        this.preEQ.lowShelf.gain.value = preset.preEQ.lowShelf;
        this.preEQ.midPeak.gain.value = preset.preEQ.mid;
        this.preEQ.highShelf.gain.value = preset.preEQ.highShelf;
        this.preEQ.lowCut.frequency.value = preset.preEQ.lowCut;

        // Multiband comp thresholds
        this.multibandComp.bands.low.comp.threshold.value = preset.comp.lowThresh;
        this.multibandComp.bands.mid.comp.threshold.value = preset.comp.midThresh;
        this.multibandComp.bands.high.comp.threshold.value = preset.comp.highThresh;

        // Stereo width
        this.stereoWidener.setWidth(preset.stereoWidth || 0.3);

        // Post-EQ
        if (preset.postEQ) {
            this.postEQ.air.gain.value = preset.postEQ.air;
            this.postEQ.presence.gain.value = preset.postEQ.presence;
        }

        // Limiter
        this.limiter.setThreshold(preset.limiter.threshold);
        this.limiter.setCeiling(preset.limiter.ceiling);

        // Saturator
        if (this.saturator.drive) this.saturator.drive.gain.value = preset.drive || 1.0;
    }

    dispose() {
        this.input.disconnect();
        this.output.disconnect();
    }
}

export const MASTERING_PRESETS = {
    Streaming: {
        name: 'Streaming (Spotify/Apple)',
        target: '-14 LUFS',
        preEQ: { lowCut: 30, lowShelf: 1, mid: 0, highShelf: 0.5 },
        comp: { lowThresh: -18, midThresh: -20, highThresh: -22 },
        stereoWidth: 0.3,
        postEQ: { air: 1.5, presence: 0.5 },
        limiter: { threshold: -1, ceiling: -1 },
        drive: 1.1
    },
    CD: {
        name: 'CD / Loud Master',
        target: '-9 LUFS',
        preEQ: { lowCut: 25, lowShelf: 2, mid: 0.5, highShelf: 1 },
        comp: { lowThresh: -15, midThresh: -16, highThresh: -18 },
        stereoWidth: 0.4,
        postEQ: { air: 2, presence: 1 },
        limiter: { threshold: -0.3, ceiling: -0.1 },
        drive: 1.3
    },
    Vinyl: {
        name: 'Vinyl / Analog',
        target: '-12 LUFS',
        preEQ: { lowCut: 40, lowShelf: 0, mid: -0.5, highShelf: -1 },
        comp: { lowThresh: -20, midThresh: -22, highThresh: -24 },
        stereoWidth: 0.2,
        postEQ: { air: 0, presence: 0 },
        limiter: { threshold: -2, ceiling: -1 },
        drive: 1.0
    },
    Broadcast: {
        name: 'Broadcast / Radio',
        target: '-16 LUFS',
        preEQ: { lowCut: 50, lowShelf: -1, mid: 1, highShelf: 0 },
        comp: { lowThresh: -16, midThresh: -18, highThresh: -20 },
        stereoWidth: 0.2,
        postEQ: { air: 1, presence: 1.5 },
        limiter: { threshold: -1.5, ceiling: -1 },
        drive: 1.1
    },
    Podcast: {
        name: 'Podcast / Voice',
        target: '-16 LUFS',
        preEQ: { lowCut: 80, lowShelf: -2, mid: 2, highShelf: 1 },
        comp: { lowThresh: -14, midThresh: -16, highThresh: -20 },
        stereoWidth: 0,
        postEQ: { air: 1, presence: 2 },
        limiter: { threshold: -1, ceiling: -1 },
        drive: 1.0
    },
};
