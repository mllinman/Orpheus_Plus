// ============================================
// ORPHEUS DAW — Utility Helpers
// ============================================

// Time ↔ Bars/Beats conversion
export function timeToBarsBeatsTicks(timeInSeconds, bpm, timeSignature = [4, 4]) {
    const totalBeats = (timeInSeconds / 60) * bpm;
    const beatsPerBar = timeSignature[0];
    const bars = Math.floor(totalBeats / beatsPerBar) + 1;
    const beats = Math.floor(totalBeats % beatsPerBar) + 1;
    const ticks = Math.floor(((totalBeats % 1) * 960)); // 960 ticks per beat
    return { bars, beats, ticks };
}

export function formatBarsBeats(timeInSeconds, bpm, timeSignature) {
    const { bars, beats, ticks } = timeToBarsBeatsTicks(timeInSeconds, bpm, timeSignature);
    return `${bars}.${beats}.${String(ticks).padStart(3, '0')}`;
}

export function formatTime(seconds) {
    const h = Math.floor(seconds / 3600);
    const m = Math.floor((seconds % 3600) / 60);
    const s = Math.floor(seconds % 60);
    const ms = Math.floor((seconds % 1) * 1000);
    return `${String(h).padStart(2, '0')}:${String(m).padStart(2, '0')}:${String(s).padStart(2, '0')}.${String(ms).padStart(3, '0')}`;
}

// dB ↔ linear
export function dbToLinear(db) {
    return Math.pow(10, db / 20);
}

export function linearToDb(linear) {
    if (linear <= 0) return -Infinity;
    return 20 * Math.log10(linear);
}

// MIDI ↔ frequency
export function midiToFrequency(note) {
    return 440 * Math.pow(2, (note - 69) / 12);
}

export function frequencyToMidi(freq) {
    return 69 + 12 * Math.log2(freq / 440);
}

// MIDI note names
const NOTE_NAMES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];

export function midiToNoteName(note) {
    const octave = Math.floor(note / 12) - 1;
    const name = NOTE_NAMES[note % 12];
    return `${name}${octave}`;
}

export function noteNameToMidi(name) {
    const match = name.match(/^([A-G]#?)(-?\d+)$/);
    if (!match) return -1;
    const noteIndex = NOTE_NAMES.indexOf(match[1]);
    const octave = parseInt(match[2]);
    return (octave + 1) * 12 + noteIndex;
}

// Unique ID generator
let idCounter = 0;
export function uid() {
    return `${Date.now().toString(36)}_${(idCounter++).toString(36)}`;
}

// Clamp
export function clamp(value, min, max) {
    return Math.max(min, Math.min(max, value));
}

// Map range
export function mapRange(value, inMin, inMax, outMin, outMax) {
    return outMin + ((value - inMin) / (inMax - inMin)) * (outMax - outMin);
}

// Generate waveform data for display
export function generateWaveformData(length = 1000, type = 'random') {
    const data = new Float32Array(length);
    switch (type) {
        case 'random':
            for (let i = 0; i < length; i++) {
                // Simulate audio waveform with volume envelope
                const env = Math.sin((i / length) * Math.PI) * 0.8 + 0.2;
                data[i] = (Math.random() * 2 - 1) * env *
                    (Math.sin(i * 0.1) * 0.3 + Math.sin(i * 0.03) * 0.3 + Math.random() * 0.4);
            }
            break;
        case 'sine':
            for (let i = 0; i < length; i++) {
                data[i] = Math.sin((i / length) * Math.PI * 20) * 0.8;
            }
            break;
        case 'drum':
            for (let i = 0; i < length; i++) {
                const t = i / length;
                const transient = Math.exp(-t * 20) * Math.sin(t * 200);
                const body = Math.sin(t * 40) * Math.exp(-t * 5) * 0.6;
                data[i] = (transient + body + (Math.random() - 0.5) * 0.1 * Math.exp(-t * 10));
            }
            break;
        case 'vocal':
            for (let i = 0; i < length; i++) {
                const t = i / length;
                const env = Math.sin(t * Math.PI);
                data[i] = env * (Math.sin(t * 80) * 0.4 + Math.sin(t * 120) * 0.3 + (Math.random() - 0.5) * 0.3);
            }
            break;
    }
    return data;
}

// Track colors palette
export const TRACK_COLORS = [
    '#4a90d9', '#9b59b6', '#27ae60', '#e67e22', '#e74c3c',
    '#f1c40f', '#00bcd4', '#e91e8a', '#009688', '#5c6bc0',
    '#ff7043', '#66bb6a', '#ab47bc', '#29b6f6', '#ffca28'
];

export function getTrackColor(index) {
    return TRACK_COLORS[index % TRACK_COLORS.length];
}

// Snap values
export const SNAP_VALUES = [
    { label: 'Off', value: 0 },
    { label: '1 Bar', value: 4 },
    { label: '1/2', value: 2 },
    { label: '1/4', value: 1 },
    { label: '1/8', value: 0.5 },
    { label: '1/16', value: 0.25 },
    { label: '1/32', value: 0.125 },
    { label: '1/4T', value: 2 / 3 },
    { label: '1/8T', value: 1 / 3 },
    { label: '1/16T', value: 1 / 6 },
];

export function snapValue(beat, snapSize) {
    if (snapSize <= 0) return beat;
    return Math.round(beat / snapSize) * snapSize;
}
