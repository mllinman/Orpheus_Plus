import React, { useState, useMemo, useCallback } from 'react';
import { useProjectStore } from '../../stores/projectStore';
import { useUIStore } from '../../stores/uiStore';
import { uid } from '../../utils/helpers';

// ── Instrument Presets ──
const INSTRUMENT_LIBRARY = {
  drums: {
    label: '🥁 Drums',
    instruments: [
      { id: 'kick', name: 'Kick', note: 36, icon: '🔴' },
      { id: 'snare', name: 'Snare', note: 38, icon: '🟡' },
      { id: 'clap', name: 'Clap', note: 39, icon: '👏' },
      { id: 'hihat_closed', name: 'Closed Hi-Hat', note: 42, icon: '🔵' },
      { id: 'hihat_open', name: 'Open Hi-Hat', note: 46, icon: '🟣' },
      { id: 'ride', name: 'Ride', note: 51, icon: '🟤' },
      { id: 'crash', name: 'Crash', note: 49, icon: '💥' },
      { id: 'tom_high', name: 'High Tom', note: 50, icon: '🔶' },
      { id: 'tom_mid', name: 'Mid Tom', note: 47, icon: '🔷' },
      { id: 'tom_low', name: 'Low Tom', note: 45, icon: '⬛' },
      { id: 'rim', name: 'Rimshot', note: 37, icon: '⭕' },
      { id: 'cowbell', name: 'Cowbell', note: 56, icon: '🔔' },
    ]
  },
  percussion: {
    label: '🪘 Percussion',
    instruments: [
      { id: 'shaker', name: 'Shaker', note: 70, icon: '🎵' },
      { id: 'tambourine', name: 'Tambourine', note: 54, icon: '🪩' },
      { id: 'bongo_hi', name: 'Bongo High', note: 60, icon: '🥁' },
      { id: 'bongo_lo', name: 'Bongo Low', note: 61, icon: '🪘' },
      { id: 'conga_hi', name: 'Conga High', note: 62, icon: '🎶' },
      { id: 'conga_lo', name: 'Conga Low', note: 63, icon: '🎼' },
      { id: 'woodblock', name: 'Woodblock', note: 76, icon: '🪵' },
      { id: 'triangle', name: 'Triangle', note: 81, icon: '🔺' },
    ]
  },
  bass: {
    label: '🎸 Bass',
    instruments: [
      { id: 'bass_c1', name: 'Bass C1', note: 24, icon: '🎸' },
      { id: 'bass_e1', name: 'Bass E1', note: 28, icon: '🎸' },
      { id: 'bass_g1', name: 'Bass G1', note: 31, icon: '🎸' },
      { id: 'bass_c2', name: 'Bass C2', note: 36, icon: '🎸' },
      { id: 'sub_bass', name: 'Sub Bass', note: 24, icon: '📯' },
    ]
  },
  keys: {
    label: '🎹 Keys',
    instruments: [
      { id: 'piano_c3', name: 'Piano C3', note: 48, icon: '🎹' },
      { id: 'piano_c4', name: 'Piano C4 (Middle C)', note: 60, icon: '🎹' },
      { id: 'piano_c5', name: 'Piano C5', note: 72, icon: '🎹' },
      { id: 'organ', name: 'Organ', note: 60, icon: '⛪' },
      { id: 'synth_pad', name: 'Synth Pad', note: 60, icon: '🌊' },
      { id: 'synth_lead', name: 'Synth Lead', note: 60, icon: '⚡' },
    ]
  },
  guitar: {
    label: '🎸 Guitar',
    instruments: [
      { id: 'guitar_e2', name: 'Guitar E2', note: 40, icon: '🎸' },
      { id: 'guitar_a2', name: 'Guitar A2', note: 45, icon: '🎸' },
      { id: 'guitar_d3', name: 'Guitar D3', note: 50, icon: '🎸' },
      { id: 'guitar_g3', name: 'Guitar G3', note: 55, icon: '🎸' },
      { id: 'guitar_b3', name: 'Guitar B3', note: 59, icon: '🎸' },
      { id: 'guitar_e4', name: 'Guitar E4', note: 64, icon: '🎸' },
      { id: 'guitar_strum', name: 'Guitar Strum', note: 48, icon: '🎶' },
    ]
  },
  fx: {
    label: '✨ FX & One-shots',
    instruments: [
      { id: 'riser', name: 'Riser', note: 60, icon: '📈' },
      { id: 'impact', name: 'Impact', note: 36, icon: '💥' },
      { id: 'sweep', name: 'Sweep', note: 72, icon: '🌀' },
      { id: 'vinyl_stop', name: 'Vinyl Stop', note: 48, icon: '📀' },
      { id: 'noise_hit', name: 'Noise Hit', note: 60, icon: '💨' },
    ]
  },
};

