import React, { useState, useCallback } from 'react';
import { useProjectStore } from '../../stores/projectStore';

export default function AudioCleanerPanel({ trackId, clipId, onClose }) {
  const { cleanAudioClip } = useProjectStore();

  const [options, setOptions] = useState({
    popRemoval: true,
    decrackle: true,
    artifactSmoothing: true,
    humanize: false,
    dcOffset: true,
    crossfadeRepair: true,
    strength: 0.5,
  });
  const [processing, setProcessing] = useState(false);
  const [result, setResult] = useState(null);

  const handleClean = useCallback(async () => {
    setProcessing(true);
    setResult(null);
    try {
      const stats = await cleanAudioClip(trackId, clipId, options);
      setResult(stats);
    } catch (err) {
      setResult({ error: err.message });
    }
    setProcessing(false);
  }, [trackId, clipId, options, cleanAudioClip]);

  const toggle = (key) => setOptions(o => ({ ...o, [key]: !o[key] }));

  return (
    <div className="audio-cleaner-panel">
      <div className="panel-header">
        <span>🧹 Audio Cleanup</span>
        {onClose && <button className="btn btn-xs btn-ghost" onClick={onClose}>✕</button>}
      </div>

      <div className="cleaner-body">
        {/* Strength Slider */}
        <div className="cleaner-section">
          <label className="cleaner-label">
            Cleanup Strength
            <span className="cleaner-value">
              {options.strength < 0.33 ? 'Light' : options.strength < 0.66 ? 'Medium' : 'Aggressive'}
            </span>
          </label>
          <input
            type="range" min={0} max={1} step={0.01}
            value={options.strength}
            onChange={e => setOptions(o => ({ ...o, strength: parseFloat(e.target.value) }))}
            className="cleaner-slider"
          />
        </div>

        {/* Toggle Switches */}
        <div className="cleaner-section">
          <div className="cleaner-label">Processing Modules</div>
          {[
            { key: 'popRemoval', label: '🔇 Pop & Click Removal', desc: 'Detect and interpolate amplitude spikes' },
            { key: 'decrackle', label: '✨ Decrackle', desc: 'Median filter for micro-discontinuities' },
            { key: 'artifactSmoothing', label: '🌊 Artifact Smoothing', desc: 'Spectral smoothing for digital artifacts' },
            { key: 'crossfadeRepair', label: '🔗 Crossfade Repair', desc: 'Fix zero-crossing discontinuities' },
            { key: 'dcOffset', label: '⚡ DC Offset Removal', desc: 'Center waveform at zero' },
            { key: 'humanize', label: '🎭 Humanize', desc: 'Add subtle micro-variations' },
          ].map(({ key, label, desc }) => (
            <div key={key} className="cleaner-toggle-row">
              <div className="cleaner-toggle-info">
                <span className="cleaner-toggle-label">{label}</span>
                <span className="cleaner-toggle-desc">{desc}</span>
              </div>
              <button
                className={`cleaner-toggle-btn ${options[key] ? 'active' : ''}`}
                onClick={() => toggle(key)}
              >
                {options[key] ? 'ON' : 'OFF'}
              </button>
            </div>
          ))}
        </div>

        {/* Action Buttons */}
        <div className="cleaner-actions">
          <button
            className="btn btn-primary"
            onClick={handleClean}
            disabled={processing}
          >
            {processing ? '⏳ Processing...' : '🧹 Clean Selected Clip'}
          </button>
        </div>

        {/* Results */}
        {result && (
          <div className={`cleaner-result ${result.error ? 'error' : 'success'}`}>
            {result.error ? (
              <span>❌ {result.error}</span>
            ) : (
              <div className="cleaner-stats">
                <span className="stat">✅ Cleanup Complete</span>
                {result.popsRemoved > 0 && <span className="stat">🔇 {result.popsRemoved} pops removed</span>}
                {result.cracklesFixed > 0 && <span className="stat">✨ {result.cracklesFixed} crackles fixed</span>}
                {result.artifactsSmoothed > 0 && <span className="stat">🌊 {result.artifactsSmoothed} artifacts smoothed</span>}
                {result.dcCorrected && <span className="stat">⚡ DC offset corrected</span>}
              </div>
            )}
          </div>
        )}
      </div>
    </div>
  );
}
