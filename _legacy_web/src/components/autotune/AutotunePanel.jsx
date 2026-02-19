import React, { useState, useEffect, useRef } from 'react';
import { PitchCorrector } from '../../audio/PitchCorrector';

const SCALES = Object.keys(PitchCorrector.SCALES);
const KEYS = PitchCorrector.KEYS;

const SCALE_LABELS = {
  chromatic: 'Chromatic',
  major: 'Major (Ionian)',
  minor: 'Natural Minor',
  harmonicMinor: 'Harmonic Minor',
  melodicMinor: 'Melodic Minor',
  dorian: 'Dorian',
  mixolydian: 'Mixolydian',
  phrygian: 'Phrygian',
  lydian: 'Lydian',
  pentatonicMajor: 'Pentatonic Major',
  pentatonicMinor: 'Pentatonic Minor',
  blues: 'Blues',
  wholeHalfDim: 'Whole-Half Dim.',
};

// Pitch to Note helper
const NOTE_NAMES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
function getNoteFromFrequency(frequency) {
  if (!frequency || frequency < 20) return { note: '--', cents: 0 };
  const noteNum = 12 * (Math.log(frequency / 440) / Math.log(2));
  const midi = Math.round(noteNum) + 69;
  const note = NOTE_NAMES[midi % 12] + (Math.floor(midi / 12) - 1);
  const cents = (noteNum - Math.round(noteNum)) * 100;
  return { note, cents };
}

