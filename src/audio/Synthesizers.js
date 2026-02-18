// ============================================
// ORPHEUS DAW — Wavetable Synthesizer
// ============================================
// Web Audio based wavetable synth with morphing and modulation

export class WavetableSynth {
    static WAVEFORMS = ['sine', 'triangle', 'sawtooth', 'square', 'pulse', 'supersaw'];

    constructor(audioContext) {
        this.ctx = audioContext;
        this.output = this.ctx.createGain();
        this.output.gain.value = 0.5;

        // Oscillator parameters
        this.voices = [];
        this.config = {
            waveform: 'sawtooth',
            detune: 0,
            unisonVoices: 1,
            unisonSpread: 10,
            attack: 0.01,
            decay: 0.2,
            sustain: 0.7,
            release: 0.3,
            filterCutoff: 8000,
            filterResonance: 1,
            filterType: 'lowpass',
            filterEnvAmount: 0,
            lfoRate: 2,
            lfoDepth: 0,
            lfoTarget: 'pitch', // pitch, filter, volume
        };
    }

    noteOn(note, velocity = 100) {
        const freq = 440 * Math.pow(2, (note - 69) / 12);
        const vel = velocity / 127;
        const voice = this._createVoice(freq, vel);
        this.voices.push({ note, voice });
        return voice;
    }

    noteOff(note) {
        const idx = this.voices.findIndex(v => v.note === note);
        if (idx === -1) return;
        const { voice } = this.voices[idx];
        const now = this.ctx.currentTime;
        voice.envelope.gain.setTargetAtTime(0, now, this.config.release / 4);
        voice.stopTime = now + this.config.release;
        setTimeout(() => {
            try {
                voice.oscillators.forEach(o => o.stop());
                voice.envelope.disconnect();
            } catch (e) { /* already stopped */ }
        }, this.config.release * 1000 + 100);
        this.voices.splice(idx, 1);
    }

    _createVoice(freq, vel) {
        const envelope = this.ctx.createGain();
        envelope.gain.value = 0;

        // Filter
        const filter = this.ctx.createBiquadFilter();
        filter.type = this.config.filterType;
        filter.frequency.value = this.config.filterCutoff;
        filter.Q.value = this.config.filterResonance;

        // Create unison oscillators
        const oscillators = [];
        for (let i = 0; i < this.config.unisonVoices; i++) {
            const osc = this.ctx.createOscillator();
            osc.type = this.config.waveform === 'supersaw' ? 'sawtooth' : this.config.waveform;
            const spreadOffset = this.config.unisonVoices > 1
                ? (i / (this.config.unisonVoices - 1) - 0.5) * this.config.unisonSpread
                : 0;
            osc.frequency.value = freq;
            osc.detune.value = this.config.detune + spreadOffset;
            osc.connect(filter);
            osc.start();
            oscillators.push(osc);
        }

        filter.connect(envelope);
        envelope.connect(this.output);

        // ADSR
        const now = this.ctx.currentTime;
        envelope.gain.setValueAtTime(0, now);
        envelope.gain.linearRampToValueAtTime(vel, now + this.config.attack);
        envelope.gain.linearRampToValueAtTime(
            vel * this.config.sustain,
            now + this.config.attack + this.config.decay
        );

        // Filter envelope
        if (this.config.filterEnvAmount !== 0) {
            const envFreq = this.config.filterCutoff + this.config.filterEnvAmount * 4000;
            filter.frequency.setValueAtTime(this.config.filterCutoff, now);
            filter.frequency.linearRampToValueAtTime(envFreq, now + this.config.attack);
            filter.frequency.linearRampToValueAtTime(this.config.filterCutoff, now + this.config.attack + this.config.decay);
        }

        // LFO
        if (this.config.lfoDepth > 0) {
            const lfo = this.ctx.createOscillator();
            const lfoGain = this.ctx.createGain();
            lfo.frequency.value = this.config.lfoRate;
            lfoGain.gain.value = this.config.lfoDepth;

            if (this.config.lfoTarget === 'pitch') {
                oscillators.forEach(o => lfo.connect(lfoGain).connect(o.detune));
            } else if (this.config.lfoTarget === 'filter') {
                lfo.connect(lfoGain);
                lfoGain.connect(filter.frequency);
            } else if (this.config.lfoTarget === 'volume') {
                lfo.connect(lfoGain);
                lfoGain.connect(envelope.gain);
            }
            lfo.start();
        }

        return { oscillators, filter, envelope };
    }

