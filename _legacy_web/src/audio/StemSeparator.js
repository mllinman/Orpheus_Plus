// ============================================
// ORPHEUS DAW — STEM Separator
// ============================================
// Uses frequency-band isolation via crossover filters
// to separate audio into Vocals, Drums, Bass, and Other stems.
// This is a real-time Web Audio API approach using spectral filtering.

export class StemSeparator {
    constructor(audioContext, destination) {
        this.ctx = audioContext;
        this.destination = destination;

        // Input gain
        this.input = this.ctx.createGain();
        this.input.gain.value = 1;

        // ── Stem channels ──
        this.stems = {
            vocals: this._createStemChannel('vocals', [300, 4000], 'bandpass'),
            drums: this._createStemChannel('drums', [80, 8000], 'peaking'),
            bass: this._createStemChannel('bass', [20, 250], 'lowpass'),
            other: this._createStemChannel('other', [4000, 20000], 'highpass'),
        };

        // Connect input to all stem processing chains
        this._buildRoutingGraph();

        // Analysers for metering
        this.analysers = {};
        for (const [name, stem] of Object.entries(this.stems)) {
            const analyser = this.ctx.createAnalyser();
            analyser.fftSize = 1024;
            analyser.smoothingTimeConstant = 0.85;
            stem.output.connect(analyser);
            this.analysers[name] = analyser;
        }
    }

    _createStemChannel(name, freqRange, filterType) {
        const gain = this.ctx.createGain();
        gain.gain.value = 1;

        const solo = false;
        const mute = false;

        // Create filter chain for frequency isolation
        const filters = [];

        if (filterType === 'bandpass' || filterType === 'peaking') {
            // Bandpass: highpass at low freq + lowpass at high freq
            const hp = this.ctx.createBiquadFilter();
            hp.type = 'highpass';
            hp.frequency.value = freqRange[0];
            hp.Q.value = 0.7;
            filters.push(hp);

            const lp = this.ctx.createBiquadFilter();
            lp.type = 'lowpass';
            lp.frequency.value = freqRange[1];
            lp.Q.value = 0.7;
            filters.push(lp);

            // For drums, add a transient emphasis filter
            if (filterType === 'peaking') {
                const peak = this.ctx.createBiquadFilter();
                peak.type = 'peaking';
                peak.frequency.value = 200;
                peak.gain.value = 3;
                peak.Q.value = 0.5;
                filters.push(peak);

                // High-frequency click/attack emphasis
                const attack = this.ctx.createBiquadFilter();
                attack.type = 'peaking';
                attack.frequency.value = 5000;
                attack.gain.value = 4;
                attack.Q.value = 1;
                filters.push(attack);
            }

            // For vocals, add presence emphasis
            if (name === 'vocals') {
                const presence = this.ctx.createBiquadFilter();
                presence.type = 'peaking';
                presence.frequency.value = 2500;
                presence.gain.value = 3;
                presence.Q.value = 1.2;
                filters.push(presence);

                // Reduce low-mid mud
                const deMud = this.ctx.createBiquadFilter();
                deMud.type = 'peaking';
                deMud.frequency.value = 400;
                deMud.gain.value = -2;
                deMud.Q.value = 0.8;
                filters.push(deMud);
            }
        } else if (filterType === 'lowpass') {
            const lp = this.ctx.createBiquadFilter();
            lp.type = 'lowpass';
            lp.frequency.value = freqRange[1];
            lp.Q.value = 1.2;
            filters.push(lp);

            // Sub boost
            const sub = this.ctx.createBiquadFilter();
            sub.type = 'peaking';
            sub.frequency.value = 60;
            sub.gain.value = 3;
            sub.Q.value = 1;
            filters.push(sub);
        } else if (filterType === 'highpass') {
            const hp = this.ctx.createBiquadFilter();
            hp.type = 'highpass';
            hp.frequency.value = freqRange[0];
            hp.Q.value = 0.7;
            filters.push(hp);
        }

        // Chain filters
        for (let i = 0; i < filters.length - 1; i++) {
            filters[i].connect(filters[i + 1]);
        }

        // Connect last filter to gain
        const lastFilter = filters[filters.length - 1];
        lastFilter.connect(gain);

        const output = this.ctx.createGain();
        output.gain.value = 1;
        gain.connect(output);

        return {
            name,
            filters,
            input: filters[0],
            gain,
            output,
            volume: 1,
            solo: false,
            mute: false,
            pan: 0,
            freqRange,
        };
    }

    _buildRoutingGraph() {
        for (const stem of Object.values(this.stems)) {
            this.input.connect(stem.input);
            stem.output.connect(this.destination);
        }
    }

    setStemVolume(stemName, volume) {
        const stem = this.stems[stemName];
        if (stem) {
            stem.volume = volume;
            stem.gain.gain.setTargetAtTime(volume, this.ctx.currentTime, 0.02);
        }
    }

    setStemMute(stemName, mute) {
        const stem = this.stems[stemName];
        if (stem) {
            stem.mute = mute;
            this._updateStemRouting();
        }
    }

    setStemSolo(stemName, solo) {
        const stem = this.stems[stemName];
        if (stem) {
            stem.solo = solo;
            this._updateStemRouting();
        }
    }

    _updateStemRouting() {
        const hasSolo = Object.values(this.stems).some(s => s.solo);

        for (const stem of Object.values(this.stems)) {
            let shouldPlay;
            if (hasSolo) {
                shouldPlay = stem.solo && !stem.mute;
            } else {
                shouldPlay = !stem.mute;
            }

            const targetGain = shouldPlay ? stem.volume : 0;
            stem.output.gain.setTargetAtTime(targetGain, this.ctx.currentTime, 0.02);
        }
    }

    getStemLevel(stemName) {
        const analyser = this.analysers[stemName];
        if (!analyser) return 0;
        const data = new Float32Array(analyser.fftSize);
        analyser.getFloatTimeDomainData(data);
        let sum = 0;
        for (let i = 0; i < data.length; i++) {
            sum += data[i] * data[i];
        }
        return Math.sqrt(sum / data.length);
    }

    getState() {
        const state = {};
        for (const [name, stem] of Object.entries(this.stems)) {
            state[name] = {
                volume: stem.volume,
                mute: stem.mute,
                solo: stem.solo,
                level: this.getStemLevel(name),
            };
        }
        return state;
    }

    disconnect() {
        this.input.disconnect();
        for (const stem of Object.values(this.stems)) {
            stem.output.disconnect();
        }
    }

    dispose() {
        this.disconnect();
    }
}
