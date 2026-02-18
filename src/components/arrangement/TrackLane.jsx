import React, { useRef, useEffect, useCallback } from 'react';
import { useUIStore } from '../../stores/uiStore';
import { useProjectStore } from '../../stores/projectStore';

export default function TrackLane({ track, trackIndex, height, pixelsPerBeat, totalWidth }) {
  const canvasRef = useRef(null);
  const { selectedClipId, setSelectedClip, activeTool, showContextMenu } = useUIStore();
  const { splitClip, updateClip, removeClip, reverseClip, bpm } = useProjectStore();
  
  // Drag state for fades
  const [dragState, setDragState] = React.useState(null); // { type: 'fadeIn'|'fadeOut', clipId, startX, startVal }
  const [hoverState, setHoverState] = React.useState(null); // 'fadeIn' | 'fadeOut' | null

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    canvas.width = totalWidth;
    canvas.height = height;

    // Background
    ctx.fillStyle = trackIndex % 2 === 0 ? '#151524' : '#171729';
    ctx.fillRect(0, 0, canvas.width, canvas.height);

    // Grid lines (beats)
    for (let beat = 0; beat <= totalWidth / pixelsPerBeat; beat++) {
      const x = beat * pixelsPerBeat;
      const isBar = beat % 4 === 0;
      ctx.strokeStyle = isBar ? '#1e1e32' : '#17172a';
      ctx.lineWidth = isBar ? 1 : 0.5;
      ctx.beginPath();
      ctx.moveTo(x, 0);
      ctx.lineTo(x, canvas.height);
      ctx.stroke();
    }

    // Muted overlay
    if (track.mute) {
      ctx.fillStyle = 'rgba(0,0,0,0.3)';
      ctx.fillRect(0, 0, canvas.width, canvas.height);
    }

    // Draw clips
    for (const clip of track.clips) {
      drawClip(ctx, clip, track, pixelsPerBeat, height, selectedClipId === clip.id, bpm);
    }

    // Bottom border
    ctx.strokeStyle = '#1a1a2e';
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.moveTo(0, canvas.height - 0.5);
    ctx.lineTo(canvas.width, canvas.height - 0.5);
    ctx.stroke();

  }, [track, trackIndex, height, pixelsPerBeat, totalWidth, selectedClipId, bpm]);

  // Helper to get mouse info
  const getMouseInfo = (e) => {
      const canvas = canvasRef.current;
      const rect = canvas.getBoundingClientRect();
      const x = e.clientX - rect.left;
      const y = e.clientY - rect.top;
      return { x, y };
  };

  const handleMouseDown = useCallback((e) => {
      const { x, y } = getMouseInfo(e);
      
      // Check for fade handles first
      for (const clip of track.clips) {
          const cx = clip.startBeat * pixelsPerBeat;
          const cw = clip.lengthBeats * pixelsPerBeat;
          const fadeW = 10; // Handle width
          
          if (clip.type === 'audio') {
              // Fade In Handle (Top Left)
              if (Math.abs(x - (cx + (clip.fadeIn || 0) * (bpm/60) * pixelsPerBeat)) < fadeW && y < 20) {
                  setDragState({ type: 'fadeIn', clipId: clip.id, startX: x, startVal: clip.fadeIn || 0 });
                 return;
              }
              // Fade Out Handle (Top Right)
              if (Math.abs(x - (cx + cw - (clip.fadeOut || 0) * (bpm/60) * pixelsPerBeat)) < fadeW && y < 20) {
                  setDragState({ type: 'fadeOut', clipId: clip.id, startX: x, startVal: clip.fadeOut || 0 });
                  return;
              }
          }
      }

      // Normal Click / Split
      for (const clip of track.clips) {
        const clipX = clip.startBeat * pixelsPerBeat;
        const clipW = clip.lengthBeats * pixelsPerBeat;
        if (x >= clipX && x <= clipX + clipW) {
             if (activeTool === 'split') {
                 const splitBeat = x / pixelsPerBeat;
                 splitClip(track.id, clip.id, splitBeat);
             } else {
                 setSelectedClip(track.id, clip.id);
             }
             return;
        }
      }
      setSelectedClip(null, null);
  }, [track, pixelsPerBeat, activeTool, splitClip, setSelectedClip, bpm]);

  const handleMouseMove = useCallback((e) => {
      const { x, y } = getMouseInfo(e);

      if (dragState) {
          const clip = track.clips.find(c => c.id === dragState.clipId);
          if (!clip) return;
          
          const deltaPx = x - dragState.startX;
          const deltaTime = (deltaPx / pixelsPerBeat) * (60 / bpm);
          
          if (dragState.type === 'fadeIn') {
              const newVal = Math.max(0, Math.min(clip.lengthBeats * (60/bpm), dragState.startVal + deltaTime));
              updateClip(track.id, clip.id, { fadeIn: newVal });
          } else {
              const newVal = Math.max(0, Math.min(clip.lengthBeats * (60/bpm), dragState.startVal - deltaTime));
              updateClip(track.id, clip.id, { fadeOut: newVal });
          }
          return;
      }

      // Hover detection
      let hit = null;
      for (const clip of track.clips) {
          if (clip.type !== 'audio') continue;
          const cx = clip.startBeat * pixelsPerBeat;
          const cw = clip.lengthBeats * pixelsPerBeat;
          const fadeW = 10;
          const inX = cx + (clip.fadeIn || 0) * (bpm/60) * pixelsPerBeat;
          const outX = cx + cw - (clip.fadeOut || 0) * (bpm/60) * pixelsPerBeat;

          if (Math.abs(x - inX) < fadeW && y < 20) hit = 'fadeIn';
          else if (Math.abs(x - outX) < fadeW && y < 20) hit = 'fadeOut';
      }
      setHoverState(hit);

  }, [dragState, track, pixelsPerBeat, bpm, updateClip]);

  const handleMouseUp = useCallback(() => {
      setDragState(null);
  }, []);

  const handleContextMenu = useCallback((e) => {
      e.preventDefault();
      const { x, y } = getMouseInfo(e);
      
      for (const clip of track.clips) {
          const clipX = clip.startBeat * pixelsPerBeat;
          const clipW = clip.lengthBeats * pixelsPerBeat;
          if (x >= clipX && x <= clipX + clipW) {
             showContextMenu(e.clientX, e.clientY, [
                 { label: `Rename "${clip.name}"`, action: () => {
                     const newName = window.prompt('Rename Clip:', clip.name);
                     if (newName) updateClip(track.id, clip.id, { name: newName });
                 }},
                 { divider: true },
                 { label: 'Split at Cursor', action: () => {
                     const splitBeat = x / pixelsPerBeat;
                     splitClip(track.id, clip.id, splitBeat);
                 }},
                 { label: clip.isReversed ? 'Un-Reverse Audio' : 'Reverse Audio', action: () => {
                     if (clip.type === 'audio') reverseClip(track.id, clip.id);
                 }},
                 { divider: true },
                 { label: 'Delete', danger: true, action: () => removeClip(track.id, clip.id) }
             ]);
             return;
          }
      }
  }, [track, pixelsPerBeat, showContextMenu, splitClip, updateClip, removeClip, reverseClip]);

  // Global mouse up to catch release outside canvas
  useEffect(() => {
      window.addEventListener('mouseup', handleMouseUp);
      window.addEventListener('mousemove', handleMouseMove); // Handle drag outside
      return () => {
          window.removeEventListener('mouseup', handleMouseUp);
          window.removeEventListener('mousemove', handleMouseMove);
      };
  }, [handleMouseUp, handleMouseMove]);


  const getCursor = () => {
      if (hoverState || dragState) return 'ew-resize';
      if (activeTool === 'split') return 'cell';
      return 'default';
  };

  return (
    <div className="track-lane" style={{ height, cursor: getCursor() }}>
      <canvas
        ref={canvasRef}
        className="track-lane-canvas"
        onMouseDown={handleMouseDown}
        onContextMenu={handleContextMenu}
        // remove onClick, move logic to onMouseDown to handle drag
      />
    </div>
  );
}


