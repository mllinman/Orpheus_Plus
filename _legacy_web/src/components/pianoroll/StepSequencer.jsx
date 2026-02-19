import React, { useState, useCallback } from 'react';
import { useProjectStore } from '../../stores/projectStore';
import { useUIStore } from '../../stores/uiStore';

const DRUM_LABELS = [
  'Kick', 'Snare', 'Hi-Hat', 'Open HH', 'Clap', 'Tom Hi', 'Tom Mid', 'Tom Lo',
  'Crash', 'Ride', 'Rim', 'Cowbell', 'Shaker', 'Tamb', 'Clave', 'Perc'
];

const NOTE_MAP = [36, 38, 42, 46, 39, 50, 47, 43, 49, 51, 37, 56, 69, 54, 75, 67];

export default function StepSequencer({ steps = 16 }) {
  const { selectedTrackId } = useUIStore();
  const { tracks, addNote, removeNote } = useProjectStore();
  const track = tracks.find(t => t.id === selectedTrackId);

  const [numSteps, setNumSteps] = useState(steps);
  const [swing, setSwing] = useState(0);
  const [velocities, setVelocities] = useState(
    Array.from({ length: 16 }, () => new Array(64).fill(100))
  );
  const [probabilities, setProbabilities] = useState(
    Array.from({ length: 16 }, () => new Array(64).fill(1))
  );

  // Grid state: grid[row][step] = active
  const [grid, setGrid] = useState(() =>
    Array.from({ length: 16 }, () => new Array(64).fill(false))
  );

  const toggleStep = useCallback((row, step) => {
    setGrid(prev => {
      const next = prev.map(r => [...r]);
      next[row][step] = !next[row][step];

      // Sync with project store
      if (selectedTrackId && track) {
        const note = NOTE_MAP[row];
        const stepLengthBeats = 4 / numSteps; // assume 1 bar = 4 beats
        const startBeat = step * stepLengthBeats;
        const swingOffset = step % 2 === 1 ? swing * stepLengthBeats * 0.5 : 0;

        if (next[row][step]) {
          // Add note
          const targetClipId = track.clips?.[0]?.id;
          if (targetClipId) {
            addNote(selectedTrackId, targetClipId, {
              pitch: note,
              startBeat: startBeat + swingOffset,
              lengthBeats: stepLengthBeats * 0.9,
              velocity: velocities[row][step],
            });
          }
        }
      }
      return next;
    });
  }, [selectedTrackId, track, numSteps, swing, velocities, addNote]);

  const adjustVelocity = useCallback((row, step, delta) => {
    setVelocities(prev => {
      const next = prev.map(r => [...r]);
      next[row][step] = Math.max(1, Math.min(127, next[row][step] + delta));
      return next;
    });
  }, []);

  const clearGrid = () => {
    setGrid(Array.from({ length: 16 }, () => new Array(64).fill(false)));
  };

  const randomize = (density = 0.3) => {
    setGrid(prev => prev.map((row, ri) =>
      row.map((_, si) => si < numSteps ? Math.random() < density : false)
    ));
  };

  return (
    <div className="step-sequencer">
      <div className="step-seq-header">
        <h3>🥁 Step Sequencer</h3>
        <div className="step-seq-controls">
          <select className="select input-sm" value={numSteps} onChange={e => setNumSteps(Number(e.target.value))}>
            <option value={16}>16 Steps</option>
            <option value={32}>32 Steps</option>
            <option value={64}>64 Steps</option>
          </select>
          <label className="step-seq-label">
            Swing
            <input type="range" min="0" max="1" step="0.05" value={swing}
              onChange={e => setSwing(parseFloat(e.target.value))} className="step-seq-range" />
            <span className="mono">{Math.round(swing * 100)}%</span>
          </label>
          <button className="btn btn-xs btn-ghost" onClick={clearGrid}>Clear</button>
          <button className="btn btn-xs btn-ghost" onClick={() => randomize()}>Random</button>
        </div>
      </div>

      <div className="step-seq-grid" style={{ gridTemplateColumns: `100px repeat(${numSteps}, 1fr)` }}>
        {/* Header row */}
        <div className="step-seq-label-cell"></div>
        {Array.from({ length: numSteps }, (_, i) => (
          <div key={i} className={`step-seq-col-header ${i % 4 === 0 ? 'bar-start' : ''}`}>
            {i + 1}
          </div>
        ))}

        {/* Drum rows */}
        {DRUM_LABELS.map((label, row) => (
          <React.Fragment key={row}>
            <div className="step-seq-row-label">{label}</div>
            {Array.from({ length: numSteps }, (_, step) => (
              <div
                key={step}
                className={`step-seq-cell ${grid[row][step] ? 'active' : ''} ${step % 4 === 0 ? 'bar-start' : ''}`}
                onClick={() => toggleStep(row, step)}
                onContextMenu={e => { e.preventDefault(); adjustVelocity(row, step, -10); }}
                onWheel={e => adjustVelocity(row, step, e.deltaY > 0 ? -5 : 5)}
                style={grid[row][step] ? { opacity: 0.3 + (velocities[row][step] / 127) * 0.7 } : undefined}
                title={`Vel: ${velocities[row][step]}`}
              />
            ))}
          </React.Fragment>
        ))}
      </div>
    </div>
  );
}