    setConfig(params) {
        Object.assign(this.config, params);
    }

    panic() {
        this.voices.forEach(v => {
            try { v.voice.oscillators.forEach(o => o.stop()); } catch (e) { }
            try { v.voice.envelope.disconnect(); } catch (e) { }
        });
        this.voices = [];
    }

    destroy() {
        this.panic();
        this.output.disconnect();
    }
}

// ============================================
// FM Synthesizer (4-operator)
// ============================================
export class FMSynth {
    constructor(audioContext) {
        this.ctx = audioContext;
        this.output = this.ctx.createGain();
        this.output.gain.value = 0.4;
        this.voices = [];
        this.config = {
            algorithm: 1,
            operators: [
                { ratio: 1, level: 1, attack: 0.01, decay: 0.3, sustain: 0.5, release: 0.3 },
                { ratio: 2, level: 0.5, attack: 0.01, decay: 0.2, sustain: 0.3, release: 0.2 },
                { ratio: 3, level: 0.3, attack: 0.01, decay: 0.1, sustain: 0.1, release: 0.1 },
                { ratio: 4, level: 0.1, attack: 0.01, decay: 0.05, sustain: 0, release: 0.1 },
            ],
        };
    }

    noteOn(note, velocity = 100) {
        const freq = 440 * Math.pow(2, (note - 69) / 12);
        const vel = velocity / 127;
        const voice = this._createFMVoice(freq, vel);
        this.voices.push({ note, voice });
    }

    noteOff(note) {
        const idx = this.voices.findIndex(v => v.note === note);
        if (idx === -1) return;
        const { voice } = this.voices[idx];
        const now = this.ctx.currentTime;
        voice.envelopes.forEach((env, i) => {
            const rel = this.config.operators[i].release;
            env.gain.setTargetAtTime(0, now, rel / 4);
        });
        setTimeout(() => {
            try { voice.oscillators.forEach(o => o.stop()); } catch (e) { }
        }, 1000);
        this.voices.splice(idx, 1);
    }

    _createFMVoice(freq, vel) {
        const ops = this.config.operators;
        const oscillators = [];
        const envelopes = [];
        const gains = [];

        // Create 4 operators
        for (let i = 0; i < 4; i++) {
            const osc = this.ctx.createOscillator();
            osc.type = 'sine';
            osc.frequency.value = freq * ops[i].ratio;

            const env = this.ctx.createGain();
            env.gain.value = 0;

            const gain = this.ctx.createGain();
            gain.gain.value = ops[i].level * vel * 500; // FM index

            osc.connect(gain);
            gain.connect(env);

            // ADSR
            const now = this.ctx.currentTime;
            env.gain.setValueAtTime(0, now);
            env.gain.linearRampToValueAtTime(1, now + ops[i].attack);
            env.gain.linearRampToValueAtTime(ops[i].sustain, now + ops[i].attack + ops[i].decay);

            osc.start();
            oscillators.push(osc);
            envelopes.push(env);
            gains.push(gain);
        }

        // Algorithm 1: Op4→Op3→Op2→Op1→Output
        envelopes[3].connect(oscillators[2].frequency);
        envelopes[2].connect(oscillators[1].frequency);
        envelopes[1].connect(oscillators[0].frequency);

        const outGain = this.ctx.createGain();
        outGain.gain.value = vel;
        envelopes[0].connect(outGain);
        outGain.connect(this.output);

        return { oscillators, envelopes, gains, outGain };
    }

