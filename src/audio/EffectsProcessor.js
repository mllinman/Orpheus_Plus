// ============================================
// ORPHEUS DAW — Effects Processor
// ============================================

export function createEQ(ctx) {
    const bands = [
        { type: 'lowshelf', frequency: 100, gain: 0 },
        { type: 'peaking', frequency: 500, gain: 0, Q: 1.4 },
        { type: 'peaking', frequency: 2000, gain: 0, Q: 1.4 },
        { type: 'highshelf', frequency: 8000, gain: 0 }
    ];

    const filters = bands.map(b => {
        const f = ctx.createBiquadFilter();
        f.type = b.type;
        f.frequency.value = b.frequency;
        f.gain.value = b.gain;
        if (b.Q) f.Q.value = b.Q;
        return f;
    });

    // Chain
    for (let i = 0; i < filters.length - 1; i++) {
        filters[i].connect(filters[i + 1]);
    }

    return {
        type: 'eq',
        name: 'Parametric EQ',
        input: filters[0],
        output: filters[filters.length - 1],
        filters,
        bands,
        bypass: false,
        setBand(index, freq, gain, q) {
            if (freq !== undefined) filters[index].frequency.value = freq;
            if (gain !== undefined) filters[index].gain.value = gain;
            if (q !== undefined && filters[index].type === 'peaking') filters[index].Q.value = q;
            bands[index] = { ...bands[index], frequency: filters[index].frequency.value, gain: filters[index].gain.value };
        }
    };
}

export function createCompressor(ctx) {
    const comp = ctx.createDynamicsCompressor();
    comp.threshold.value = -24;
    comp.knee.value = 12;
    comp.ratio.value = 4;
    comp.attack.value = 0.003;
    comp.release.value = 0.25;

    const makeupGain = ctx.createGain();
    makeupGain.gain.value = 1;
    comp.connect(makeupGain);

    return {
        type: 'compressor',
        name: 'Compressor',
        input: comp,
        output: makeupGain,
        node: comp,
        makeupGain,
        bypass: false,
        setThreshold(v) { comp.threshold.value = v; },
        setRatio(v) { comp.ratio.value = v; },
        setAttack(v) { comp.attack.value = v; },
        setRelease(v) { comp.release.value = v; },
        setKnee(v) { comp.knee.value = v; },
        setMakeupGain(v) { makeupGain.gain.value = v; },
        getReduction() { return comp.reduction; }
    };
}

export function createReverb(ctx) {
    const convolver = ctx.createConvolver();
    const wetGain = ctx.createGain();
    const dryGain = ctx.createGain();
    const input = ctx.createGain();
    const output = ctx.createGain();

    wetGain.gain.value = 0.3;
    dryGain.gain.value = 0.7;

    input.connect(convolver);
    input.connect(dryGain);
    convolver.connect(wetGain);
    wetGain.connect(output);
    dryGain.connect(output);

    // Generate impulse response
    const duration = 2.5;
    const length = ctx.sampleRate * duration;
    const impulse = ctx.createBuffer(2, length, ctx.sampleRate);
    for (let ch = 0; ch < 2; ch++) {
        const data = impulse.getChannelData(ch);
        for (let i = 0; i < length; i++) {
            data[i] = (Math.random() * 2 - 1) * Math.exp(-i / (ctx.sampleRate * 0.8));
        }
    }
    convolver.buffer = impulse;

    return {
        type: 'reverb',
        name: 'Reverb',
        input,
        output,
        bypass: false,
        setMix(wet) {
            wetGain.gain.value = wet;
            dryGain.gain.value = 1 - wet;
        },
        setDecay(seconds) {
            const len = ctx.sampleRate * seconds;
            const imp = ctx.createBuffer(2, len, ctx.sampleRate);
            for (let ch = 0; ch < 2; ch++) {
                const d = imp.getChannelData(ch);
                for (let i = 0; i < len; i++) {
                    d[i] = (Math.random() * 2 - 1) * Math.exp(-i / (ctx.sampleRate * (seconds * 0.3)));
                }
            }
            convolver.buffer = imp;
        }
    };
}

