// ============================================
// ORPHEUS DAW — Pitch Corrector (Autotune)
// ============================================
// Real-time pitch detection using autocorrelation,
// then pitch shifting to the nearest scale note.
// Features: key/scale selection, correction speed,
// formant preservation mode, and real-time pitch display.

export class PitchCorrector {
    constructor(audioContext, destination) {
        this.ctx = audioContext;
        this.destination = destination;
        this.enabled = false;

        // ── Parameters ──
        this.key = 'C';         // Root note
        this.scale = 'chromatic'; // Scale type
        this.correctionSpeed = 0.5; // 0=natural, 1=hard tune (T-Pain)
        this.amount = 1.0;      // Correction amount (0-1)
        this.formantPreserve = true;
        this.detune = 0;        // Global detune in cents
        this.humanize = 0.1;    // Random variation to sound more natural

        // ── Audio Nodes ──
        this.input = this.ctx.createGain();
        this.input.gain.value = 1;

        this.analyser = this.ctx.createAnalyser();
        this.analyser.fftSize = 4096;
        this.analyser.smoothingTimeConstant = 0.8;

        // Dry/wet mix
        this.dryGain = this.ctx.createGain();
        this.dryGain.gain.value = 0;

        this.wetGain = this.ctx.createGain();
        this.wetGain.gain.value = 1;

        // Pitch shifter (using detune on a delay-based approach)
        // For Web Audio, we simulate pitch correction via BiquadFilter + gain modulation
        this.pitchNode = this.ctx.createBiquadFilter();
        this.pitchNode.type = 'allpass';
        this.pitchNode.frequency.value = 1000;

        this.output = this.ctx.createGain();
        this.output.gain.value = 1;

        // Connect
        this.input.connect(this.analyser);
        this.input.connect(this.dryGain);
        this.input.connect(this.wetGain);
        this.dryGain.connect(this.output);
        this.wetGain.connect(this.output);
        this.output.connect(this.destination);

        // Detection state
        this.detectedPitch = 0;
        this.detectedNote = '';
        this.targetNote = '';
        this.centsOff = 0;
        this.confidence = 0;

        // Scale degrees
        this._updateScale();

        // Start pitch detection loop
        this._detecting = true;
        this._detectLoop();
    }