export default function MidiInstrumentPicker({ trackId, clipId, onClose }) {
  const { addNote, updateNote, removeNote } = useProjectStore();
  const tracks = useProjectStore(s => s.tracks);
  const { horizontalZoom, snapValue } = useUIStore();

  const [selectedCategory, setSelectedCategory] = useState('drums');
  const [selectedInstrument, setSelectedInstrument] = useState(null);
  const [replaceMode, setReplaceMode] = useState(false); // Replace existing notes vs add
  const [targetBeat, setTargetBeat] = useState(0);
  const [velocity, setVelocity] = useState(100);
  const [noteLength, setNoteLength] = useState(0.25); // Quarter beat

  // Find the clip
  const track = tracks.find(t => t.id === trackId);
  const clip = track?.clips?.find(c => c.id === clipId);
  const notes = clip?.notes || [];

  const categories = useMemo(() => Object.entries(INSTRUMENT_LIBRARY), []);

  const handleAddInstrument = useCallback(() => {
    if (!selectedInstrument || !clipId || !trackId) return;

    const newNote = {
      id: uid(),
      pitch: selectedInstrument.note,
      startBeat: targetBeat,
      lengthBeats: noteLength,
      velocity,
      instrumentId: selectedInstrument.id,
      instrumentName: selectedInstrument.name,
    };

    addNote(trackId, clipId, newNote);
  }, [selectedInstrument, targetBeat, noteLength, velocity, trackId, clipId, addNote]);

  const handleReplaceAtBeat = useCallback((beat) => {
    if (!selectedInstrument || !clipId || !trackId) return;

    // Find notes at this beat
    const notesAtBeat = notes.filter(n =>
      n.startBeat <= beat && n.startBeat + (n.lengthBeats || 0.25) > beat
    );

    // Remove existing notes at this beat
    notesAtBeat.forEach(n => removeNote(trackId, clipId, n.id));

    // Add new instrument note
    const newNote = {
      id: uid(),
      pitch: selectedInstrument.note,
      startBeat: beat,
      lengthBeats: noteLength,
      velocity,
      instrumentId: selectedInstrument.id,
      instrumentName: selectedInstrument.name,
    };
    addNote(trackId, clipId, newNote);
  }, [selectedInstrument, notes, noteLength, velocity, trackId, clipId, addNote, removeNote]);

  return (
    <div className="midi-instrument-picker">
      <div className="panel-header">
        <span>🎹 Instrument Picker</span>
        {onClose && <button className="btn btn-xs btn-ghost" onClick={onClose}>✕</button>}
      </div>

      <div className="mip-body">
        {/* Category Tabs */}
        <div className="mip-categories">
          {categories.map(([key, cat]) => (
            <button
              key={key}
              className={`mip-cat-btn ${selectedCategory === key ? 'active' : ''}`}
              onClick={() => { setSelectedCategory(key); setSelectedInstrument(null); }}
            >
              {cat.label}
            </button>
          ))}
        </div>

        {/* Instrument Grid */}
        <div className="mip-instruments">
          {INSTRUMENT_LIBRARY[selectedCategory]?.instruments.map(inst => (
            <button
              key={inst.id}
              className={`mip-inst-btn ${selectedInstrument?.id === inst.id ? 'active' : ''}`}
              onClick={() => setSelectedInstrument(inst)}
              title={`MIDI Note ${inst.note}`}
            >
              <span className="mip-inst-icon">{inst.icon}</span>
              <span className="mip-inst-name">{inst.name}</span>
            </button>
          ))}
        </div>

        {/* Controls */}
        {selectedInstrument && (
          <div className="mip-controls">
            <div className="mip-selected">
              <span>{selectedInstrument.icon} <strong>{selectedInstrument.name}</strong></span>
              <span className="mip-note-num">MIDI {selectedInstrument.note}</span>
            </div>

            <div className="mip-row">
              <label>
                Beat Position
                <input
                  type="number"
                  min={0}
                  step={snapValue || 0.25}
                  value={targetBeat}
                  onChange={e => setTargetBeat(parseFloat(e.target.value) || 0)}
                  className="input input-sm"
                  style={{ width: 70 }}
                />
              </label>
              <label>
                Velocity
                <input
                  type="range" min={1} max={127}
                  value={velocity}
                  onChange={e => setVelocity(parseInt(e.target.value))}
                  className="mip-slider"
                />
                <span className="mip-val">{velocity}</span>
              </label>
            </div>

            <div className="mip-row">
              <label>
                Length
                <select
                  value={noteLength}
                  onChange={e => setNoteLength(parseFloat(e.target.value))}
                  className="select input-sm"
                >
                  <option value={0.0625}>1/64</option>
                  <option value={0.125}>1/32</option>
                  <option value={0.25}>1/16</option>
                  <option value={0.5}>1/8</option>
                  <option value={1}>1/4</option>
                  <option value={2}>1/2</option>
                  <option value={4}>1 bar</option>
                </select>
              </label>
              <label className="mip-replace-toggle">
                <input
                  type="checkbox"
                  checked={replaceMode}
                  onChange={e => setReplaceMode(e.target.checked)}
                />
                Replace mode
              </label>
            </div>

            <div className="mip-actions">
              <button className="btn btn-primary btn-sm" onClick={handleAddInstrument}>
                ➕ Add at Beat {targetBeat}
              </button>
              {replaceMode && (
                <button
                  className="btn btn-sm btn-ghost"
                  onClick={() => handleReplaceAtBeat(targetBeat)}
                >
                  🔄 Replace at Beat {targetBeat}
                </button>
              )}
            </div>

            {/* Quick beat grid — click to place */}
            <div className="mip-beat-grid-label">Click to place on beat:</div>
            <div className="mip-beat-grid">
              {Array.from({ length: Math.min(clip?.lengthBeats || 16, 32) }, (_, i) => {
                const beat = i * (snapValue || 1);
                const hasNote = notes.some(n =>
                  n.startBeat === beat && n.pitch === selectedInstrument?.note
                );
                return (
                  <button
                    key={i}
                    className={`mip-beat-cell ${hasNote ? 'filled' : ''} ${i % 4 === 0 ? 'bar-start' : ''}`}
                    onClick={() => {
                      setTargetBeat(beat);
                      if (replaceMode) handleReplaceAtBeat(beat);
                      else handleAddInstrument();
                    }}
                    title={`Beat ${beat}`}
                  />
                );
              })}
            </div>
          </div>
        )}
      </div>
    </div>
  );
}
