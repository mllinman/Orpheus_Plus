import React, { useState, useEffect, useRef } from 'react';
import { MASTERING_PRESETS } from '../../audio/MasteringChain';

export default function MasteringPanel() {
  const [enabled, setEnabled] = useState(true);
  const [abBypass, setAbBypass] = useState(false);
  const [preset, setPreset] = useState('Streaming');
  const [lufs, setLufs] = useState(-14.2);
  const [gainReduction, setGainReduction] = useState({ low: 0, mid: 0, high: 0, limiter: 0 });

  // Parameters
  const [inputTrim, setInputTrim] = useState(0);
  const [outputGain, setOutputGain] = useState(0);

  // Pre-EQ
  const [lowShelf, setLowShelf] = useState(1);
  const [midGain, setMidGain] = useState(0);
  const [highShelf, setHighShelf] = useState(0.5);
  const [lowCut, setLowCut] = useState(30);
  
  // Saturator (New)
  const [satDrive, setSatDrive] = useState(0); // dB

  // Multiband comp
  const [lowThresh, setLowThresh] = useState(-18);
  const [midThresh, setMidThresh] = useState(-20);
  const [highThresh, setHighThresh] = useState(-22);

  // Stereo + Limiter
  const [stereoWidth, setStereoWidth] = useState(40);
  const [limiterThresh, setLimiterThresh] = useState(-1);
  const [limiterCeiling, setLimiterCeiling] = useState(-0.1);

  // Post-EQ
  const [airGain, setAirGain] = useState(1.5);
  const [presenceGain, setPresenceGain] = useState(0.5);

  const animRef = useRef(null);

  // Animate LUFS and gain reduction
  useEffect(() => {
    const animate = () => {
      if (enabled && !abBypass) {
        const meters = audioEngine.getMasteringMeters();
        setLufs(meters.lufs);
        setGainReduction({
          low: meters.gainReduction, 
          mid: meters.gainReduction,
          high: meters.gainReduction,
          limiter: meters.gainReduction * 1.5,
        });
      }
      animRef.current = requestAnimationFrame(animate);
    };
    animRef.current = requestAnimationFrame(animate);
    return () => cancelAnimationFrame(animRef.current);
  }, [enabled, abBypass]);

  const handlePresetChange = (name) => {
    setPreset(name);
    // Apply to Engine
    audioEngine.setMasteringParam('preset', null, name);
    
    // Update local state from preset definition
    const p = MASTERING_PRESETS[name];
    if (p) {
      setLowShelf(p.preEQ.lowShelf);
      setMidGain(p.preEQ.mid);
      setHighShelf(p.preEQ.highShelf);
      setLowCut(p.preEQ.lowCut);
      setLowThresh(p.comp.lowThresh);
      setMidThresh(p.comp.midThresh);
      setHighThresh(p.comp.highThresh);
      setStereoWidth(p.stereoWidth * 100);
      setAirGain(p.postEQ.air);
      setPresenceGain(p.postEQ.presence);
      setLimiterThresh(p.limiter.threshold);
      setLimiterCeiling(p.limiter.ceiling);
      setSatDrive(p.saturation?.drive || 0);
    }
  };

  const lufsColor = lufs > -9 ? '#ff6b6b' : lufs > -14 ? '#fdcb6e' : '#00b894';

  return (
    <div className="mastering-panel">
      <div className="mastering-header">
        <div className="mastering-title-row">
          <span style={{ fontSize: 16 }}>🎛</span>
          <span className="mastering-title">MASTERING CHAIN</span>
          <button
            className={`btn btn-sm ${enabled ? 'active' : ''}`}
            onClick={() => { setEnabled(!enabled); /* TODO: Bypass logic */ }}
          >
            {enabled ? 'ON' : 'BYPASS'}
          </button>
        </div>
      </div>

      <div className="mastering-content">
        {/* Preset Selector */}
        <div className="mastering-section">
          <div className="mastering-section-label">PRESET</div>
          <select
            className="select"
            value={preset}
            onChange={(e) => handlePresetChange(e.target.value)}
            style={{ width: '100%' }}
          >
            {Object.entries(MASTERING_PRESETS).map(([key, p]) => (
              <option key={key} value={key}>{p.name}</option>
            ))}
          </select>
        </div>

        {/* LUFS Meter */}
        <div className="mastering-section lufs-section">
          <div className="mastering-section-label">LOUDNESS</div>
          <div className="lufs-display">
            <div className="lufs-value mono" style={{ color: lufsColor }}>
              {lufs.toFixed(1)}
            </div>
            <div className="lufs-unit">LUFS</div>
          </div>
          <div className="lufs-bar-container">
            <div className="lufs-bar">
              <div
                className="lufs-bar-fill"
                style={{
                  width: `${Math.max(0, Math.min(100, ((lufs + 40) / 40) * 100))}%`,
                  background: lufsColor,
                }}
              />
            </div>
            <div className="lufs-markers">
              <span>-40</span>
              <span>-14</span>
              <span>0</span>
            </div>
          </div>
          <div className="lufs-target mono" style={{ fontSize: 'var(--text-xs)', color: 'var(--text-muted)' }}>
            Target: {MASTERING_PRESETS[preset]?.target}
          </div>
        </div>

        {/* Input / Output Trim */}
        <div className="mastering-section">
          <div className="mastering-section-label">GAIN STAGING</div>
          <div className="mastering-knob-row">
            <KnobControl 
                label="INPUT" value={inputTrim} min={-12} max={12} unit="dB" 
                onChange={(v) => { setInputTrim(v); audioEngine.masterGain.gain.value = Math.pow(10, v/20); }} 
            />
             <KnobControl 
                label="DRIVE" value={satDrive} min={0} max={24} unit="dB" 
                onChange={(v) => { setSatDrive(v); audioEngine.setMasteringParam('saturator', 'drive', Math.pow(10, v/20)); }} 
                step={0.5}
            />
            <KnobControl 
                label="OUTPUT" value={outputGain} min={-12} max={12} unit="dB" 
                onChange={(v) => { setOutputGain(v); audioEngine.setMasteringParam('output', 'gain', v); }} 
            />
          </div>
        </div>

        {/* Pre-EQ */}
        <div className="mastering-section">
          <div className="mastering-section-label">EQUALIZER</div>
          <div className="mastering-knob-row">
            <KnobControl label="LOW CUT" value={lowCut} min={20} max={150} unit="Hz" onChange={(v) => { setLowCut(v); audioEngine.setMasteringParam('preEQ', 'lowCut', v); }} step={5} />
            <KnobControl label="LOW" value={lowShelf} min={-6} max={6} unit="dB" onChange={(v) => { setLowShelf(v); audioEngine.setMasteringParam('preEQ', 'lowShelf', v); }} />
            <KnobControl label="MID" value={midGain} min={-6} max={6} unit="dB" onChange={(v) => { setMidGain(v); audioEngine.setMasteringParam('preEQ', 'midGain', v); }} />
            <KnobControl label="HIGH" value={highShelf} min={-6} max={6} unit="dB" onChange={(v) => { setHighShelf(v); audioEngine.setMasteringParam('preEQ', 'highShelf', v); }} />
          </div>
        </div>

        {/* Multiband Compressor */}
        <div className="mastering-section">
          <div className="mastering-section-label">MULTIBAND COMP</div>
          <div className="mastering-knob-row">
            <CompBand label="LOW" threshold={lowThresh} reduction={gainReduction.low} onThreshChange={(v) => { setLowThresh(v); audioEngine.setMasteringParam('comp', 'lowThreshold', v); }} />
            <CompBand label="MID" threshold={midThresh} reduction={gainReduction.mid} onThreshChange={(v) => { setMidThresh(v); audioEngine.setMasteringParam('comp', 'midThreshold', v); }} />
            <CompBand label="HIGH" threshold={highThresh} reduction={gainReduction.high} onThreshChange={(v) => { setHighThresh(v); audioEngine.setMasteringParam('comp', 'highThreshold', v); }} />
          </div>
        </div>

        {/* Stereo Width + Post-EQ */}
        <div className="mastering-section">
          <div className="mastering-section-label">IMAGING</div>
          <div className="mastering-knob-row">
            <KnobControl label="WIDTH" value={stereoWidth} min={0} max={200} unit="%" onChange={(v) => { setStereoWidth(v); audioEngine.setMasteringParam('width', 'amount', v); }} step={1} />
            <KnobControl label="AIR" value={airGain} min={0} max={10} unit="dB" onChange={(v) => { setAirGain(v); audioEngine.setMasteringParam('postEQ', 'air', v); }} />
            <KnobControl label="PRESENCE" value={presenceGain} min={0} max={6} unit="dB" onChange={(v) => { setPresenceGain(v); audioEngine.setMasteringParam('postEQ', 'presence', v); }} />
          </div>
        </div>

        {/* Limiter */}
        <div className="mastering-section">
          <div className="mastering-section-label">LIMITER</div>
          <div className="mastering-knob-row">
            <KnobControl label="THRESH" value={limiterThresh} min={-12} max={0} unit="dB" onChange={(v) => { setLimiterThresh(v); audioEngine.setMasteringParam('limiter', 'threshold', v); }} />
            <KnobControl label="CEILING" value={limiterCeiling} min={-3} max={0} unit="dB" onChange={(v) => { setLimiterCeiling(v); audioEngine.setMasteringParam('limiter', 'ceiling', v); }} />
            <GRMeter label="GR" value={gainReduction.limiter} />
          </div>
        </div>
      </div>
    </div>
  );
}

