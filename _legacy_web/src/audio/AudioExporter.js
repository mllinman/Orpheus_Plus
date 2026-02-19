// ============================================
// ORPHEUS DAW — Audio Exporter (HD Export)
// ============================================
// Supports HD WAV export with custom save location via File System Access API

import { audioEngine } from './AudioEngine';
import { audioBufferManager } from './AudioBufferManager';

class AudioExporter {
    constructor() {
        this.isExporting = false;
        this.progress = 0;
        this.listeners = new Set();
    }

    /**
     * Check if File System Access API is available
     */
    get hasFilePicker() {
        return typeof window.showSaveFilePicker === 'function';
    }

    /**
     * Export the project as HD WAV
     * @param {Object} options
     * @param {number} options.sampleRate - 44100, 48000, 96000, or 192000
     * @param {number} options.bitDepth - 16, 24, or 32
     * @param {number} options.channels - 1 or 2
     * @param {number} options.duration - Duration in seconds
     * @param {boolean} options.normalize - Whether to normalize the output
     * @param {boolean} options.dither - Whether to apply dithering (for 16-bit)
     * @param {boolean} options.chooseLocation - Whether to use File System Access API
     * @param {string} options.fileName - Suggested file name
     * @param {Function} options.onProgress - Progress callback (0-1)
     */
    async exportWAV(options = {}) {
        const {
            sampleRate = 44100,
            bitDepth = 24,
            channels = 2,
            duration = 30,
            normalize = false,
            dither = false,
            chooseLocation = false,
            fileName = 'Orpheus_Export',
            onProgress = null,
        } = options;

        this.isExporting = true;
        this.progress = 0;
        this._notify();

        try {
            // Create offline context for rendering
            const offlineCtx = new OfflineAudioContext(channels, sampleRate * duration, sampleRate);

            // Create master gain
            const masterGain = offlineCtx.createGain();
            masterGain.gain.value = 0.8;
            masterGain.connect(offlineCtx.destination);

            // Schedule all audio buffers from loaded clips
            const { tracks, bpm } = await this._getProjectState();
            let hasContent = false;

            for (const track of tracks) {
                if (track.mute) continue;
                const trackGain = offlineCtx.createGain();
                trackGain.gain.value = track.volume;

                // Pan
                const panner = offlineCtx.createStereoPanner();
                panner.pan.value = track.pan;

                trackGain.connect(panner);
                panner.connect(masterGain);

                for (const clip of track.clips) {
                    if (clip.bufferId) {
                        const entry = audioBufferManager.getBuffer(clip.bufferId);
                        if (entry) {
                            const source = offlineCtx.createBufferSource();
                            source.buffer = entry.buffer;
                            const clipGain = offlineCtx.createGain();
                            clipGain.gain.value = clip.gain || 1;
                            source.connect(clipGain);
                            clipGain.connect(trackGain);

                            const startTime = (clip.startBeat / bpm) * 60;
                            const offset = clip.offset || 0;
                            source.start(startTime, offset);
                            hasContent = true;
                        }
                    }
                }
            }

            // If no real audio content, generate a placeholder tone so the file isn't empty
            if (!hasContent) {
                const osc = offlineCtx.createOscillator();
                const oscGain = offlineCtx.createGain();
                oscGain.gain.value = 0.3;
                osc.frequency.value = 440;
                osc.connect(oscGain);
                oscGain.connect(masterGain);
                osc.start(0);
                osc.stop(Math.min(2, duration));

                // Add a second tone for richness
                const osc2 = offlineCtx.createOscillator();
                osc2.frequency.value = 554.37; // C#5
                osc2.type = 'sine';
                const osc2Gain = offlineCtx.createGain();
                osc2Gain.gain.value = 0.15;
                osc2.connect(osc2Gain);
                osc2Gain.connect(masterGain);
                osc2.start(0.5);
                osc2.stop(Math.min(3, duration));
            }

            // Progress simulation during rendering
            const progressInterval = setInterval(() => {
                if (this.progress < 0.9) {
                    this.progress += 0.05;
                    if (onProgress) onProgress(this.progress);
                    this._notify();
                }
            }, 200);

            // Render
            const renderedBuffer = await offlineCtx.startRendering();
            clearInterval(progressInterval);

            this.progress = 0.95;
            if (onProgress) onProgress(0.95);
            this._notify();

            // Normalize if requested
            let audioData = [];
            for (let ch = 0; ch < channels; ch++) {
                audioData.push(new Float32Array(renderedBuffer.getChannelData(ch < renderedBuffer.numberOfChannels ? ch : 0)));
            }

            if (normalize) {
                let peak = 0;
                for (const chData of audioData) {
                    for (let i = 0; i < chData.length; i++) {
                        const abs = Math.abs(chData[i]);
                        if (abs > peak) peak = abs;
                    }
                }
                if (peak > 0 && peak !== 1) {
                    const scale = 1.0 / peak;
                    for (const chData of audioData) {
                        for (let i = 0; i < chData.length; i++) {
                            chData[i] *= scale;
                        }
                    }
                }
            }

            // Apply dithering for 16-bit
            if (dither && bitDepth === 16) {
                for (const chData of audioData) {
                    for (let i = 0; i < chData.length; i++) {
                        chData[i] += (Math.random() - 0.5) / 32768;
                    }
                }
            }

            // Encode to WAV
            const wavBuffer = this._encodeWAV(audioData, sampleRate, bitDepth, channels);

            this.progress = 1.0;
            if (onProgress) onProgress(1.0);
            this._notify();

            // Save with File System Access API or fallback to download
            const fullFileName = `${fileName}.wav`;

            if (chooseLocation && this.hasFilePicker) {
                try {
                    const handle = await window.showSaveFilePicker({
                        suggestedName: fullFileName,
                        types: [{
                            description: 'WAV Audio File',
                            accept: { 'audio/wav': ['.wav'] },
                        }],
                    });
                    const writable = await handle.createWritable();
                    await writable.write(wavBuffer);
                    await writable.close();
                } catch (err) {
                    // User cancelled the picker — fall back to download
                    if (err.name !== 'AbortError') {
                        this._downloadBlob(wavBuffer, fullFileName);
                    }
                }
            } else {
                this._downloadBlob(wavBuffer, fullFileName);
            }

            this.isExporting = false;
            this._notify();
            return true;
        } catch (err) {
            this.isExporting = false;
            this.progress = 0;
            this._notify();
            throw err;
        }
    }