    setConfig(params) { Object.assign(this.config, params); }
    panic() { this.voices.forEach(v => { try { v.voice.oscillators.forEach(o => o.stop()); } catch (e) { } }); this.voices = []; }
    destroy() { this.panic(); this.output.disconnect(); }
}

// ============================================
// Drum Machine
// ============================================
export class DrumMachine {
    constructor(audioContext) {
        this.ctx = audioContext;
        this.output = this.ctx.createGain();
        this.output.gain.value = 0.8;
        this.samples = {}; // Map<string, AudioBuffer>
    }

    // Synthesize basic drum sounds
    kick(time = 0) {
        const osc = this.ctx.createOscillator();
        const gain = this.ctx.createGain();
        osc.frequency.setValueAtTime(150, this.ctx.currentTime + time);
        osc.frequency.exponentialRampToValueAtTime(40, this.ctx.currentTime + time + 0.1);
        gain.gain.setValueAtTime(1, this.ctx.currentTime + time);
        gain.gain.exponentialRampToValueAtTime(0.001, this.ctx.currentTime + time + 0.4);
        osc.connect(gain);
        gain.connect(this.output);
        osc.start(this.ctx.currentTime + time);
        osc.stop(this.ctx.currentTime + time + 0.4);
    }

    snare(time = 0) {
        // Noise burst + pitched body
        const bufferSize = this.ctx.sampleRate * 0.2;
        const buffer = this.ctx.createBuffer(1, bufferSize, this.ctx.sampleRate);
        const data = buffer.getChannelData(0);
        for (let i = 0; i < bufferSize; i++) data[i] = Math.random() * 2 - 1;

        const noise = this.ctx.createBufferSource();
        noise.buffer = buffer;
        const noiseGain = this.ctx.createGain();
        noiseGain.gain.setValueAtTime(0.8, this.ctx.currentTime + time);
        noiseGain.gain.exponentialRampToValueAtTime(0.001, this.ctx.currentTime + time + 0.2);
        const filter = this.ctx.createBiquadFilter();
        filter.type = 'highpass';
        filter.frequency.value = 2000;
        noise.connect(filter);
        filter.connect(noiseGain);
        noiseGain.connect(this.output);
        noise.start(this.ctx.currentTime + time);

        const osc = this.ctx.createOscillator();
        const oscGain = this.ctx.createGain();
        osc.frequency.value = 200;
        oscGain.gain.setValueAtTime(0.5, this.ctx.currentTime + time);
        oscGain.gain.exponentialRampToValueAtTime(0.001, this.ctx.currentTime + time + 0.1);
        osc.connect(oscGain);
        oscGain.connect(this.output);
        osc.start(this.ctx.currentTime + time);
        osc.stop(this.ctx.currentTime + time + 0.1);
    }

    hihat(time = 0, open = false) {
        const bufferSize = this.ctx.sampleRate * (open ? 0.3 : 0.05);
        const buffer = this.ctx.createBuffer(1, bufferSize, this.ctx.sampleRate);
        const data = buffer.getChannelData(0);
        for (let i = 0; i < bufferSize; i++) data[i] = Math.random() * 2 - 1;

        const src = this.ctx.createBufferSource();
        src.buffer = buffer;
        const gain = this.ctx.createGain();
        gain.gain.setValueAtTime(0.4, this.ctx.currentTime + time);
        gain.gain.exponentialRampToValueAtTime(0.001, this.ctx.currentTime + time + (open ? 0.3 : 0.05));
        const filter = this.ctx.createBiquadFilter();
        filter.type = 'highpass';
        filter.frequency.value = 6000;
        src.connect(filter);
        filter.connect(gain);
        gain.connect(this.output);
        src.start(this.ctx.currentTime + time);
    }

    destroy() { this.output.disconnect(); }
}
