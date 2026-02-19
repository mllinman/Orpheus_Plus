// ============================================
// ORPHEUS DAW — Automation Lane Component
// ============================================

import React, { useRef, useEffect, useState, useCallback } from 'react';
import { useProjectStore } from '../../stores/projectStore';
import { useUIStore } from '../../stores/uiStore';

const PARAM_OPTIONS = [
  { value: 'volume', label: 'Volume', min: 0, max: 1, default: 0.75, color: '#6c5ce7' },
  { value: 'pan', label: 'Pan', min: -1, max: 1, default: 0, color: '#00b894' },
  { value: 'mute', label: 'Mute', min: 0, max: 1, default: 0, color: '#e17055' },
  { value: 'eq_low', label: 'EQ Low', min: -12, max: 12, default: 0, color: '#fdcb6e' },
  { value: 'eq_mid', label: 'EQ Mid', min: -12, max: 12, default: 0, color: '#74b9ff' },
  { value: 'eq_high', label: 'EQ High', min: -12, max: 12, default: 0, color: '#a29bfe' },
  { value: 'send_1', label: 'Send 1', min: 0, max: 1, default: 0, color: '#55efc4' },
];

export default function AutomationLane({ trackId, laneIndex = 0 }) {
  const canvasRef = useRef(null);
  const containerRef = useRef(null);
  const [selectedParam, setSelectedParam] = useState('volume');
  const [points, setPoints] = useState([]);
  const [isDragging, setIsDragging] = useState(false);
  const [dragPointIdx, setDragPointIdx] = useState(-1);
  const horizontalZoom = useUIStore(s => s.horizontalZoom);
  const track = useProjectStore(s => s.tracks.find(t => t.id === trackId));

  const paramInfo = PARAM_OPTIONS.find(p => p.value === selectedParam) || PARAM_OPTIONS[0];

  // Initialize with track's existing automation or defaults
  useEffect(() => {
    if (track?.automationLanes?.[laneIndex]?.points) {
      setPoints(track.automationLanes[laneIndex].points);
      setSelectedParam(track.automationLanes[laneIndex].param || 'volume');
    } else {
      // Default: flat line at the parameter's default
      setPoints([
        { beat: 0, value: paramInfo.default },
        { beat: 32, value: paramInfo.default },
      ]);
    }
  }, [trackId, laneIndex]);

  // Canvas drawing
  const draw = useCallback(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    const W = canvas.width;
    const H = canvas.height;

    ctx.clearRect(0, 0, W, H);

    // Background
    ctx.fillStyle = 'rgba(18, 18, 30, 0.6)';
    ctx.fillRect(0, 0, W, H);

    // Grid lines (every 4 beats)
    ctx.strokeStyle = 'rgba(255,255,255,0.04)';
    ctx.lineWidth = 1;
    for (let beat = 0; beat <= 64; beat += 4) {
      const x = beat * horizontalZoom;
      ctx.beginPath();
      ctx.moveTo(x, 0);
      ctx.lineTo(x, H);
      ctx.stroke();
    }

    // Center line (for bipolar params like pan)
    if (paramInfo.min < 0) {
      ctx.strokeStyle = 'rgba(255,255,255,0.08)';
      ctx.setLineDash([4, 4]);
      ctx.beginPath();
      ctx.moveTo(0, H / 2);
      ctx.lineTo(W, H / 2);
      ctx.stroke();
      ctx.setLineDash([]);
    }

    if (points.length < 2) return;

    // Sort points by beat
    const sorted = [...points].sort((a, b) => a.beat - b.beat);

    // Normalize value to Y position
    const toY = (val) => {
      const norm = (val - paramInfo.min) / (paramInfo.max - paramInfo.min);
      return H - norm * (H - 8) - 4;
    };
    const toX = (beat) => beat * horizontalZoom;

    // Draw fill under curve
    ctx.beginPath();
    ctx.moveTo(toX(sorted[0].beat), H);
    ctx.lineTo(toX(sorted[0].beat), toY(sorted[0].value));
    for (let i = 1; i < sorted.length; i++) {
      ctx.lineTo(toX(sorted[i].beat), toY(sorted[i].value));
    }
    ctx.lineTo(toX(sorted[sorted.length - 1].beat), H);
    ctx.closePath();
    ctx.fillStyle = paramInfo.color + '15';
    ctx.fill();

    // Draw line
    ctx.beginPath();
    ctx.moveTo(toX(sorted[0].beat), toY(sorted[0].value));
    for (let i = 1; i < sorted.length; i++) {
      ctx.lineTo(toX(sorted[i].beat), toY(sorted[i].value));
    }
    ctx.strokeStyle = paramInfo.color;
    ctx.lineWidth = 2;
    ctx.stroke();

    // Draw points
    sorted.forEach((pt, i) => {
      const x = toX(pt.beat);
      const y = toY(pt.value);

      // Outer glow
      ctx.beginPath();
      ctx.arc(x, y, 6, 0, Math.PI * 2);
      ctx.fillStyle = paramInfo.color + '40';
      ctx.fill();

      // Inner dot
      ctx.beginPath();
      ctx.arc(x, y, 3.5, 0, Math.PI * 2);
      ctx.fillStyle = paramInfo.color;
      ctx.fill();
      ctx.strokeStyle = '#fff';
      ctx.lineWidth = 1;
      ctx.stroke();
    });
  }, [points, horizontalZoom, paramInfo]);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas || !containerRef.current) return;
    const rect = containerRef.current.getBoundingClientRect();
    canvas.width = Math.max(rect.width, 64 * horizontalZoom);
    canvas.height = 60;
    draw();
  }, [draw, horizontalZoom]);

  // Mouse handlers for adding/dragging points
  const handleMouseDown = (e) => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const rect = canvas.getBoundingClientRect();
    const x = e.clientX - rect.left;
    const y = e.clientY - rect.top;
    const H = canvas.height;

    // Check if clicking near an existing point
    const sorted = [...points].sort((a, b) => a.beat - b.beat);
    const hitRadius = 8;
    for (let i = 0; i < sorted.length; i++) {
      const px = sorted[i].beat * horizontalZoom;
      const norm = (sorted[i].value - paramInfo.min) / (paramInfo.max - paramInfo.min);
      const py = H - norm * (H - 8) - 4;
      if (Math.abs(x - px) < hitRadius && Math.abs(y - py) < hitRadius) {
        // Right click or ctrl+click to delete
        if (e.button === 2 || e.ctrlKey) {
          e.preventDefault();
          if (sorted.length > 2) {
            setPoints(prev => prev.filter((_, idx) => idx !== i));
          }
          return;
        }
        setIsDragging(true);
        setDragPointIdx(i);
        return;
      }
    }

    // Add a new point
    const beat = Math.max(0, x / horizontalZoom);
    const norm = 1 - (y - 4) / (H - 8);
    const value = Math.max(paramInfo.min, Math.min(paramInfo.max, paramInfo.min + norm * (paramInfo.max - paramInfo.min)));
    const newPoints = [...points, { beat: Math.round(beat * 4) / 4, value: Math.round(value * 100) / 100 }];
    setPoints(newPoints);
    setIsDragging(true);
    setDragPointIdx(newPoints.length - 1);
  };

  const handleMouseMove = useCallback((e) => {
    if (!isDragging || dragPointIdx < 0) return;
    const canvas = canvasRef.current;
    if (!canvas) return;
    const rect = canvas.getBoundingClientRect();
    const x = e.clientX - rect.left;
    const y = e.clientY - rect.top;
    const H = canvas.height;

    const beat = Math.max(0, x / horizontalZoom);
    const norm = 1 - (y - 4) / (H - 8);
    const value = Math.max(paramInfo.min, Math.min(paramInfo.max, paramInfo.min + norm * (paramInfo.max - paramInfo.min)));

    setPoints(prev => prev.map((pt, i) =>
      i === dragPointIdx ? { beat: Math.round(beat * 4) / 4, value: Math.round(value * 100) / 100 } : pt
    ));
  }, [isDragging, dragPointIdx, horizontalZoom, paramInfo]);

  const handleMouseUp = useCallback(() => {
    setIsDragging(false);
    setDragPointIdx(-1);
    // Save automation data to track store
    useProjectStore.getState().updateTrackAutomation(trackId, laneIndex, selectedParam, points);
  }, [trackId, laneIndex, selectedParam, points]);

  useEffect(() => {
    if (isDragging) {
      window.addEventListener('mousemove', handleMouseMove);
      window.addEventListener('mouseup', handleMouseUp);
    }
    return () => {
      window.removeEventListener('mousemove', handleMouseMove);
      window.removeEventListener('mouseup', handleMouseUp);
    };
  }, [isDragging, handleMouseMove, handleMouseUp]);

  return (
    <div className="automation-lane" ref={containerRef}>
      <div className="automation-lane-header">
        <select
          className="select input-sm automation-param-select"
          value={selectedParam}
          onChange={(e) => {
            setSelectedParam(e.target.value);
            const info = PARAM_OPTIONS.find(p => p.value === e.target.value);
            setPoints([
              { beat: 0, value: info?.default || 0 },
              { beat: 32, value: info?.default || 0 },
            ]);
          }}
        >
          {PARAM_OPTIONS.map(p => (
            <option key={p.value} value={p.value}>{p.label}</option>
          ))}
        </select>
        <span className="automation-color-dot" style={{ background: paramInfo.color }} />
      </div>
      <canvas
        ref={canvasRef}
        className="automation-canvas"
        onMouseDown={handleMouseDown}
        onContextMenu={(e) => e.preventDefault()}
      />
    </div>
  );
}