function drawClip(ctx, clip, track, pxPerBeat, laneHeight, isSelected, bpm) {
  const x = clip.startBeat * pxPerBeat;
  const w = clip.lengthBeats * pxPerBeat;
  const y = 2;
  const h = laneHeight - 4;
  const r = 4;

  // Clip body with color
  const clipColor = clip.color || track.color;
  ctx.fillStyle = clip.type === 'midi' ? '#1a2840' : '#1e1e3a';
  ctx.beginPath();
  ctx.roundRect(x, y, w, h, r);
  ctx.fill();

  // Colored top bar
  ctx.fillStyle = clipColor;
  ctx.beginPath();
  ctx.roundRect(x, y, w, 18, [r, r, 0, 0]);
  ctx.fill();

  // Fades Visualization
  if (clip.type === 'audio' && bpm) {
      if (clip.fadeIn > 0 || clip.fadeOut > 0) {
          ctx.fillStyle = 'rgba(255,255,255,0.2)';
          
          if (clip.fadeIn) {
              const fadeInBeats = clip.fadeIn * (bpm / 60);
              const fadeW = Math.min(w, fadeInBeats * pxPerBeat);
              ctx.beginPath();
              ctx.moveTo(x, y); // Top Left
              ctx.lineTo(x + fadeW, y); // Top Right of fade
              ctx.lineTo(x, y + 18); // Bottom Left of header
              ctx.fill();
          }
          if (clip.fadeOut) {
              const fadeOutBeats = clip.fadeOut * (bpm / 60);
              const fadeW = Math.min(w, fadeOutBeats * pxPerBeat);
              ctx.beginPath();
              ctx.moveTo(x + w, y); // Top Right
              ctx.lineTo(x + w - fadeW, y); // Top Left of fade
              ctx.lineTo(x + w, y + 18); // Bottom Right of header
              ctx.fill();
          }
      }
      
      // Draw Handles (Always visible if audio?)
      // Actually only checking logic used them in hover...
      // Let's draw small circles/triangles if we want them visible always
      ctx.fillStyle = 'rgba(255,255,255,0.5)';
      const fadeHandleW = 10;
      
      // Fade In Handle
      const inBeats = (clip.fadeIn || 0) * (bpm / 60);
      const inX = x + inBeats * pxPerBeat;
      ctx.beginPath();
      ctx.moveTo(inX, y);
      ctx.lineTo(inX + 6, y);
      ctx.lineTo(inX, y + 6);
      ctx.fill();
      
      // Fade Out Handle
      const outBeats = (clip.fadeOut || 0) * (bpm / 60);
      const outX = x + w - outBeats * pxPerBeat;
      ctx.beginPath();
      ctx.moveTo(outX, y);
      ctx.lineTo(outX - 6, y);
      ctx.lineTo(outX, y + 6);
      ctx.fill();
  }

  // Clip name
  ctx.fillStyle = '#fff';
  ctx.font = '10px Inter, sans-serif';
  ctx.fillText(clip.name, x + 6, y + 13, w - 12);

  // Waveform or MIDI display
  const contentY = y + 20;
  const contentH = h - 22;

  if (clip.type === 'audio' && clip.waveformData) {
    drawWaveform(ctx, clip.waveformData, x + 2, contentY, w - 4, contentH, clipColor);
  } else if (clip.type === 'midi' && clip.notes) {
    drawMidiPreview(ctx, clip.notes, x + 2, contentY, w - 4, contentH, clip.lengthBeats, clipColor);
  }

  // Selection border
  if (isSelected) {
    ctx.strokeStyle = '#fff';
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.roundRect(x, y, w, h, r);
    ctx.stroke();
  } else {
    ctx.strokeStyle = 'rgba(255,255,255,0.1)';
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.roundRect(x, y, w, h, r);
    ctx.stroke();
  }
}

