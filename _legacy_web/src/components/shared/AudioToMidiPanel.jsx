import React, { useState, useCallback } from 'react';
import { useProjectStore } from '../../stores/projectStore';

const MODES = [
  { id: 'monophonic', label: 'Monophonic', desc: 'Single melody line — best for vocals, leads, bass' },
  { id: 'polyphonic', label: 'Polyphonic', desc: 'Chords & harmonics — best for piano, guitar, pads' },
  { id: 'stems', label: 'Full Stem Separation', desc: 'Separate to bass, keys, percussion + isolate vocals' },
];

const QUANTIZE_OPTIONS = [
  { value: 1, label: '1 bar' },
  { value: 0.5, label: '1/2' },
  { value: 0.25, label: '1/4' },
  { value: 0.125, label: '1/8' },
  { value: 0.0625, label: '1/16' },
  { value: 0.03125, label: '1/32' },
];

export default function AudioToMidiPanel({ trackId, clipId, onClose }) {
  const { convertToMidi, convertTrackToMidi, bpm } = useProjectStore();

  const [mode, setMode] = useState('monophonic');
  const [sensitivity, setSensitivity] = useState(0.5);
  const [quantizeGrid, setQuantizeGrid] = useState(0.0625); // 1/16
  const [minVelocity, setMinVelocity] = useState(10);
  const [processing, setProcessing] = useState(false);
  const [result, setResult] = useState(null);

  const handleConvert = useCallback(async () => {
    setProcessing(true);
    setResult(null);
    try {
      const options = { mode, sensitivity, quantizeGrid, bpm, minVelocity };
      let res;
      if (mode === 'stems') {
        res = await convertTrackToMidi(trackId, options);
      } else {
        res = await convertToMidi(trackId, clipId, options);
      }
      setResult(res);
    } catch (err) {
      setResult({ error: err.message });
    }
    setProcessing(false);
  }, [trackId, clipId, mode, sensitivity, quantizeGrid, bpm, minVelocity, convertToMidi, convertTrackToMidi]);

  return (
    <div className="audio-to-midi-panel">
      <div className="panel-header">
        <span>🎹 Audio to MIDI</span>
        {onClose && <button className="btn btn-xs btn-ghost" onClick={onClose}>✕</button>}
      </div>

      <div className="atm-body">
        {/* Mode Selector */}
        <div className="atm-section">
          <div className="atm-label">Conversion Mode</div>
          <div className="atm-mode-grid">
            {MODES.map(m => (
              <button
                key={m.id}
                className={`atm-mode-btn ${mode === m.id ? 'active' : ''}`}
                onClick={() => setMode(m.id)}
              >
                <span className="atm-mode-title">{m.label}</span>
                <span className="atm-mode-desc">{m.desc}</span>
              </button>
            ))}
          </div>
        </div>

        {/* Sensitivity */}
        <div className="atm-section">
          <label className="atm-label">
            Detection Sensitivity
            <span className="atm-value">{Math.round(sensitivity * 100)}%</span>
          </label>
          <input
            type="range" min={0.1} max={1} step={0.05}
            value={sensitivity}
            onChange={e => setSensitivity(parseFloat(e.target.value))}
            className="atm-slider"
          />
        </div>

        {/* Quantize Grid */}
        <div className="atm-section">
          <div className="atm-label">Quantize Grid</div>
          <div className="atm-grid-btns">
            {QUANTIZE_OPTIONS.map(q => (
              <button
                key={q.value}
                className={`btn btn-xs ${quantizeGrid === q.value ? 'btn-primary' : 'btn-ghost'}`}
                onClick={() => setQuantizeGrid(q.value)}
              >
                {q.label}
              </button>
            ))}
          </div>
        </div>

        {/* Min Velocity */}
        <div className="atm-section">
          <label className="atm-label">
            Min Velocity
            <span className="atm-value">{minVelocity}</span>
          </label>
          <input
            type="range" min={1} max={64} step={1}
            value={minVelocity}
            onChange={e => setMinVelocity(parseInt(e.target.value))}
            className="atm-slider"
          />
        </div>

        {/* Convert Button */}
        <div className="atm-actions">
          <button
            className="btn btn-primary"
            onClick={handleConvert}
            disabled={processing}
          >
            {processing ? '⏳ Converting...' : '🎹 Convert to MIDI'}
          </button>
        </div>

        {/* Results */}
        {result && !result.error && (
          <div className="atm-result success">
            <div className="atm-result-header">✅ Conversion Complete</div>
            {result.tracks && result.tracks.map((t, i) => (
              <div key={i} className="atm-result-track">
                <span className="atm-track-name">{t.name}</span>
                <span className="atm-track-notes">{t.notes?.length || 0} notes</span>
              </div>
            ))}
            {result.vocalBuffer && (
              <div className="atm-result-track vocal">
                <span className="atm-track-name">🎤 Vocal Track (isolated)</span>
                <span className="atm-track-notes">{result.vocalBuffer.duration.toFixed(1)}s</span>
              </div>
            )}
          </div>
        )}

        {result && result.error && (
          <div className="atm-result error">
            <span>❌ {result.error}</span>
          </div>
        )}
      </div>
    </div>
  );
}