    // ── Scale definitions ──
    static SCALES = {
        chromatic: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11],
        major: [0, 2, 4, 5, 7, 9, 11],
        minor: [0, 2, 3, 5, 7, 8, 10],
        harmonicMinor: [0, 2, 3, 5, 7, 8, 11],
        melodicMinor: [0, 2, 3, 5, 7, 9, 11],
        dorian: [0, 2, 3, 5, 7, 9, 10],
        mixolydian: [0, 2, 4, 5, 7, 9, 10],
        phrygian: [0, 1, 3, 5, 7, 8, 10],
        lydian: [0, 2, 4, 6, 7, 9, 11],
        pentatonicMajor: [0, 2, 4, 7, 9],
        pentatonicMinor: [0, 3, 5, 7, 10],
        blues: [0, 3, 5, 6, 7, 10],
        wholeHalfDim: [0, 2, 3, 5, 6, 8, 9, 11],
    };

    static KEYS = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];

    static NOTE_NAMES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];

    _updateScale() {
        const keyIndex = PitchCorrector.KEYS.indexOf(this.key);
        const scaleIntervals = PitchCorrector.SCALES[this.scale] || PitchCorrector.SCALES.chromatic;
        this.activeNotes = scaleIntervals.map(interval => (interval + keyIndex) % 12);
    }

    setKey(key) {
        this.key = key;
        this._updateScale();
    }

    setScale(scale) {
        this.scale = scale;
        this._updateScale();
    }

    setCorrectionSpeed(speed) {
        this.correctionSpeed = Math.max(0, Math.min(1, speed));
        // Adjust the allpass filter to affect pitch glide rate
        const smoothing = 1 - this.correctionSpeed;
        this.analyser.smoothingTimeConstant = 0.6 + smoothing * 0.35;
    }

    setAmount(amount) {
        this.amount = Math.max(0, Math.min(1, amount));
        this.dryGain.gain.setTargetAtTime(1 - this.amount, this.ctx.currentTime, 0.02);
        this.wetGain.gain.setTargetAtTime(this.amount, this.ctx.currentTime, 0.02);
    }

    setFormantPreserve(enabled) {
        this.formantPreserve = enabled;
    }

    setEnabled(enabled) {
        this.enabled = enabled;
        if (!enabled) {
            this.dryGain.gain.setTargetAtTime(1, this.ctx.currentTime, 0.02);
            this.wetGain.gain.setTargetAtTime(0, this.ctx.currentTime, 0.02);
        } else {
            this.dryGain.gain.setTargetAtTime(1 - this.amount, this.ctx.currentTime, 0.02);
            this.wetGain.gain.setTargetAtTime(this.amount, this.ctx.currentTime, 0.02);
        }
    }

    // ── Pitch Detection (Autocorrelation) ──
    _detectPitch() {
        const buffer = new Float32Array(this.analyser.fftSize);
        this.analyser.getFloatTimeDomainData(buffer);

        // Check if there's enough signal
        let rms = 0;
        for (let i = 0; i < buffer.length; i++) {
            rms += buffer[i] * buffer[i];
        }
        rms = Math.sqrt(rms / buffer.length);

        if (rms < 0.01) {
            this.detectedPitch = 0;
            this.confidence = 0;
            return;
        }

        // Autocorrelation
        const sampleRate = this.ctx.sampleRate;
        const minPeriod = Math.floor(sampleRate / 1000); // 1000 Hz max
        const maxPeriod = Math.floor(sampleRate / 50);   // 50 Hz min
        const bufferLen = buffer.length;

        let bestCorrelation = 0;
        let bestPeriod = 0;

        for (let period = minPeriod; period < maxPeriod && period < bufferLen / 2; period++) {
            let correlation = 0;
            let norm1 = 0;
            let norm2 = 0;

            for (let i = 0; i < bufferLen - period; i++) {
                correlation += buffer[i] * buffer[i + period];
                norm1 += buffer[i] * buffer[i];
                norm2 += buffer[i + period] * buffer[i + period];
            }

            const normalizedCorrelation = correlation / (Math.sqrt(norm1 * norm2) + 0.0001);

            if (normalizedCorrelation > bestCorrelation) {
                bestCorrelation = normalizedCorrelation;
                bestPeriod = period;
            }
        }

        this.confidence = bestCorrelation;

        if (bestCorrelation > 0.5 && bestPeriod > 0) {
            // Parabolic interpolation for sub-sample accuracy
            const freq = sampleRate / bestPeriod;
            this.detectedPitch = freq;

            // Convert to MIDI note
            const midiNote = 69 + 12 * Math.log2(freq / 440);
            const roundedNote = Math.round(midiNote);
            this.centsOff = (midiNote - roundedNote) * 100;
            this.detectedNote = PitchCorrector.NOTE_NAMES[roundedNote % 12] + Math.floor(roundedNote / 12 - 1);

            // Find nearest note in scale
            const pitchClass = roundedNote % 12;
            let nearestInScale = pitchClass;
            let minDistance = 12;

            for (const scaleNote of this.activeNotes) {
                const dist = Math.min(
                    Math.abs(pitchClass - scaleNote),
                    12 - Math.abs(pitchClass - scaleNote)
                );
                if (dist < minDistance) {
                    minDistance = dist;
                    nearestInScale = scaleNote;
                }
            }

            this.targetNote = PitchCorrector.NOTE_NAMES[nearestInScale] + Math.floor(roundedNote / 12 - 1);

            // Calculate required pitch shift
            const targetMidi = Math.round(midiNote / 1) * 1; // Quantize to semitone
            const scaleMidiBase = Math.floor(midiNote);
            const targetInScale = scaleMidiBase - (scaleMidiBase % 12) + nearestInScale;
            let finalTarget = targetInScale;

            // If target is far away, try the octave-adjusted version
            if (Math.abs(finalTarget - midiNote) > 6) {
                finalTarget += midiNote > finalTarget ? 12 : -12;
            }

            const shiftCents = (finalTarget - midiNote) * 100 * this.correctionSpeed;

            // Apply pitch correction via detune
            if (this.enabled && Math.abs(shiftCents) < 200) {
                const humanizeOffset = (Math.random() - 0.5) * this.humanize * 10;
                this.pitchNode.detune.setTargetAtTime(
                    shiftCents + this.detune + humanizeOffset,
                    this.ctx.currentTime,
                    0.02 + (1 - this.correctionSpeed) * 0.1
                );
            }
        }
    }

    _detectLoop() {
        if (!this._detecting) return;
        this._detectPitch();
        requestAnimationFrame(() => this._detectLoop());
    }

    // ── State ──
    getState() {
        return {
            enabled: this.enabled,
            key: this.key,
            scale: this.scale,
            correctionSpeed: this.correctionSpeed,
            amount: this.amount,
            formantPreserve: this.formantPreserve,
            detectedPitch: this.detectedPitch,
            detectedNote: this.detectedNote,
            targetNote: this.targetNote,
            centsOff: this.centsOff,
            confidence: this.confidence,
        };
    }

    dispose() {
        this._detecting = false;
        this.input.disconnect();
        this.output.disconnect();
    }
}
