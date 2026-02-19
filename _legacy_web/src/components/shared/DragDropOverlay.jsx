// ============================================
// ORPHEUS DAW — Drag & Drop Overlay
// ============================================

import React, { useState, useCallback, useEffect } from 'react';
import { audioBufferManager } from '../../audio/AudioBufferManager';
import { pluginManager } from '../../audio/PluginManager';
import { useProjectStore } from '../../stores/projectStore';
import { audioEngine } from '../../audio/AudioEngine';

const AUDIO_EXTENSIONS = ['wav', 'mp3', 'ogg', 'flac', 'aiff', 'aif', 'm4a', 'wma', 'webm'];
const PLUGIN_EXTENSIONS = ['dll', 'vst', 'vst3', 'component', 'so'];

export default function DragDropOverlay() {
  const [isDragging, setIsDragging] = useState(false);
  const [dragType, setDragType] = useState(null); // 'audio' | 'plugin' | 'mixed'
  const [status, setStatus] = useState(null); // { type: 'success'|'error', message }

  const detectFileTypes = (files) => {
    let hasAudio = false, hasPlugin = false;
    for (const file of files) {
      const ext = file.name.split('.').pop().toLowerCase();
      if (AUDIO_EXTENSIONS.includes(ext)) hasAudio = true;
      if (PLUGIN_EXTENSIONS.includes(ext)) hasPlugin = true;
    }
    if (hasAudio && hasPlugin) return 'mixed';
    if (hasPlugin) return 'plugin';
    return 'audio';
  };

  const handleDragEnter = useCallback((e) => {
    e.preventDefault();
    e.stopPropagation();
    setIsDragging(true);
    if (e.dataTransfer.items) {
      const files = Array.from(e.dataTransfer.items);
      // We can't read file names on dragenter in some browsers, so just show generic
      setDragType('audio');
    }
  }, []);

  const handleDragOver = useCallback((e) => {
    e.preventDefault();
    e.stopPropagation();
  }, []);

  const handleDragLeave = useCallback((e) => {
    e.preventDefault();
    e.stopPropagation();
    // Only hide if we're leaving the overlay itself
    if (e.currentTarget === e.target) {
      setIsDragging(false);
      setDragType(null);
    }
  }, []);

  const handleDrop = useCallback(async (e) => {
    e.preventDefault();
    e.stopPropagation();
    setIsDragging(false);

    const files = Array.from(e.dataTransfer.files);
    if (files.length === 0) return;

    const type = detectFileTypes(files);
    let audioCount = 0, pluginCount = 0, errors = [];

    await audioEngine.init();

    for (const file of files) {
      const ext = file.name.split('.').pop().toLowerCase();

      if (AUDIO_EXTENSIONS.includes(ext)) {
        try {
          const bufferId = await audioBufferManager.loadFile(file);
          const bufEntry = audioBufferManager.getBuffer(bufferId);
          const proj = useProjectStore.getState();
          const bpm = proj.bpm;

          // Find first audio track or create one
          let targetTrack = proj.tracks.find(t => t.type === 'audio');
          if (!targetTrack) {
            proj.addTrack('audio');
            targetTrack = useProjectStore.getState().tracks[useProjectStore.getState().tracks.length - 1];
          }

          // Calculate clip length in beats
          const lengthBeats = Math.ceil(audioBufferManager.durationToBeats(bufEntry.duration, bpm));

          // Find the next open position on the track
          let startBeat = 0;
          targetTrack.clips.forEach(c => {
            const end = c.startBeat + c.lengthBeats;
            if (end > startBeat) startBeat = end;
          });

          // Add the clip
          proj.addClip(targetTrack.id, {
            id: `clip_${Date.now()}_${Math.random().toString(36).slice(2, 6)}`,
            trackId: targetTrack.id,
            type: 'audio',
            name: file.name.replace(/\.[^.]+$/, ''),
            startBeat,
            lengthBeats,
            offset: 0,
            gain: 1,
            fadeIn: 0,
            fadeOut: 0,
            waveformData: bufEntry.waveformData,
            bufferId,
            color: null,
          });
          audioCount++;
        } catch (err) {
          errors.push(`${file.name}: ${err.message}`);
        }
      } else if (PLUGIN_EXTENSIONS.includes(ext)) {
        try {
          await pluginManager.importPlugin(file);
          pluginCount++;
        } catch (err) {
          errors.push(`${file.name}: ${err.message}`);
        }
      } else {
        errors.push(`${file.name}: Unsupported format`);
      }
    }

    // Show status
    const parts = [];
    if (audioCount > 0) parts.push(`${audioCount} audio file${audioCount > 1 ? 's' : ''} imported`);
    if (pluginCount > 0) parts.push(`${pluginCount} plugin${pluginCount > 1 ? 's' : ''} loaded`);
    if (errors.length > 0) parts.push(`${errors.length} failed`);

    setStatus({
      type: errors.length > 0 && parts.length === 1 ? 'error' : 'success',
      message: parts.join(', '),
    });

    // Auto-hide status
    setTimeout(() => setStatus(null), 3000);
  }, []);

  // Register document-level drag listeners so drops are always detected
  useEffect(() => {
    const onEnter = (e) => { e.preventDefault(); setIsDragging(true); };
    const onOver = (e) => { e.preventDefault(); };
    const onLeave = (e) => { e.preventDefault(); if (e.target === document.documentElement) setIsDragging(false); };
    document.addEventListener('dragenter', onEnter);
    document.addEventListener('dragover', onOver);
    document.addEventListener('dragleave', onLeave);
    document.addEventListener('drop', handleDrop);
    return () => {
      document.removeEventListener('dragenter', onEnter);
      document.removeEventListener('dragover', onOver);
      document.removeEventListener('dragleave', onLeave);
      document.removeEventListener('drop', handleDrop);
    };
  }, [handleDrop]);

  return (
    <>
      {/* Overlay */}
      {isDragging && (
        <div className="drag-drop-overlay" onDragOver={(e) => e.preventDefault()} onDrop={handleDrop}>
          <div className="drag-drop-zone">
            <div className="drag-drop-icon">📁</div>
            <h2 className="drag-drop-title">Drop Files Here</h2>
            <p className="drag-drop-subtitle">
              Audio: .wav, .mp3, .ogg, .flac, .aiff<br />
              Plugins: .vst, .vst3, .dll
            </p>
            <div className="drag-drop-hints">
              <span className="drag-hint">🎵 Audio files → New clips on timeline</span>
              <span className="drag-hint">🔌 Plugin files → Added to plugin library</span>
            </div>
          </div>
        </div>
      )}

      {/* Import status notification */}
      {status && (
        <div className={`import-notification ${status.type}`}>
          <span>{status.type === 'success' ? '✓' : '✗'}</span>
          <span>{status.message}</span>
        </div>
      )}
    </>
  );
}
