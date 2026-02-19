import React, { useRef, useEffect, useCallback, useState } from 'react';
import { useProjectStore } from '../../stores/projectStore';
import { useUIStore } from '../../stores/uiStore';
import { midiToNoteName, uid } from '../../utils/helpers';

const TOTAL_NOTES = 128;
const NOTE_HEIGHT = 14;
const PIANO_WIDTH = 48;

export default function PianoRollView() {
  const canvasRef = useRef(null);
  const pianoRef = useRef(null);
  const scrollRef = useRef(null);
  const velocityScrollRef = useRef(null);
  const { horizontalZoom, snapValue, snapEnabled, selectedClipTrackId, selectedClipId } = useUIStore();
  const tracks = useProjectStore(s => s.tracks);
  const { addNote, removeNote, updateNote } = useProjectStore();

  // Find the selected MIDI clip
  let selectedClip = null;
  let selectedTrack = null;
  if (selectedClipTrackId && selectedClipId) {
    selectedTrack = tracks.find(t => t.id === selectedClipTrackId);
    if (selectedTrack) {
      selectedClip = selectedTrack.clips.find(c => c.id === selectedClipId);
    }
  }

  // Fallback: use first MIDI clip
  if (!selectedClip) {
    for (const track of tracks) {
      if (track.type === 'midi') {
        for (const clip of track.clips) {
          if (clip.type === 'midi') {
            selectedClip = clip;
            selectedTrack = track;
            break;
          }
        }
        if (selectedClip) break;
      }
    }
  }

  const clipLength = selectedClip?.lengthBeats || 16;
  const gridWidth = clipLength * horizontalZoom;
  const totalHeight = TOTAL_NOTES * NOTE_HEIGHT;

  // Draw piano keys
  useEffect(() => {
    const canvas = pianoRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    canvas.width = PIANO_WIDTH;
    canvas.height = totalHeight;

    for (let note = 0; note < TOTAL_NOTES; note++) {
      const y = (TOTAL_NOTES - 1 - note) * NOTE_HEIGHT;
      const noteName = midiToNoteName(note);
      const isBlack = noteName.includes('#');

      ctx.fillStyle = isBlack ? '#1a1a2e' : '#252540';
      ctx.fillRect(0, y, PIANO_WIDTH, NOTE_HEIGHT);

      ctx.strokeStyle = '#12121e';
      ctx.lineWidth = 0.5;
      ctx.beginPath();
      ctx.moveTo(0, y + NOTE_HEIGHT);
      ctx.lineTo(PIANO_WIDTH, y + NOTE_HEIGHT);
      ctx.stroke();

      // Note label for C notes
      if (note % 12 === 0) {
        ctx.fillStyle = '#a0a0b8';
        ctx.font = '9px Inter, sans-serif';
        ctx.fillText(noteName, 4, y + NOTE_HEIGHT - 3);
      }
    }
  }, [totalHeight]);

  // Draw grid and notes
  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    canvas.width = gridWidth;
    canvas.height = totalHeight;

    // Background
    for (let note = 0; note < TOTAL_NOTES; note++) {
      const y = (TOTAL_NOTES - 1 - note) * NOTE_HEIGHT;
      const noteName = midiToNoteName(note);
      const isBlack = noteName.includes('#');
      ctx.fillStyle = isBlack ? '#0e0e1a' : '#141420';
      ctx.fillRect(0, y, gridWidth, NOTE_HEIGHT);
    }

    // Grid lines (beats)
    for (let beat = 0; beat <= clipLength; beat++) {
      const x = beat * horizontalZoom;
      const isBar = beat % 4 === 0;
      ctx.strokeStyle = isBar ? '#2a2a42' : '#1a1a2e';
      ctx.lineWidth = isBar ? 1 : 0.5;
      ctx.beginPath();
      ctx.moveTo(x, 0);
      ctx.lineTo(x, totalHeight);
      ctx.stroke();
    }

    // Horizontal lines
    for (let note = 0; note < TOTAL_NOTES; note++) {
      const y = (TOTAL_NOTES - 1 - note) * NOTE_HEIGHT;
      const isC = note % 12 === 0;
      ctx.strokeStyle = isC ? '#2a2a42' : '#16162a';
      ctx.lineWidth = isC ? 1 : 0.3;
      ctx.beginPath();
      ctx.moveTo(0, y + NOTE_HEIGHT);
      ctx.lineTo(gridWidth, y + NOTE_HEIGHT);
      ctx.stroke();
    }

    // Draw notes
    if (selectedClip && selectedClip.notes) {
      for (const note of selectedClip.notes) {
        const x = note.startBeat * horizontalZoom;
        const w = Math.max(4, note.lengthBeats * horizontalZoom);
        const y = (TOTAL_NOTES - 1 - note.pitch) * NOTE_HEIGHT + 1;
        const h = NOTE_HEIGHT - 2;

        // Velocity-based color
        const vel = note.velocity / 127;
        const r = Math.round(80 + vel * 100);
        const g = Math.round(130 - vel * 60);
        const b = Math.round(200 + vel * 55);

        ctx.fillStyle = `rgb(${r},${g},${b})`;
        ctx.beginPath();
        ctx.roundRect(x, y, w, h, 2);
        ctx.fill();

        // Border
        ctx.strokeStyle = `rgba(255,255,255,0.3)`;
        ctx.lineWidth = 1;
        ctx.beginPath();
        ctx.roundRect(x, y, w, h, 2);
        ctx.stroke();

        // Note name inside if large enough
        if (w > 30) {
          ctx.fillStyle = '#fff';
          ctx.font = '9px Inter, sans-serif';
          ctx.fillText(midiToNoteName(note.pitch), x + 3, y + h - 2);
        }
      }
    }
  }, [selectedClip, horizontalZoom, totalHeight, gridWidth, clipLength]);

  // Handle click to add notes
  const handleCanvasClick = useCallback((e) => {
    if (!selectedClip || !selectedTrack) return;
    const canvas = canvasRef.current;
    const rect = canvas.getBoundingClientRect();
    const scrollLeft = scrollRef.current?.scrollLeft || 0;
    const scrollTop = scrollRef.current?.scrollTop || 0;

    const x = e.clientX - rect.left + scrollLeft;
    const y = e.clientY - rect.top + scrollTop;

    const beat = x / horizontalZoom;
    const pitch = TOTAL_NOTES - 1 - Math.floor(y / NOTE_HEIGHT);

    // Check if clicking existing note
    if (selectedClip.notes) {
      for (const note of selectedClip.notes) {
        const nx = note.startBeat * horizontalZoom;
        const nw = note.lengthBeats * horizontalZoom;
        const ny = (TOTAL_NOTES - 1 - note.pitch) * NOTE_HEIGHT;
        if (x >= nx && x <= nx + nw && y >= ny && y <= ny + NOTE_HEIGHT) {
          removeNote(selectedTrack.id, selectedClip.id, note.id);
          return;
        }
      }
    }

    // Add new note
    const snap = snapEnabled ? snapValue : 0.25;
    const snappedBeat = Math.floor(beat / snap) * snap;
    const newNote = {
      id: uid(),
      pitch: Math.max(0, Math.min(127, pitch)),
      startBeat: snappedBeat,
      lengthBeats: snap,
      velocity: 100,
    };
    addNote(selectedTrack.id, selectedClip.id, newNote);
  }, [selectedClip, selectedTrack, horizontalZoom, snapValue, snapEnabled, addNote, removeNote]);

  // Sync Scroll
  const handleScroll = (e) => {
      if (velocityScrollRef.current) {
          velocityScrollRef.current.scrollLeft = e.target.scrollLeft;
      }
  };

  const handleVelocityScroll = (e) => {
      if (scrollRef.current) {
          scrollRef.current.scrollLeft = e.target.scrollLeft;
      }
  };

  // Scroll to middle C on mount (only once)
  useEffect(() => {
    if (scrollRef.current) {
      const middleCY = (TOTAL_NOTES - 60) * NOTE_HEIGHT;
      scrollRef.current.scrollTop = middleCY - scrollRef.current.clientHeight / 2;
    }
  }, []);

  return (
    <div className="pianoroll-view">
      <div className="pianoroll-header">
        <span>PIANO ROLL</span>
        {selectedClip && (
          <span className="text-muted" style={{ marginLeft: 8, fontSize: 'var(--text-xs)' }}>
            {selectedClip.name} — {selectedTrack?.name}
          </span>
        )}
      </div>
      <div className="pianoroll-split-container">
        {/* Keys + Grid Section */}
        <div className="pianoroll-main-row">
            <div className="piano-keys-container">
                <canvas ref={pianoRef} className="piano-keys-canvas" />
            </div>
            <div 
                className="pianoroll-grid-scroll" 
                ref={scrollRef} 
                onScroll={handleScroll}
            >
                <canvas
                    ref={canvasRef}
                    className="pianoroll-grid-canvas"
                    onClick={handleCanvasClick}
                    // onMouseDown...
                />
            </div>
        </div>

        {/* Velocity Lane */}
        <div className="pianoroll-velocity-row" style={{ height: 100 }}>
            <div className="piano-keys-spacer" style={{ width: PIANO_WIDTH, borderRight: '1px solid #333' }}>
                <span className="text-muted" style={{ fontSize: 10, padding: 4 }}>VEL</span>
            </div>
            <div 
                className="pianoroll-velocity-scroll" 
                ref={velocityScrollRef}
                onScroll={handleVelocityScroll}
            >
                <VelocityLane 
                    notes={selectedClip?.notes || []} 
                    clipLength={clipLength}
                    zoom={horizontalZoom}
                    gridWidth={gridWidth}
                    onUpdateVelocity={(noteId, vel) => updateNote(selectedTrack.id, selectedClip.id, noteId, { velocity: vel })}
                />
            </div>
        </div>
      </div>
      {!selectedClip && (
        <div className="pianoroll-empty">
          <p className="text-muted">Select a MIDI clip to edit</p>
        </div>
      )}
    </div>
  );
}