    _downloadBlob(buffer, fileName) {
        const blob = new Blob([buffer], { type: 'audio/wav' });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = fileName;
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        URL.revokeObjectURL(url);
    }

    /**
     * Encode an AudioBuffer directly to a WAV Blob
     * @param {AudioBuffer} audioBuffer
     * @returns {Blob}
     */
    encodeBufferToBlob(audioBuffer) {
        const channels = audioBuffer.numberOfChannels;
        const sampleRate = audioBuffer.sampleRate;
        const channelData = [];
        for (let i = 0; i < channels; i++) {
            channelData.push(audioBuffer.getChannelData(i));
        }
        // Encode as 16-bit WAV for compatibility
        const wavBuffer = this._encodeWAV(channelData, sampleRate, 16, channels);
        return new Blob([wavBuffer], { type: 'audio/wav' });
    }

    async _getProjectState() {
        // Import dynamically to avoid circular deps
        const { useProjectStore } = await import('../stores/projectStore');
        return useProjectStore.getState();
    }

    /**
     * Encode PCM data to WAV format
     */
    _encodeWAV(channelData, sampleRate, bitDepth, numChannels) {
        const bytesPerSample = bitDepth / 8;
        const numSamples = channelData[0].length;
        const dataSize = numSamples * numChannels * bytesPerSample;
        const buffer = new ArrayBuffer(44 + dataSize);
        const view = new DataView(buffer);

        // RIFF header
        this._writeString(view, 0, 'RIFF');
        view.setUint32(4, 36 + dataSize, true);
        this._writeString(view, 8, 'WAVE');

        // fmt chunk
        this._writeString(view, 12, 'fmt ');
        view.setUint32(16, 16, true); // chunk size
        view.setUint16(20, bitDepth === 32 ? 3 : 1, true); // format: 3 = IEEE float, 1 = PCM
        view.setUint16(22, numChannels, true);
        view.setUint32(24, sampleRate, true);
        view.setUint32(28, sampleRate * numChannels * bytesPerSample, true);
        view.setUint16(32, numChannels * bytesPerSample, true);
        view.setUint16(34, bitDepth, true);

        // data chunk
        this._writeString(view, 36, 'data');
        view.setUint32(40, dataSize, true);

        let offset = 44;
        for (let i = 0; i < numSamples; i++) {
            for (let ch = 0; ch < numChannels; ch++) {
                const sample = Math.max(-1, Math.min(1, channelData[ch][i]));

                switch (bitDepth) {
                    case 16: {
                        const val = Math.max(-32768, Math.min(32767, Math.round(sample * 32767)));
                        view.setInt16(offset, val, true);
                        break;
                    }
                    case 24: {
                        const val = Math.max(-8388608, Math.min(8388607, Math.round(sample * 8388607)));
                        view.setUint8(offset, val & 0xFF);
                        view.setUint8(offset + 1, (val >> 8) & 0xFF);
                        view.setUint8(offset + 2, (val >> 16) & 0xFF);
                        break;
                    }
                    case 32: {
                        view.setFloat32(offset, sample, true);
                        break;
                    }
                }
                offset += bytesPerSample;
            }
        }

        return buffer;
    }

    _writeString(view, offset, str) {
        for (let i = 0; i < str.length; i++) {
            view.setUint8(offset + i, str.charCodeAt(i));
        }
    }

    subscribe(fn) {
        this.listeners.add(fn);
        return () => this.listeners.delete(fn);
    }

    _notify() {
        for (const fn of this.listeners) fn({
            isExporting: this.isExporting,
            progress: this.progress,
        });
    }
}

export const audioExporter = new AudioExporter();