// ── Knob Control ──
function KnobControl({ label, value, min, max, unit, onChange, step = 0.1 }) {
  const angle = ((value - min) / (max - min)) * 270 - 135;

  return (
    <div className="mastering-knob">
      <div className="knob-visual">
        <svg width="36" height="36" viewBox="0 0 36 36">
          {/* Track */}
          <circle cx="18" cy="18" r="14" fill="none" stroke="rgba(255,255,255,0.08)" strokeWidth="3"
            strokeDasharray="66" strokeDashoffset="16.5"
            transform="rotate(135, 18, 18)" strokeLinecap="round" />
          {/* Value arc */}
          <circle cx="18" cy="18" r="14" fill="none" stroke="var(--accent-primary)" strokeWidth="3"
            strokeDasharray={`${((value - min) / (max - min)) * 66} 66`}
            strokeDashoffset="16.5"
            transform="rotate(135, 18, 18)" strokeLinecap="round" />
          {/* Indicator */}
          <line
            x1="18" y1="18"
            x2={18 + Math.cos((angle * Math.PI) / 180) * 10}
            y2={18 + Math.sin((angle * Math.PI) / 180) * 10}
            stroke="var(--text-primary)" strokeWidth="2" strokeLinecap="round"
          />
          <circle cx="18" cy="18" r="5" fill="var(--bg-elevated)" stroke="var(--border-default)" strokeWidth="1" />
        </svg>
      </div>
      <input
        type="range"
        min={min}
        max={max}
        step={step}
        value={value}
        onChange={(e) => onChange(parseFloat(e.target.value))}
        className="knob-range"
      />
      <div className="knob-label">{label}</div>
      <div className="knob-value mono">{typeof value === 'number' ? value.toFixed(1) : value} {unit}</div>
    </div>
  );
}