export default function AutotunePanel() {
  const { selectedTrackId } = useUIStore();
  const { tracks, setTrackAutotune } = useProjectStore();
  
  const track = tracks.find(t => t.id === selectedTrackId);
  const config = track?.autotune || { 
      enabled: false, key: 'C', scale: 'chromatic', 
      speed: 0.5, amount: 1.0, humanize: 0.1, retune: 0, formant: true 
  };

  const updateParam = (key, value) => {
      if (!track) return;
      setTrackAutotune(track.id, { [key]: value });
      // Notify engine for realtime update
      audioEngine.setAutotuneParam(track.id, key, value);
  };
  
  // Local state for visualization only
  const [detectedPitch, setDetectedPitch] = useState(0);
  const [detectedNote, setDetectedNote] = useState('--');
  const [targetNote, setTargetNote] = useState('--');
  const [centsOff, setCentsOff] = useState(0);
  const [confidence, setConfidence] = useState(0);

  const animRef = useRef(null);

  if (!track) {
      return (
          <div className="autotune-panel">
              <div className="autotune-header">
                  <div className="autotune-title-row">
                      <span className="autotune-title">AUTOTUNE</span>
                  </div>
              </div>
              <div className="autotune-content" style={{ display: 'flex', alignItems: 'center', justifyContent: 'center', height: '100%', color: 'var(--text-muted)' }}>
                  Select a track to edit Autotune settings
              </div>
          </div>
      );
  }

  // Real pitch detection visualization
  useEffect(() => {
    const animate = () => {
      if (config.enabled) {
        const { detectedPitch, clarity } = audioEngine.getDetectedPitch();
        
        if (clarity > 0.8 && detectedPitch > 50) {
           const { note, cents } = getNoteFromFrequency(detectedPitch);
           setDetectedPitch(detectedPitch);
           setDetectedNote(note);
           setTargetNote(note); // Ideally this would snap to scale
           setCentsOff(cents);
           setConfidence(clarity);
        } else {
            // Decay
            setConfidence(prev => Math.max(0, prev - 0.05));
        }
      }
      animRef.current = requestAnimationFrame(animate);
    };
    animRef.current = requestAnimationFrame(animate);
    return () => cancelAnimationFrame(animRef.current);
  }, [enabled]);

  // Get active scale notes for the keyboard display
  const keyIndex = KEYS.indexOf(config.key);
  const scaleIntervals = PitchCorrector.SCALES[config.scale] || PitchCorrector.SCALES.chromatic;
  const activeNotes = scaleIntervals.map(i => (i + keyIndex) % 12);

  const speedLabel = config.speed < 0.2 ? 'Natural' : config.speed < 0.5 ? 'Moderate' : config.speed < 0.8 ? 'Fast' : 'Hard Tune';
  const speedColor = config.speed < 0.5 ? 'var(--accent-success)' : config.speed < 0.8 ? 'var(--accent-warning)' : 'var(--accent-danger)';

  return (
    <div className="autotune-panel">
      <div className="autotune-header">
        <div className="autotune-title-row">
          <span style={{ fontSize: 16 }}>🎤</span>
          <span className="autotune-title">AUTOTUNE</span>
          <button
            className={`btn btn-sm ${config.enabled ? 'active' : ''}`}
            onClick={() => updateParam('enabled', !config.enabled)}
          >
            {config.enabled ? 'ON' : 'OFF'}
          </button>
        </div>
      </div>

      <div className="autotune-content">
        {/* Pitch Display */}
        <div className="autotune-pitch-display">
          <div className="pitch-detected">
            <div className="pitch-note-large mono" style={{
              color: config.enabled ? (Math.abs(centsOff) < 10 ? 'var(--accent-success)' : 'var(--accent-warning)') : 'var(--text-muted)'
            }}>
              {config.enabled ? detectedNote : '--'}
            </div>
            <div className="pitch-freq mono">
              {config.enabled ? `${detectedPitch.toFixed(1)} Hz` : '--- Hz'}
            </div>
          </div>

          {/* Cents meter */}
          <div className="cents-meter">
            <div className="cents-label">CENTS</div>
            <div className="cents-bar">
              <div className="cents-center" />
              <div
                className="cents-indicator"
                style={{
                  left: `${50 + (config.enabled ? centsOff : 0) * 0.5}%`,
                  background: Math.abs(centsOff) < 10 ? 'var(--accent-success)' : 'var(--accent-warning)',
                }}
              />
            </div>
            <div className="cents-labels-row">
              <span>-50</span>
              <span>0</span>
              <span>+50</span>
            </div>
          </div>

          {/* Target note */}
          <div className="pitch-target">
            <span className="text-muted" style={{ fontSize: 'var(--text-xs)' }}>TARGET</span>
            <span className="pitch-target-note mono">{config.enabled ? targetNote : '--'}</span>
          </div>

          {/* Confidence bar */}
          <div className="confidence-row">
            <span className="text-muted" style={{ fontSize: 'var(--text-xs)' }}>CONFIDENCE</span>
            <div className="confidence-bar">
              <div
                className="confidence-fill"
                style={{ width: `${(config.enabled ? confidence : 0) * 100}%` }}
              />
            </div>
          </div>
        </div>

        {/* Key & Scale */}
        <div className="autotune-section">
          <div className="autotune-section-label">KEY & SCALE</div>
          <div className="key-scale-row">
            <select className="select" value={config.key} onChange={(e) => updateParam('key', e.target.value)} style={{ width: 70 }}>
              {KEYS.map(k => <option key={k} value={k}>{k}</option>)}
            </select>
            <select className="select" value={config.scale} onChange={(e) => updateParam('scale', e.target.value)} style={{ flex: 1 }}>
              {SCALES.map(s => <option key={s} value={s}>{SCALE_LABELS[s] || s}</option>)}
            </select>
          </div>

          {/* Piano keyboard showing active notes */}
          <div className="mini-keyboard">
            {KEYS.map((noteName, i) => {
              const isBlack = noteName.includes('#');
              const isActive = activeNotes.includes(i);
              const isDetected = config.enabled && detectedNote.startsWith(noteName);
              return (
                <div
                  key={noteName}
                  className={`mini-key ${isBlack ? 'black' : 'white'} ${isActive ? 'in-scale' : 'out-scale'} ${isDetected ? 'detected' : ''}`}
                  data-tooltip={noteName}
                >
                  {!isBlack && <span className="mini-key-label">{noteName}</span>}
                </div>
              );
            })}
          </div>
        </div>

        {/* Correction Speed */}
        <div className="autotune-section">
          <div className="autotune-section-label">CORRECTION SPEED</div>
          <div className="speed-control">
            <div className="speed-bar">
              <input
                type="range"
                min="0"
                max="1"
                step="0.01"
                value={config.speed}
                onChange={(e) => updateParam('speed', parseFloat(e.target.value))}
                style={{ width: '100%', accentColor: speedColor }}
              />
            </div>
            <div className="speed-labels">
              <span className="text-muted">Natural</span>
              <span className="mono" style={{ color: speedColor, fontWeight: 600, fontSize: 'var(--text-sm)' }}>{speedLabel}</span>
              <span className="text-muted">Hard</span>
            </div>
          </div>
        </div>

        {/* Amount */}
        <div className="autotune-section">
          <div className="autotune-section-label">AMOUNT</div>
          <div className="autotune-slider-row">
            <input
              type="range"
              min="0"
              max="1"
              step="0.01"
              value={config.amount}
              onChange={(e) => updateParam('amount', parseFloat(e.target.value))}
              style={{ flex: 1, accentColor: 'var(--accent-primary)' }}
            />
            <span className="mono" style={{ fontSize: 'var(--text-xs)', minWidth: 36, textAlign: 'right' }}>
              {Math.round(config.amount * 100)}%
            </span>
          </div>
        </div>

        {/* Humanize */}
        <div className="autotune-section">
          <div className="autotune-section-label">HUMANIZE</div>
          <div className="autotune-slider-row">
            <input
              type="range"
              min="0"
              max="1"
              step="0.01"
              value={config.humanize}
              onChange={(e) => updateParam('humanize', parseFloat(e.target.value))}
              style={{ flex: 1, accentColor: 'var(--accent-secondary)' }}
            />
            <span className="mono" style={{ fontSize: 'var(--text-xs)', minWidth: 36, textAlign: 'right' }}>
              {Math.round(config.humanize * 100)}%
            </span>
          </div>
        </div>

        {/* Retune */}
        <div className="autotune-section">
          <div className="autotune-section-label">RETUNE / DETUNE</div>
          <div className="autotune-slider-row">
            <input
              type="range"
              min="-100"
              max="100"
              step="1"
              value={config.retune}
              onChange={(e) => updateParam('retune', parseFloat(e.target.value))}
              style={{ flex: 1, accentColor: 'var(--accent-tertiary)' }}
            />
            <span className="mono" style={{ fontSize: 'var(--text-xs)', minWidth: 46, textAlign: 'right' }}>
              {config.retune > 0 ? '+' : ''}{config.retune} ct
            </span>
          </div>
        </div>

        {/* Toggles */}
        <div className="autotune-section">
          <div className="autotune-toggles">
            <label className="autotune-toggle">
              <input
                type="checkbox"
                checked={config.formant}
                onChange={(e) => updateParam('formant', e.target.checked)}
              />
              <span>Formant Preservation</span>
            </label>
          </div>
        </div>
      </div>
    </div>
  );
}
