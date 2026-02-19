// ============================================
// ORPHEUS DAW — MIDI Arpeggiator
// ============================================
// Transforms held notes into arpeggiated patterns

export class MidiArpeggiator {
    static PATTERNS = {
        up: 'up',
        down: 'down',
        upDown: 'upDown',
        downUp: 'downUp',
        random: 'random',
        played: 'played',
        chord: 'chord',
    };

    static RATE_VALUES = {
        '1/4': 1,      // quarter note
        '1/8': 0.5,    // eighth note
        '1/8T': 1 / 3,   // triplet eighth
        '1/16': 0.25,  // sixteenth
        '1/16T': 1 / 6,  // triplet sixteenth
        '1/32': 0.125, // thirty-second
    };

    /**
     * Generate arpeggiated note sequence
     * @param {Array<{pitch: number, velocity: number}>} inputNotes
     * @param {Object} config
     * @returns {Array<{pitch, velocity, startBeat, lengthBeats}>}
     */
    static generate(inputNotes, config = {}) {
        const {
            pattern = 'up',
            rate = '1/8',
            octaves = 1,
            gateLength = 0.8,   // 0-1, fraction of step length
            steps = 16,
            swing = 0,
            velocity = null,     // null = use input velocity
        } = config;

        if (!inputNotes || inputNotes.length === 0) return [];

        // Sort input notes by pitch
        const sorted = [...inputNotes].sort((a, b) => a.pitch - b.pitch);

        // Expand across octaves
        const expanded = [];
        for (let oct = 0; oct < octaves; oct++) {
            for (const note of sorted) {
                expanded.push({
                    pitch: note.pitch + (oct * 12),
                    velocity: velocity !== null ? velocity : note.velocity,
                });
            }
        }

        // Build pattern sequence
        const sequence = MidiArpeggiator._buildPattern(expanded, pattern, steps);

        // Convert to timed events
        const stepLength = MidiArpeggiator.RATE_VALUES[rate] || 0.5;
        const result = [];

        for (let i = 0; i < sequence.length; i++) {
            const note = sequence[i];
            const swingOffset = i % 2 === 1 ? swing * stepLength * 0.5 : 0;

            result.push({
                pitch: note.pitch,
                velocity: note.velocity,
                startBeat: i * stepLength + swingOffset,
                lengthBeats: stepLength * gateLength,
            });
        }

        return result;
    }

    static _buildPattern(notes, pattern, steps) {
        const len = notes.length;
        if (len === 0) return [];

        const sequence = [];

        switch (pattern) {
            case 'up':
                for (let i = 0; i < steps; i++) sequence.push(notes[i % len]);
                break;

            case 'down':
                for (let i = 0; i < steps; i++) sequence.push(notes[(len - 1) - (i % len)]);
                break;

            case 'upDown': {
                const cycle = len > 1 ? (len * 2 - 2) : 1;
                for (let i = 0; i < steps; i++) {
                    const pos = i % cycle;
                    sequence.push(notes[pos < len ? pos : cycle - pos]);
                }
                break;
            }

            case 'downUp': {
                const reversed = [...notes].reverse();
                const cycle = len > 1 ? (len * 2 - 2) : 1;
                for (let i = 0; i < steps; i++) {
                    const pos = i % cycle;
                    sequence.push(reversed[pos < len ? pos : cycle - pos]);
                }
                break;
            }

            case 'random':
                for (let i = 0; i < steps; i++) {
                    sequence.push(notes[Math.floor(Math.random() * len)]);
                }
                break;

            case 'played':
                for (let i = 0; i < steps; i++) sequence.push(notes[i % len]);
                break;

            case 'chord':
                for (let i = 0; i < steps; i++) {
                    // All notes play simultaneously — handled as a single step
                    sequence.push(notes[0]); // Placeholder, actual chord output differs
                }
                break;

            default:
                for (let i = 0; i < steps; i++) sequence.push(notes[i % len]);
        }

        return sequence;
    }
}