export function createDelay(ctx) {
    const input = ctx.createGain();
    const output = ctx.createGain();
    const delay = ctx.createDelay(5.0);
    const feedback = ctx.createGain();
    const wetGain = ctx.createGain();
    const dryGain = ctx.createGain();

    delay.delayTime.value = 0.375;
    feedback.gain.value = 0.4;
    wetGain.gain.value = 0.3;
    dryGain.gain.value = 0.7;

    input.connect(delay);
    input.connect(dryGain);
    delay.connect(feedback);
    feedback.connect(delay);
    delay.connect(wetGain);
    wetGain.connect(output);
    dryGain.connect(output);

    return {
        type: 'delay',
        name: 'Delay',
        input,
        output,
        bypass: false,
        setTime(t) { delay.delayTime.value = t; },
        setFeedback(f) { feedback.gain.value = Math.min(0.95, f); },
        setMix(wet) {
            wetGain.gain.value = wet;
            dryGain.gain.value = 1 - wet;
        }
    };
}

export function createChorus(ctx) {
    const input = ctx.createGain();
    const output = ctx.createGain();
    const delay = ctx.createDelay(0.1);
    const lfo = ctx.createOscillator();
    const lfoGain = ctx.createGain();
    const wetGain = ctx.createGain();
    const dryGain = ctx.createGain();

    delay.delayTime.value = 0.015;
    lfo.frequency.value = 1.5;
    lfoGain.gain.value = 0.003;
    wetGain.gain.value = 0.5;
    dryGain.gain.value = 0.5;

    lfo.connect(lfoGain);
    lfoGain.connect(delay.delayTime);
    lfo.start();

    input.connect(delay);
    input.connect(dryGain);
    delay.connect(wetGain);
    wetGain.connect(output);
    dryGain.connect(output);

    return {
        type: 'chorus',
        name: 'Chorus',
        input,
        output,
        bypass: false,
        setRate(r) { lfo.frequency.value = r; },
        setDepth(d) { lfoGain.gain.value = d * 0.01; },
        setMix(wet) {
            wetGain.gain.value = wet;
            dryGain.gain.value = 1 - wet;
        }
    };
}

export const EFFECT_TYPES = [
    { id: 'eq', name: 'Parametric EQ', create: createEQ },
    { id: 'compressor', name: 'Compressor', create: createCompressor },
    { id: 'reverb', name: 'Reverb', create: createReverb },
    { id: 'delay', name: 'Delay', create: createDelay },
    { id: 'chorus', name: 'Chorus', create: createChorus },
    { id: 'saturation', name: 'Saturation', create: createSaturation },
    { id: 'gate', name: 'Noise Gate', create: createGate },
    { id: 'stereoWidener', name: 'Stereo Widener', create: createStereoWidener },
    { id: 'autoPan', name: 'Auto Pan', create: createAutoPan },
    { id: 'bitcrusher', name: 'Bitcrusher', create: createBitcrusher },
    { id: 'freqShifter', name: 'Frequency Shifter', create: createFrequencyShifter },
    { id: 'transientShaper', name: 'Transient Shaper', create: createTransientShaper },
];