function VelocityLane({ notes, clipLength, zoom, gridWidth, onUpdateVelocity }) {
    const canvasRef = useRef(null);

    useEffect(() => {
        const canvas = canvasRef.current;
        if (!canvas) return;
        const ctx = canvas.getContext('2d');
        canvas.width = gridWidth;
        canvas.height = 100;

        // Background
        ctx.fillStyle = '#111';
        ctx.fillRect(0, 0, canvas.width, canvas.height);

        // Grid lines (beats)
        for (let beat = 0; beat <= clipLength; beat++) {
            const x = beat * zoom;
            const isBar = beat % 4 === 0;
            ctx.strokeStyle = isBar ? '#2a2a42' : '#1a1a2e';
            ctx.lineWidth = isBar ? 1 : 0.5;
            ctx.beginPath();
            ctx.moveTo(x, 0);
            ctx.lineTo(x, canvas.height);
            ctx.stroke();
        }

        // Draw Velocity Bars
        notes.forEach(note => {
            const x = note.startBeat * zoom;
            const w = Math.max(4, note.lengthBeats * zoom);
            const h = (note.velocity / 127) * canvas.height;
            const y = canvas.height - h;

            ctx.fillStyle = `rgb(100, 150, 255)`; // Blue-ish
            ctx.fillRect(x, y, w, h);

            // Handle head
            ctx.fillStyle = '#fff';
            ctx.fillRect(x, y, w, 2);
        });

        // Line at top
        ctx.strokeStyle = '#333';
        ctx.beginPath();
        ctx.moveTo(0, 0);
        ctx.lineTo(canvas.width, 0);
        ctx.stroke();

    }, [notes, clipLength, zoom, gridWidth]);

    const handleMouseDown = (e) => {
        const canvas = canvasRef.current;
        const rect = canvas.getBoundingClientRect();
        const startY = e.clientY;
        const x = e.clientX - rect.left;
        
        // Find note at x
        // If multiple notes overlap at X, pick the one currently playing? 
        // Or finding the one with closest center?
        // Simple: find first matching note.
        const beat = x / zoom;
        const note = notes.find(n => beat >= n.startBeat && beat <= n.startBeat + n.lengthBeats);
        
        if (note) {
             const onMove = (mv) => {
                 const rect = canvas.getBoundingClientRect();
                 const y = Math.max(0, Math.min(canvas.height, mv.clientY - rect.top));
                 const val = 127 - (y / canvas.height) * 127;
                 onUpdateVelocity(note.id, Math.round(val));
             };
             const onUp = () => {
                 window.removeEventListener('mousemove', onMove);
                 window.removeEventListener('mouseup', onUp);
             };
             window.addEventListener('mousemove', onMove);
             window.addEventListener('mouseup', onUp);
             
             // Initial set
             const y = Math.max(0, Math.min(canvas.height, e.clientY - rect.top));
             const val = 127 - (y / canvas.height) * 127;
             onUpdateVelocity(note.id, Math.round(val));
        }
    };

    return (
        <canvas 
            ref={canvasRef} 
            className="velocity-lane-canvas"
            style={{ width: gridWidth, height: 100, display: 'block' }}
            onMouseDown={handleMouseDown}
        />
    );
}