// ── Compressor Band ──
function CompBand({ label, threshold, reduction, onThreshChange }) {
  const grHeight = Math.min(100, Math.abs(reduction) * 15);

  return (
    <div className="comp-band">
      <div className="comp-band-label">{label}</div>
      <div className="comp-gr-bar">
        <div className="comp-gr-fill" style={{ height: `${grHeight}%`, background: 'var(--accent-warning)' }} />
      </div>
      <input
        type="range"
        min={-30}
        max={0}
        step={0.5}
        value={threshold}
        onChange={(e) => onThreshChange(parseFloat(e.target.value))}
        className="knob-range"
      />
      <div className="knob-value mono">{threshold.toFixed(0)} dB</div>
      <div className="knob-value mono" style={{ color: 'var(--accent-warning)' }}>{reduction.toFixed(1)} dB</div>
    </div>
  );
}

// ── Gain Reduction Meter ──
function GRMeter({ label, value }) {
  const grHeight = Math.min(100, Math.abs(value) * 20);
  return (
    <div className="comp-band">
      <div className="comp-band-label">{label}</div>
      <div className="comp-gr-bar large">
        <div className="comp-gr-fill" style={{ height: `${grHeight}%`, background: 'var(--accent-danger)' }} />
      </div>
      <div className="knob-value mono" style={{ color: 'var(--accent-danger)' }}>{value.toFixed(1)} dB</div>
    </div>
  );
}