// ─── Saturation (Waveshaper) ───
export function createSaturation(ctx) {
    const input = ctx.createGain();
    const output = ctx.createGain();
    const wetGain = ctx.createGain();
    const dryGain = ctx.createGain();
    const waveshaper = ctx.createWaveShaper();

    wetGain.gain.value = 0.5;
    dryGain.gain.value = 0.5;

    // Tape-style saturation curve
    const makeCurve = (drive) => {
        const samples = 44100;
        const curve = new Float32Array(samples);
        const k = drive * 50;
        for (let i = 0; i < samples; i++) {
            const x = (i * 2) / samples - 1;
            curve[i] = ((1 + k) * x) / (1 + k * Math.abs(x));
        }
        return curve;
    };
    waveshaper.curve = makeCurve(0.5);
    waveshaper.oversample = '4x';

    input.connect(waveshaper);
    input.connect(dryGain);
    waveshaper.connect(wetGain);
    wetGain.connect(output);
    dryGain.connect(output);

    return {
        type: 'saturation', name: 'Saturation', input, output, bypass: false,
        setDrive(v) { waveshaper.curve = makeCurve(v); },
        setMix(wet) { wetGain.gain.value = wet; dryGain.gain.value = 1 - wet; },
    };
}

// ─── Noise Gate ───
export function createGate(ctx) {
    const input = ctx.createGain();
    const output = ctx.createGain();
    const gateGain = ctx.createGain();
    const analyzer = ctx.createAnalyser();
    analyzer.fftSize = 256;

    input.connect(analyzer);
    input.connect(gateGain);
    gateGain.connect(output);

    let threshold = -40, attack = 0.001, release = 0.05, active = true;
    const dataArray = new Uint8Array(analyzer.frequencyBinCount);

    const process = () => {
        if (!active) return;
        analyzer.getByteTimeDomainData(dataArray);
        let sum = 0;
        for (let i = 0; i < dataArray.length; i++) {
            const v = (dataArray[i] - 128) / 128;
            sum += v * v;
        }
        const rms = Math.sqrt(sum / dataArray.length);
        const db = 20 * Math.log10(Math.max(rms, 0.0001));
        const now = ctx.currentTime;
        if (db < threshold) {
            gateGain.gain.setTargetAtTime(0, now, release);
        } else {
            gateGain.gain.setTargetAtTime(1, now, attack);
        }
        requestAnimationFrame(process);
    };
    process();

    return {
        type: 'gate', name: 'Noise Gate', input, output, bypass: false,
        setThreshold(v) { threshold = v; },
        setAttack(v) { attack = v / 1000; },
        setRelease(v) { release = v / 1000; },
        destroy() { active = false; },
    };
}

// ─── Stereo Widener (Mid-Side) ───
export function createStereoWidener(ctx) {
    const input = ctx.createGain();
    const output = ctx.createGain();
    const splitter = ctx.createChannelSplitter(2);
    const merger = ctx.createChannelMerger(2);
    const midGain = ctx.createGain();
    const sideGain = ctx.createGain();

    midGain.gain.value = 1;
    sideGain.gain.value = 1;

    input.connect(splitter);
    // Mid = (L + R) / 2, Side = (L - R) / 2
    splitter.connect(midGain, 0);
    splitter.connect(midGain, 1);
    splitter.connect(sideGain, 0);
    // Approximate widening by amplifying the side signal
    midGain.connect(merger, 0, 0);
    midGain.connect(merger, 0, 1);
    sideGain.connect(merger, 0, 0);
    merger.connect(output);

    return {
        type: 'stereoWidener', name: 'Stereo Widener', input, output, bypass: false,
        setWidth(v) {
            // v: 0=mono, 1=normal, 2=wide
            midGain.gain.value = 2 - v;
            sideGain.gain.value = v;
        },
    };
}

// ─── Auto Panner ───
export function createAutoPan(ctx) {
    const input = ctx.createGain();
    const output = ctx.createGain();
    const panner = ctx.createStereoPanner();
    const lfo = ctx.createOscillator();
    const lfoGain = ctx.createGain();

    lfo.frequency.value = 2;
    lfo.type = 'sine';
    lfoGain.gain.value = 0.8;

    lfo.connect(lfoGain);
    lfoGain.connect(panner.pan);
    lfo.start();

    input.connect(panner);
    panner.connect(output);

    return {
        type: 'autoPan', name: 'Auto Pan', input, output, bypass: false,
        setRate(v) { lfo.frequency.value = v; },
        setDepth(v) { lfoGain.gain.value = v; },
        setShape(v) { lfo.type = v; }, // 'sine', 'triangle', 'square'
    };
}

