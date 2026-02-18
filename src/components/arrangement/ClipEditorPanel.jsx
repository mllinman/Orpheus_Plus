import React, { useState, useCallback, useRef, useEffect } from 'react';
import { useProjectStore } from '../../stores/projectStore';
import { useUIStore } from '../../stores/uiStore';
import { uid } from '../../utils/helpers';

// ── Video-style Clip Editor ──
// Cut, fade in/out, split, and visually edit audio clips
export default function ClipEditorPanel({ trackId, clipId, onClose }) {
  const { updateClip, splitClip, removeClip } = useProjectStore();
  const tracks = useProjectStore(s => s.tracks);
  const bpm = useProjectStore(s => s.bpm);

  const track = tracks.find(t => t.id === trackId);
  const clip = track?.clips?.find(c => c.id === clipId);

  const [fadeIn, setFadeIn] = useState(clip?.fadeIn || 0);
  const [fadeOut, setFadeOut] = useState(clip?.fadeOut || 0);
  const [splitBeat, setSplitBeat] = useState(0);
  const [clipGain, setClipGain] = useState(clip?.gain ?? 1);

  const canvasRef = useRef(null);

  // Calculate time from beats
  const beatsToTime = (beats) => (beats * 60) / bpm;
  const clipDuration = clip ? beatsToTime(clip.lengthBeats || 1) : 0;

  // Draw clip waveform with fade visualization
  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas || !clip) return;
    const ctx = canvas.getContext('2d');
    const w = canvas.width = 460;
    const h = canvas.height = 100;

    // Background
    ctx.fillStyle = '#1a1a2e';
    ctx.fillRect(0, 0, w, h);

    // Grid lines (per beat)
    const pxPerBeat = w / (clip.lengthBeats || 1);
    ctx.strokeStyle = '#252540';
    ctx.lineWidth = 0.5;
    for (let b = 0; b <= (clip.lengthBeats || 1); b++) {
      const x = b * pxPerBeat;
      ctx.beginPath();
      ctx.moveTo(x, 0);
      ctx.lineTo(x, h);
      ctx.stroke();
    }

    // Waveform placeholder
    const mid = h / 2;
    ctx.strokeStyle = track?.color || '#6366f1';
    ctx.lineWidth = 1;
    ctx.beginPath();
    for (let x = 0; x < w; x++) {
      const t = x / w;
      const wave = Math.sin(t * 40) * 0.5 + Math.sin(t * 80) * 0.3 + Math.sin(t * 15) * 0.2;
      let amplitude = wave * (h * 0.35) * clipGain;

      // Apply fade-in envelope
      const fadeInPx = (fadeIn / (clip.lengthBeats || 1)) * w;
      if (x < fadeInPx && fadeInPx > 0) {
        amplitude *= x / fadeInPx;
      }

      // Apply fade-out envelope
      const fadeOutPx = (fadeOut / (clip.lengthBeats || 1)) * w;
      if (x > w - fadeOutPx && fadeOutPx > 0) {
        amplitude *= (w - x) / fadeOutPx;
      }

      ctx.lineTo(x, mid + amplitude);
    }
    ctx.stroke();

    // Mirror waveform
    ctx.beginPath();
    for (let x = 0; x < w; x++) {
      const t = x / w;
      const wave = Math.sin(t * 40) * 0.5 + Math.sin(t * 80) * 0.3 + Math.sin(t * 15) * 0.2;
      let amplitude = wave * (h * 0.35) * clipGain;

      const fadeInPx = (fadeIn / (clip.lengthBeats || 1)) * w;
      if (x < fadeInPx && fadeInPx > 0) amplitude *= x / fadeInPx;
      const fadeOutPx = (fadeOut / (clip.lengthBeats || 1)) * w;
      if (x > w - fadeOutPx && fadeOutPx > 0) amplitude *= (w - x) / fadeOutPx;

      ctx.lineTo(x, mid - amplitude);
    }
    ctx.stroke();

    // Fade overlay regions
    if (fadeIn > 0) {
      const fadeInPx = (fadeIn / (clip.lengthBeats || 1)) * w;
      const grad = ctx.createLinearGradient(0, 0, fadeInPx, 0);
      grad.addColorStop(0, 'rgba(99, 102, 241, 0.3)');
      grad.addColorStop(1, 'rgba(99, 102, 241, 0)');
      ctx.fillStyle = grad;
      ctx.fillRect(0, 0, fadeInPx, h);

      // Fade line
      ctx.strokeStyle = '#a5b4fc';
      ctx.lineWidth = 2;
      ctx.beginPath();
      ctx.moveTo(0, h);
      ctx.lineTo(fadeInPx, 0);
      ctx.stroke();
    }

    if (fadeOut > 0) {
      const fadeOutPx = (fadeOut / (clip.lengthBeats || 1)) * w;
      const startX = w - fadeOutPx;
      const grad = ctx.createLinearGradient(startX, 0, w, 0);
      grad.addColorStop(0, 'rgba(99, 102, 241, 0)');
      grad.addColorStop(1, 'rgba(99, 102, 241, 0.3)');
      ctx.fillStyle = grad;
      ctx.fillRect(startX, 0, fadeOutPx, h);

      ctx.strokeStyle = '#a5b4fc';
      ctx.lineWidth = 2;
      ctx.beginPath();
      ctx.moveTo(startX, 0);
      ctx.lineTo(w, h);
      ctx.stroke();
    }

    // Split marker
    if (splitBeat > 0 && splitBeat < (clip.lengthBeats || 1)) {
      const splitX = splitBeat * pxPerBeat;
      ctx.strokeStyle = '#ef4444';
      ctx.setLineDash([4, 4]);
      ctx.lineWidth = 2;
      ctx.beginPath();
      ctx.moveTo(splitX, 0);
      ctx.lineTo(splitX, h);
      ctx.stroke();
      ctx.setLineDash([]);

      ctx.fillStyle = '#ef4444';
      ctx.font = '10px sans-serif';
      ctx.fillText('✂', splitX - 5, 12);
    }

  }, [clip, fadeIn, fadeOut, splitBeat, clipGain, track]);

  const handleApplyFades = useCallback(() => {
    if (!trackId || !clipId) return;
    updateClip(trackId, clipId, { fadeIn, fadeOut, gain: clipGain });
  }, [trackId, clipId, fadeIn, fadeOut, clipGain, updateClip]);

  const handleSplit = useCallback(() => {
    if (!trackId || !clipId || splitBeat <= 0) return;
    splitClip(trackId, clipId, splitBeat);
  }, [trackId, clipId, splitBeat, splitClip]);

  const handleDelete = useCallback(() => {
    if (!trackId || !clipId) return;
    removeClip(trackId, clipId);
    onClose?.();
  }, [trackId, clipId, removeClip, onClose]);

  if (!clip) {
    return (
      <div className="clip-editor-panel">
        <div className="panel-header">
          <span>✂ Clip Editor</span>
          {onClose && <button className="btn btn-xs btn-ghost" onClick={onClose}>✕</button>}
        </div>
        <div className="cep-empty">Select a clip to edit</div>
      </div>
    );
  }

  return (
    <div className="clip-editor-panel">
      <div className="panel-header">
        <span>✂ Clip Editor — {clip.name || 'Clip'}</span>
        {onClose && <button className="btn btn-xs btn-ghost" onClick={onClose}>✕</button>}
      </div>

      <div className="cep-body">
        {/* Waveform Preview */}
        <div className="cep-preview">
          <canvas ref={canvasRef} className="cep-canvas" />
          <div className="cep-preview-info">
            <span>{(clip.lengthBeats || 0).toFixed(1)} beats</span>
            <span>{clipDuration.toFixed(2)}s</span>
          </div>
        </div>

        {/* Fade Controls */}
        <div className="cep-section">
          <div className="cep-section-title">Fades</div>
          <div className="cep-fade-row">
            <div className="cep-fade-control">
              <label>📈 Fade In</label>
              <input
                type="range" min={0} max={clip.lengthBeats / 2} step={0.25}
                value={fadeIn}
                onChange={e => setFadeIn(parseFloat(e.target.value))}
                className="cep-slider"
              />
              <span className="cep-val">{fadeIn.toFixed(1)} beats</span>
            </div>
            <div className="cep-fade-control">
              <label>📉 Fade Out</label>
              <input
                type="range" min={0} max={clip.lengthBeats / 2} step={0.25}
                value={fadeOut}
                onChange={e => setFadeOut(parseFloat(e.target.value))}
                className="cep-slider"
              />
              <span className="cep-val">{fadeOut.toFixed(1)} beats</span>
            </div>
          </div>
        </div>

        {/* Gain Control */}
        <div className="cep-section">
          <div className="cep-section-title">Volume / Gain</div>
          <div className="cep-gain-row">
            <input
              type="range" min={0} max={2} step={0.01}
              value={clipGain}
              onChange={e => setClipGain(parseFloat(e.target.value))}
              className="cep-slider"
            />
            <span className="cep-val">{(clipGain * 100).toFixed(0)}%</span>
            <span className="cep-db">
              {clipGain > 0 ? `${(20 * Math.log10(clipGain)).toFixed(1)} dB` : '-∞ dB'}
            </span>
          </div>
        </div>

        {/* Split Control */}
        <div className="cep-section">
          <div className="cep-section-title">✂ Cut / Split</div>
          <div className="cep-split-row">
            <label>Split at beat:</label>
            <input
              type="number"
              min={0.25}
              max={(clip.lengthBeats || 1) - 0.25}
              step={0.25}
              value={splitBeat}
              onChange={e => setSplitBeat(parseFloat(e.target.value) || 0)}
              className="input input-sm"
              style={{ width: 70 }}
            />
            <button className="btn btn-sm btn-ghost" onClick={handleSplit}>
              ✂ Split Here
            </button>
          </div>
        </div>

        {/* Extensions */}
        {clip.type === 'audio' && (
          <div className="cep-section">
            <div className="cep-section-title">✨ AI Extensions</div>
            <div className="cep-split-row">
              <button 
                className="btn btn-sm btn-ghost"
                onClick={() => useProjectStore.getState().extendTrack(trackId, clipId, Math.ceil(clip.lengthBeats / 4) * 2)}
                title="Smartly extend clip to 2x duration using pattern matching"
              >
                ↻ Extend 2x
              </button>
              <button 
                className="btn btn-sm btn-ghost"
                onClick={() => useProjectStore.getState().extendTrack(trackId, clipId, Math.ceil(clip.lengthBeats / 4) * 4)}
                title="Smartly extend clip to 4x duration using pattern matching"
              >
                ↻ Extend 4x
              </button>
            </div>
          </div>
        )}

        {/* Actions */}
        <div className="cep-actions">
          <button className="btn btn-primary btn-sm" onClick={handleApplyFades}>
            ✓ Apply Changes
          </button>
          <button className="btn btn-sm btn-ghost" onClick={() => { setFadeIn(0); setFadeOut(0); setClipGain(1); }}>
            ↺ Reset
          </button>
          <button className="btn btn-sm" onClick={handleDelete} style={{ color: '#ef4444' }}>
            🗑 Delete Clip
          </button>
        </div>
      </div>
    </div>
  );
}
