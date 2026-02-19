import React, { useState } from 'react';
import { useProjectStore } from '../../stores/projectStore';

const DEFAULT_EQ = { lowGain: 0, midGain: 0, highGain: 0, lowFreq: 200, midFreq: 1000, highFreq: 5000 };
const DEFAULT_COMP = { threshold: -20, ratio: 4, attack: 10, release: 100, makeupGain: 0 };
const DEFAULT_SAT = { drive: 0, mix: 50 };
const DEFAULT_GATE = { threshold: -40, attack: 1, release: 50 };

function KnobParam({ label, value, min, max, step = 1, unit = '', onChange }) {
  return (
    <div className="knob-wrapper">
      <input
        type="range" min={min} max={max} step={step} value={value}
        onChange={e => onChange(parseFloat(e.target.value))}
        style={{ width: 50, accentColor: 'var(--accent-primary)' }}
      />
      <span className="knob-label">{label}</span>
      <span className="knob-label mono">{typeof value === 'number' ? value.toFixed(step < 1 ? 1 : 0) : value}{unit}</span>
    </div>
  );
}

export default function ChannelStrip({ trackId }) {
  const { tracks, updateTrack } = useProjectStore();
  const track = tracks.find(t => t.id === trackId);

  const [eq, setEq] = useState(track?.channelStrip?.eq || DEFAULT_EQ);
  const [comp, setComp] = useState(track?.channelStrip?.comp || DEFAULT_COMP);
  const [sat, setSat] = useState(track?.channelStrip?.sat || DEFAULT_SAT);
  const [gate, setGate] = useState(track?.channelStrip?.gate || DEFAULT_GATE);
  const [bypassed, setBypassed] = useState({ eq: false, comp: false, sat: false, gate: false });

  const save = (section, data) => {
    const strip = { ...track?.channelStrip, [section]: data };
    updateTrack(trackId, { channelStrip: strip });
  };

  if (!track) return <div className="channel-strip">No track selected</div>;

  return (
    <div className="channel-strip">
      <div className="panel-header" style={{ margin: '-8px -8px 0', borderRadius: 'var(--radius-sm) var(--radius-sm) 0 0' }}>
        <span>🎛 {track.name} — Channel Strip</span>
      </div>

      {/* EQ Section */}
      <div className={`channel-strip-section ${bypassed.eq ? 'bypassed' : ''}`}>
        <div className="channel-strip-label" style={{ display: 'flex', justifyContent: 'space-between' }}>
          <span>EQ</span>
          <button className="btn btn-xs btn-ghost" onClick={() => setBypassed(b => ({ ...b, eq: !b.eq }))}>
            {bypassed.eq ? 'Off' : 'On'}
          </button>
        </div>
        <div className="channel-strip-knob-row">
          <KnobParam label="Low" value={eq.lowGain} min={-12} max={12} step={0.5} unit="dB"
            onChange={v => { setEq(e => ({ ...e, lowGain: v })); save('eq', { ...eq, lowGain: v }); }} />
          <KnobParam label="Mid" value={eq.midGain} min={-12} max={12} step={0.5} unit="dB"
            onChange={v => { setEq(e => ({ ...e, midGain: v })); save('eq', { ...eq, midGain: v }); }} />
          <KnobParam label="High" value={eq.highGain} min={-12} max={12} step={0.5} unit="dB"
            onChange={v => { setEq(e => ({ ...e, highGain: v })); save('eq', { ...eq, highGain: v }); }} />
        </div>
      </div>

      {/* Compressor Section */}
      <div className={`channel-strip-section ${bypassed.comp ? 'bypassed' : ''}`}>
        <div className="channel-strip-label" style={{ display: 'flex', justifyContent: 'space-between' }}>
          <span>COMPRESSOR</span>
          <button className="btn btn-xs btn-ghost" onClick={() => setBypassed(b => ({ ...b, comp: !b.comp }))}>
            {bypassed.comp ? 'Off' : 'On'}
          </button>
        </div>
        <div className="channel-strip-knob-row">
          <KnobParam label="Thresh" value={comp.threshold} min={-60} max={0} unit="dB"
            onChange={v => { setComp(c => ({ ...c, threshold: v })); save('comp', { ...comp, threshold: v }); }} />
          <KnobParam label="Ratio" value={comp.ratio} min={1} max={20}
            onChange={v => { setComp(c => ({ ...c, ratio: v })); save('comp', { ...comp, ratio: v }); }} />
          <KnobParam label="Attack" value={comp.attack} min={0.1} max={100} step={0.1} unit="ms"
            onChange={v => { setComp(c => ({ ...c, attack: v })); save('comp', { ...comp, attack: v }); }} />
          <KnobParam label="Release" value={comp.release} min={10} max={1000} unit="ms"
            onChange={v => { setComp(c => ({ ...c, release: v })); save('comp', { ...comp, release: v }); }} />
        </div>
      </div>

      {/* Saturation Section */}
      <div className={`channel-strip-section ${bypassed.sat ? 'bypassed' : ''}`}>
        <div className="channel-strip-label" style={{ display: 'flex', justifyContent: 'space-between' }}>
          <span>SATURATION</span>
          <button className="btn btn-xs btn-ghost" onClick={() => setBypassed(b => ({ ...b, sat: !b.sat }))}>
            {bypassed.sat ? 'Off' : 'On'}
          </button>
        </div>
        <div className="channel-strip-knob-row">
          <KnobParam label="Drive" value={sat.drive} min={0} max={100} unit="%"
            onChange={v => { setSat(s => ({ ...s, drive: v })); save('sat', { ...sat, drive: v }); }} />
          <KnobParam label="Mix" value={sat.mix} min={0} max={100} unit="%"
            onChange={v => { setSat(s => ({ ...s, mix: v })); save('sat', { ...sat, mix: v }); }} />
        </div>
      </div>

      {/* Gate Section */}
      <div className={`channel-strip-section ${bypassed.gate ? 'bypassed' : ''}`}>
        <div className="channel-strip-label" style={{ display: 'flex', justifyContent: 'space-between' }}>
          <span>GATE</span>
          <button className="btn btn-xs btn-ghost" onClick={() => setBypassed(b => ({ ...b, gate: !b.gate }))}>
            {bypassed.gate ? 'Off' : 'On'}
          </button>
        </div>
        <div className="channel-strip-knob-row">
          <KnobParam label="Thresh" value={gate.threshold} min={-80} max={0} unit="dB"
            onChange={v => { setGate(g => ({ ...g, threshold: v })); save('gate', { ...gate, threshold: v }); }} />
          <KnobParam label="Attack" value={gate.attack} min={0.1} max={50} step={0.1} unit="ms"
            onChange={v => { setGate(g => ({ ...g, attack: v })); save('gate', { ...gate, attack: v }); }} />
          <KnobParam label="Release" value={gate.release} min={5} max={500} unit="ms"
            onChange={v => { setGate(g => ({ ...g, release: v })); save('gate', { ...gate, release: v }); }} />
        </div>
      </div>
    </div>
  );
}
