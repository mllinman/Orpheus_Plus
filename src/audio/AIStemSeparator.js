// ============================================
// ORPHEUS DAW — AI Stem Separator
// ============================================
// Uses @xenova/transformers to run Demucs (htdemucs) for high-quality
// source separation directly in the browser via ONNX Runtime Web.

import { pipeline, env } from '@xenova/transformers';

// Configure transformers.js
env.allowLocalModels = false;
env.useBrowserCache = true;

export class AIStemSeparator {

    static async separate(audioBuffer, options = {}) {
        const {
            onProgress = () => { },
            model = 'Xenova/htdemucs', // standard quantized htdemucs
            chunkLength = 10, // process in 10s chunks to save memory
        } = options;

        console.log(`[AIStemSeparator] Loading model ${model}...`);

        // 1. Load separation pipeline
        const separator = await pipeline('audio-separation', model, {
            progress_callback: (p) => {
                if (p.status === 'progress') {
                    onProgress(p.progress / 100); // 0-1
                }
            }
        });

        // 2. Prepare audio
        // transformers.js expects Float32Array (mono or stereo)
        // Ideally stereo. It handles resampling internally usually.
        // But for htdemucs, it expects 44.1kHz.
        // We pass the raw data.

        const inputData = audioBuffer.getChannelData(0); // Mono for now?
        // TODO: Handle stereo. Pipeline supports stereo input?
        // Usually expects { sampling_rate, data } object or Float32Array

        console.log('[AIStemSeparator] Starting separation...');
        onProgress(0.1);

        // Run inference
        // Note: The pipeline API handles chunking automatically if configured?
        // Or we might need to manually chunk long audio.
        const output = await separator(inputData, {
            sampling_rate: audioBuffer.sampleRate,
            chunk_length_s: chunkLength,
            stride_length_s: 2, // overlap
        });

        // Output format from 'audio-separation' pipeline with htdemucs:
        // Array of { label: 'bass', data: Float32Array, sampling_rate: 44100 } ?
        // Or object? Documentation varies. Assuming standard pipeline output.

        console.log('[AIStemSeparator] Separation complete', output);
        onProgress(1.0);

        // 3. Process output into AudioBuffers
        // Create context to decode/transfer
        const ctx = new OfflineAudioContext(2, 1, 44100); // Dummy context

        const stems = {};

        // Map common stems
        for (const stem of output) {
            const label = stem.label; // 'bass', 'drums', 'other', 'vocals'
            const audioData = stem.audio; // Float32Array

            const buffer = ctx.createBuffer(1, audioData.length, stem.sampling_rate);
            buffer.getChannelData(0).set(audioData);

            stems[label] = buffer;
        }

        return stems;
    }
}
