import React, { useRef, useEffect, useCallback, useState } from 'react';
import { useProjectStore } from '../../stores/projectStore';
import { useUIStore } from '../../stores/uiStore';
import { uid } from '../../utils/helpers';

// ── Volume Automation Drawer ──
// Draw and animate volume changes in real-time on a per-clip or per-track basis
export default function VolumeAutomationDrawer({ trackId, onClose }) {
  const canvasRef = useRef(null);
  const tracks = useProjectStore(s => s.tracks);
  const bpm = useProjectStore(s => s.bpm);
  const { horizontalZoom, snapValue } = useUIStore();
  const { updateTrackAutomation } = useProjectStore();

  const track = tracks.find(t => t.id === trackId);
  const [isDrawing, setIsDrawing] = useState(false);
  const [points, setPoints] = useState(track?.volumeAutomation || []);
  const [curveType, setCurveType] = useState('linear'); // linear | smooth | step
  const [previewPlaying, setPreviewPlaying] = useState(false);
  const [playheadPos, setPlayheadPos] = useState(0);
  const animFrameRef = useRef(null);

  // Calculate max beats from clips
  const maxBeat = Math.max(16, ...(track?.clips?.map(c => (c.startBeat || 0) + (c.lengthBeats || 0)) || [16]));
  const canvasWidth = maxBeat * horizontalZoom;
  const canvasHeight = 120;

  // Draw the automation lane
  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    const w = canvas.width = Math.max(460, canvasWidth);
    const h = canvas.height = canvasHeight;

    // Background
    ctx.fillStyle = '#13132a';
    ctx.fillRect(0, 0, w, h);

    // Grid
    for (let beat = 0; beat <= maxBeat; beat++) {
      const x = beat * horizontalZoom;
      const isBar = beat % 4 === 0;
      ctx.strokeStyle = isBar ? '#252540' : '#1b1b35';
      ctx.lineWidth = isBar ? 1 : 0.5;
      ctx.beginPath();
      ctx.moveTo(x, 0); ctx.lineTo(x, h);
      ctx.stroke();
    }

    // Horizontal guides at 25%, 50%, 75%, 100%
    [0.25, 0.5, 0.75, 1].forEach(level => {
      const y = h - (level * h);
      ctx.strokeStyle = '#1e1e38';
      ctx.lineWidth = 0.5;
      ctx.setLineDash([3, 3]);
      ctx.beginPath();
      ctx.moveTo(0, y); ctx.lineTo(w, y);
      ctx.stroke();
      ctx.setLineDash([]);

      ctx.fillStyle = '#444';
      ctx.font = '9px sans-serif';
      ctx.fillText(`${Math.round(level * 100)}%`, 2, y - 2);
    });

    // Draw automation line
    if (points.length >= 2) {
      const sorted = [...points].sort((a, b) => a.beat - b.beat);

      // Fill area under curve
      ctx.beginPath();
      ctx.moveTo(0, h);
      sorted.forEach((p, i) => {
        const x = p.beat * horizontalZoom;
        const y = h - (p.value * h);
        if (i === 0) {
          ctx.lineTo(0, y);
        }
        if (curveType === 'smooth' && i > 0) {
          const prev = sorted[i - 1];
          const cpx = (prev.beat * horizontalZoom + x) / 2;
          ctx.bezierCurveTo(cpx, h - (prev.value * h), cpx, y, x, y);
        } else if (curveType === 'step' && i > 0) {
          const prevX = sorted[i - 1].beat * horizontalZoom;
          ctx.lineTo(x, h - (sorted[i - 1].value * h));
          ctx.lineTo(x, y);
        } else {
          ctx.lineTo(x, y);
        }
      });
      const lastX = sorted[sorted.length - 1].beat * horizontalZoom;
      ctx.lineTo(lastX, h);
      ctx.closePath();
      ctx.fillStyle = 'rgba(99, 102, 241, 0.1)';
      ctx.fill();

      // Line
      ctx.beginPath();
      sorted.forEach((p, i) => {
        const x = p.beat * horizontalZoom;
        const y = h - (p.value * h);
        if (i === 0) { ctx.moveTo(x, y); return; }

        if (curveType === 'smooth') {
          const prev = sorted[i - 1];
          const cpx = (prev.beat * horizontalZoom + x) / 2;
          ctx.bezierCurveTo(cpx, h - (prev.value * h), cpx, y, x, y);
        } else if (curveType === 'step') {
          ctx.lineTo(x, h - (sorted[i - 1].value * h));
          ctx.lineTo(x, y);
        } else {
          ctx.lineTo(x, y);
        }
      });
      ctx.strokeStyle = '#6366f1';
      ctx.lineWidth = 2;
      ctx.stroke();

      // Points
      sorted.forEach(p => {
        const x = p.beat * horizontalZoom;
        const y = h - (p.value * h);
        ctx.beginPath();
        ctx.arc(x, y, 4, 0, Math.PI * 2);
        ctx.fillStyle = '#a5b4fc';
        ctx.fill();
        ctx.strokeStyle = '#6366f1';
        ctx.lineWidth = 1.5;
        ctx.stroke();
      });
    } else if (points.length === 1) {
      const p = points[0];
      const y = h - (p.value * h);
      ctx.beginPath();
      ctx.arc(p.beat * horizontalZoom, y, 4, 0, Math.PI * 2);
      ctx.fillStyle = '#a5b4fc';
      ctx.fill();
    }

    // Playhead
    if (previewPlaying) {
      const px = playheadPos * horizontalZoom;
      ctx.strokeStyle = '#22d3ee';
      ctx.lineWidth = 2;
      ctx.beginPath();
      ctx.moveTo(px, 0); ctx.lineTo(px, h);
      ctx.stroke();
    }

  }, [points, curveType, horizontalZoom, maxBeat, previewPlaying, playheadPos]);

  // Mouse handlers for drawing
  const getPointFromEvent = useCallback((e) => {
    const rect = canvasRef.current.getBoundingClientRect();
    const x = e.clientX - rect.left;
    const y = e.clientY - rect.top;
    let beat = x / horizontalZoom;
    if (snapValue > 0) beat = Math.round(beat / snapValue) * snapValue;
    beat = Math.max(0, Math.min(maxBeat, beat));
    const value = Math.max(0, Math.min(1, 1 - y / canvasHeight));
    return { beat: Math.round(beat * 100) / 100, value: Math.round(value * 100) / 100 };
  }, [horizontalZoom, snapValue, maxBeat]);

  const handleMouseDown = useCallback((e) => {
    setIsDrawing(true);
    const pt = getPointFromEvent(e);
    // Check if clicking near existing point
    const existing = points.findIndex(p =>
      Math.abs(p.beat - pt.beat) < 0.5 && Math.abs(p.value - pt.value) < 0.1
    );
    if (e.button === 2 && existing >= 0) {
      // Right-click: delete point
      setPoints(prev => prev.filter((_, i) => i !== existing));
    } else {
      setPoints(prev => [...prev.filter(p => Math.abs(p.beat - pt.beat) > 0.1), pt]);
    }
  }, [points, getPointFromEvent]);

  const handleMouseMove = useCallback((e) => {
    if (!isDrawing) return;
    const pt = getPointFromEvent(e);
    setPoints(prev => [...prev.filter(p => Math.abs(p.beat - pt.beat) > 0.1), pt]);
  }, [isDrawing, getPointFromEvent]);

  const handleMouseUp = useCallback(() => {
    setIsDrawing(false);
  }, []);

  // Preview animation
  const togglePreview = useCallback(() => {
    if (previewPlaying) {
      setPreviewPlaying(false);
      cancelAnimationFrame(animFrameRef.current);
    } else {
      setPreviewPlaying(true);
      setPlayheadPos(0);
      const start = performance.now();
      const duration = (maxBeat / bpm) * 60 * 1000; // ms

      const animate = (now) => {
        const elapsed = now - start;
        const progress = elapsed / duration;
        if (progress >= 1) {
          setPreviewPlaying(false);
          setPlayheadPos(0);
          return;
        }
        setPlayheadPos(progress * maxBeat);
        animFrameRef.current = requestAnimationFrame(animate);
      };
      animFrameRef.current = requestAnimationFrame(animate);
    }
  }, [previewPlaying, maxBeat, bpm]);

  const handleApply = useCallback(() => {
    if (!trackId) return;
    const sorted = [...points].sort((a, b) => a.beat - b.beat);
    updateTrackAutomation(trackId, 'volume', sorted);
  }, [trackId, points, updateTrackAutomation]);

  const handleClear = () => setPoints([]);

  const addPreset = (preset) => {
    switch (preset) {
      case 'fadeIn':
        setPoints([{ beat: 0, value: 0 }, { beat: maxBeat * 0.25, value: 1 }, { beat: maxBeat, value: 1 }]);
        break;
      case 'fadeOut':
        setPoints([{ beat: 0, value: 1 }, { beat: maxBeat * 0.75, value: 1 }, { beat: maxBeat, value: 0 }]);
        break;
      case 'inOut':
        setPoints([{ beat: 0, value: 0 }, { beat: maxBeat * 0.15, value: 1 }, { beat: maxBeat * 0.85, value: 1 }, { beat: maxBeat, value: 0 }]);
        break;
      case 'swell':
        setPoints([{ beat: 0, value: 0.3 }, { beat: maxBeat * 0.5, value: 1 }, { beat: maxBeat, value: 0.3 }]);
        break;
      case 'ducking':
        const pts = [];
        for (let b = 0; b < maxBeat; b += 4) {
          pts.push({ beat: b, value: 1 });
          pts.push({ beat: b + 0.5, value: 0.3 });
          pts.push({ beat: b + 1, value: 1 });
        }
        setPoints(pts);
        break;
    }
  };

  if (!track) return null;

  return (
    <div className="volume-automation-drawer">
      <div className="panel-header">
        <span>📊 Volume Automation — {track.name}</span>
        {onClose && <button className="btn btn-xs btn-ghost" onClick={onClose}>✕</button>}
      </div>

      <div className="vad-body">
        {/* Curve type */}
        <div className="vad-toolbar">
          <div className="vad-curve-btns">
            {[
              { id: 'linear', label: '📐 Linear' },
              { id: 'smooth', label: '〰 Smooth' },
              { id: 'step', label: '⊟ Step' },
            ].map(ct => (
              <button
                key={ct.id}
                className={`btn btn-xs ${curveType === ct.id ? 'btn-primary' : 'btn-ghost'}`}
                onClick={() => setCurveType(ct.id)}
              >
                {ct.label}
              </button>
            ))}
          </div>

          <div className="vad-presets">
            <span className="vad-preset-label">Presets:</span>
            {[
              { id: 'fadeIn', label: '📈 Fade In' },
              { id: 'fadeOut', label: '📉 Fade Out' },
              { id: 'inOut', label: '🔄 In/Out' },
              { id: 'swell', label: '🌊 Swell' },
              { id: 'ducking', label: '🦆 Ducking' },
            ].map(p => (
              <button key={p.id} className="btn btn-xs btn-ghost" onClick={() => addPreset(p.id)}>
                {p.label}
              </button>
            ))}
          </div>
        </div>

        {/* Canvas */}
        <div className="vad-canvas-container">
          <canvas
            ref={canvasRef}
            className="vad-canvas"
            onMouseDown={handleMouseDown}
            onMouseMove={handleMouseMove}
            onMouseUp={handleMouseUp}
            onMouseLeave={handleMouseUp}
            onContextMenu={e => e.preventDefault()}
          />
          <div className="vad-hint">Click to add points • Drag to draw • Right-click to delete</div>
        </div>

        {/* Actions */}
        <div className="vad-actions">
          <button className="btn btn-primary btn-sm" onClick={handleApply}>
            ✓ Apply Automation
          </button>
          <button className="btn btn-sm btn-ghost" onClick={togglePreview}>
            {previewPlaying ? '⏹ Stop' : '▶ Preview'}
          </button>
          <button className="btn btn-sm btn-ghost" onClick={handleClear}>
            🗑 Clear
          </button>
          <span className="vad-point-count">{points.length} points</span>
        </div>
      </div>
    </div>
  );
}
