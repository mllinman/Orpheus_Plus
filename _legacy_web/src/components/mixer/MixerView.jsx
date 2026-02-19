import React, { useEffect, useRef, useState } from 'react';
import { useProjectStore } from '../../stores/projectStore';
import { audioEngine } from '../../audio/AudioEngine';
import ChannelStrip from './ChannelStrip';

export default function MixerView() {
  const tracks = useProjectStore(s => s.tracks);
  const masterVolume = useProjectStore(s => s.masterVolume);
  const setMasterVolume = useProjectStore(s => s.setMasterVolume);
  const [masterLevel, setMasterLevel] = useState(0);
  const animRef = useRef(null);

  // Smoothed master VU using real analyser when available
  useEffect(() => {
    let smoothed = 0;
    const animate = () => {
      const isPlaying = useProjectStore.getState().isPlaying;
      if (isPlaying && audioEngine.analyser) {
        const data = audioEngine.getMasterLevel();
        const target = Math.min(1, data.rms * 3 + data.peak * 0.3);
        smoothed = smoothed * 0.8 + target * 0.2;
      } else if (isPlaying) {
        smoothed = smoothed * 0.85 + (masterVolume * 0.6) * 0.15;
      } else {
        smoothed = smoothed * 0.9;
      }
      setMasterLevel(smoothed);
      animRef.current = requestAnimationFrame(animate);
    };
    animRef.current = requestAnimationFrame(animate);
    return () => cancelAnimationFrame(animRef.current);
  }, [masterVolume]);

  const segments = 24;
  const lit = Math.floor(masterLevel * segments);

  return (
    <div className="mixer-view">
      <div className="mixer-header">
        <span>MIXER</span>
      </div>
      <div className="mixer-channels">
        {tracks.map(track => (
          <ChannelStrip key={track.id} track={track} />
        ))}
        {/* Master Channel */}
        <div className="channel-strip master-strip">
          <div className="channel-label">
            <span className="channel-name">MASTER</span>
          </div>
          <div className="channel-inserts">
            <div className="insert-slot empty">+ Insert</div>
          </div>
          <div className="channel-sends" />
          <div className="channel-fader-area">
            <div className="vu-meter">
              <div className="vu-channel">
                {Array.from({ length: segments }).map((_, i) => {
                  const idx = segments - 1 - i;
                  const isLit = idx < lit;
                  let color = '#00b894';
                  if (idx >= segments * 0.85) color = '#ff6b6b';
                  else if (idx >= segments * 0.7) color = '#fdcb6e';
                  return (
                    <div
                      key={i}
                      className="vu-segment"
                      style={{
                        background: isLit ? color : 'rgba(255,255,255,0.05)',
                        boxShadow: isLit ? `0 0 4px ${color}50` : 'none',
                      }}
                    />
                  );
                })}
              </div>
            </div>
            <div className="fader-container">
              <input
                type="range"
                className="channel-fader"
                min="0"
                max="1"
                step="0.005"
                value={masterVolume}
                onChange={(e) => {
                  const v = parseFloat(e.target.value);
                  setMasterVolume(v);
                  audioEngine.setMasterVolume(v);
                }}
                orient="vertical"
              />
              <div className="fader-db mono">
                {masterVolume > 0 ? (20 * Math.log10(masterVolume)).toFixed(1) : '-∞'}
              </div>
            </div>
          </div>
          <div className="channel-buttons">
            <span className="led on" />
          </div>
        </div>
      </div>
    </div>
  );
}
