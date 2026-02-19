// ============================================
// ORPHEUS DAW — Synthesizer
// ============================================

export class Synthesizer {
    constructor(audioContext, destination) {
        this.ctx = audioContext;
        this.destination = destination;
        this.voices = new Map();
        this.maxVoices = 16;

        // Params
        this.waveform = 'sawtooth'; // sine, square, sawtooth, triangle
        this.attack = 0.01;
        this.decay = 0.3;
        this.sustain = 0.5;
        this.release = 0.5;

        // Filter
        this.filterType = 'lowpass';
        this.filterCutoff = 5000;
        this.filterResonance = 1;

        // Master gain
        this.gainNode = this.ctx.createGain();
        this.gainNode.gain.value = 0.5;

        // Filter node
        this.filter = this.ctx.createBiquadFilter();
        this.filter.type = this.filterType;
        this.filter.frequency.value = this.filterCutoff;
        this.filter.Q.value = this.filterResonance;

        this.filter.connect(this.gainNode);
        this.gainNode.connect(this.destination);
    }

    noteOn(note, velocity = 0.8) {
        if (this.voices.has(note)) {
            this.noteOff(note);
        }

        const freq = 440 * Math.pow(2, (note - 69) / 12);
        const now = this.ctx.currentTime;

        const osc = this.ctx.createOscillator();
        osc.type = this.waveform;
        osc.frequency.setValueAtTime(freq, now);

        const envGain = this.ctx.createGain();
        envGain.gain.setValueAtTime(0, now);
        envGain.gain.linearRampToValueAtTime(velocity, now + this.attack);
        envGain.gain.linearRampToValueAtTime(
            velocity * this.sustain,
            now + this.attack + this.decay
        );

        osc.connect(envGain);
        envGain.connect(this.filter);
        osc.start(now);

        this.voices.set(note, { osc, envGain, velocity });

        // Voice stealing
        if (this.voices.size > this.maxVoices) {
            const oldest = this.voices.keys().next().value;
            this.noteOff(oldest);
        }
    }

    noteOff(note) {
        const voice = this.voices.get(note);
        if (!voice) return;

        const now = this.ctx.currentTime;
        voice.envGain.gain.cancelScheduledValues(now);
        voice.envGain.gain.setValueAtTime(voice.envGain.gain.value, now);
        voice.envGain.gain.linearRampToValueAtTime(0, now + this.release);
        voice.osc.stop(now + this.release + 0.1);

        this.voices.delete(note);
    }

    allNotesOff() {
        for (const note of this.voices.keys()) {
            this.noteOff(note);
        }
    }

    setWaveform(type) {
        this.waveform = type;
        for (const voice of this.voices.values()) {
            voice.osc.type = type;
        }
    }

    setFilterCutoff(freq) {
        this.filterCutoff = freq;
        this.filter.frequency.setValueAtTime(freq, this.ctx.currentTime);
    }

    setFilterResonance(q) {
        this.filterResonance = q;
        this.filter.Q.setValueAtTime(q, this.ctx.currentTime);
    }

    setEnvelope(a, d, s, r) {
        this.attack = a;
        this.decay = d;
        this.sustain = s;
        this.release = r;
    }

    dispose() {
        this.allNotesOff();
        this.gainNode.disconnect();
        this.filter.disconnect();
    }
}