function drawWaveform(ctx, data, x, y, w, h, color) {
  const mid = y + h / 2;
  const step = data.length / w;

  ctx.fillStyle = color + '30';
  ctx.strokeStyle = color + '90';
  ctx.lineWidth = 1;
  ctx.beginPath();
  ctx.moveTo(x, mid);

  for (let i = 0; i < w; i++) {
    const idx = Math.floor(i * step);
    const val = data[idx] || 0;
    const amplitude = val * (h / 2) * 0.9;
    ctx.lineTo(x + i, mid - amplitude);
  }

  for (let i = w - 1; i >= 0; i--) {
    const idx = Math.floor(i * step);
    const val = data[idx] || 0;
    const amplitude = val * (h / 2) * 0.9;
    ctx.lineTo(x + i, mid + amplitude);
  }

  ctx.closePath();
  ctx.fill();

  // Center line
  ctx.beginPath();
  ctx.moveTo(x, mid);
  for (let i = 0; i < w; i++) {
    const idx = Math.floor(i * step);
    const val = data[idx] || 0;
    ctx.lineTo(x + i, mid - val * (h / 2) * 0.9);
  }
  ctx.stroke();
}

function drawMidiPreview(ctx, notes, x, y, w, h, clipLength, color) {
  if (!notes.length) return;

  const minNote = Math.min(...notes.map(n => n.pitch)) - 1;
  const maxNote = Math.max(...notes.map(n => n.pitch)) + 1;
  const noteRange = maxNote - minNote || 1;

  for (const note of notes) {
    const nx = x + (note.startBeat / clipLength) * w;
    const nw = Math.max(2, (note.lengthBeats / clipLength) * w);
    const ny = y + ((maxNote - note.pitch) / noteRange) * h;
    const nh = Math.max(2, h / noteRange * 0.8);

    const alpha = Math.round((note.velocity / 127) * 200 + 55).toString(16).padStart(2, '0');
    ctx.fillStyle = color + alpha;
    ctx.fillRect(nx, ny, nw, nh);
  }
}