// ─── Bitcrusher (AudioWorklet fallback via ScriptProcessor) ───
export function createBitcrusher(ctx) {
    const input = ctx.createGain();
    const output = ctx.createGain();

    let bitDepth = 8;
    let sampleRateReduction = 1;

    // Use ScriptProcessorNode as fallback (AudioWorklet would be preferred)
    const bufferSize = 4096;
    const processor = ctx.createScriptProcessor(bufferSize, 1, 1);
    let phase = 0;
    let lastSample = 0;

    processor.onaudioprocess = (e) => {
        const inp = e.inputBuffer.getChannelData(0);
        const out = e.outputBuffer.getChannelData(0);
        const step = Math.pow(0.5, bitDepth);

        for (let i = 0; i < inp.length; i++) {
            phase += sampleRateReduction;
            if (phase >= 1) {
                phase -= 1;
                lastSample = step * Math.floor(inp[i] / step + 0.5);
            }
            out[i] = lastSample;
        }
    };

    input.connect(processor);
    processor.connect(output);

    return {
        type: 'bitcrusher', name: 'Bitcrusher', input, output, bypass: false,
        setBitDepth(v) { bitDepth = Math.max(1, Math.min(16, v)); },
        setSampleRateReduction(v) { sampleRateReduction = Math.max(0.01, Math.min(1, v)); },
    };
}

// ─── Frequency Shifter ───
export function createFrequencyShifter(ctx) {
    const input = ctx.createGain();
    const output = ctx.createGain();
    const osc = ctx.createOscillator();
    const oscGain = ctx.createGain();

    osc.frequency.value = 0; // Shift amount in Hz
    osc.type = 'sine';
    oscGain.gain.value = 0;

    // Ring modulation approximation
    osc.connect(oscGain);
    input.connect(output);
    osc.start();

    return {
        type: 'freqShifter', name: 'Frequency Shifter', input, output, bypass: false,
        setShift(v) { osc.frequency.value = v; oscGain.gain.value = v !== 0 ? 1 : 0; },
    };
}

// ─── Transient Shaper ───
export function createTransientShaper(ctx) {
    const input = ctx.createGain();
    const output = ctx.createGain();
    const attackGain = ctx.createGain();
    const sustainGain = ctx.createGain();
    const envelope = ctx.createGain();
    const analyzer = ctx.createAnalyser();

    attackGain.gain.value = 1;
    sustainGain.gain.value = 1;
    analyzer.fftSize = 256;

    input.connect(analyzer);
    input.connect(attackGain);
    attackGain.connect(output);

    let active = true;
    const dataArray = new Uint8Array(analyzer.frequencyBinCount);
    let prevRms = 0;

    const process = () => {
        if (!active) return;
        analyzer.getByteTimeDomainData(dataArray);
        let sum = 0;
        for (let i = 0; i < dataArray.length; i++) {
            const v = (dataArray[i] - 128) / 128;
            sum += v * v;
        }
        const rms = Math.sqrt(sum / dataArray.length);
        const isTransient = rms > prevRms * 1.5;
        const now = ctx.currentTime;

        if (isTransient) {
            attackGain.gain.setTargetAtTime(attackGain.gain.value, now, 0.001);
        } else {
            attackGain.gain.setTargetAtTime(sustainGain.gain.value, now, 0.05);
        }
        prevRms = rms;
        requestAnimationFrame(process);
    };
    process();

    return {
        type: 'transientShaper', name: 'Transient Shaper', input, output, bypass: false,
        setAttack(v) { attackGain.gain.value = v; }, // 0-2, 1 = neutral
        setSustain(v) { sustainGain.gain.value = v; }, // 0-2, 1 = neutral
        destroy() { active = false; },
    };
}
