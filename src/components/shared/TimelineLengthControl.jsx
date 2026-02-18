import React, { useState, useMemo } from 'react';
import { useProjectStore } from '../../stores/projectStore';

export default function TimelineLengthControl() {
  const { bpm, timeSignature, tracks, setTimelineLength, stretchTimeline, trimTimeline } = useProjectStore();
  const [stretchFactor, setStretchFactor] = useState(1);

  const beatsPerBar = timeSignature?.[0] || 4;

  // Calculate current timeline length from clips
  const currentLength = useMemo(() => {
    let maxBeat = 16; // Minimum 4 bars
    tracks.forEach(t => {
      t.clips?.forEach(c => {
        const end = (c.startBeat || 0) + (c.lengthBeats || 0);
        if (end > maxBeat) maxBeat = end;
      });
    });
    return Math.ceil(maxBeat / beatsPerBar) * beatsPerBar;
  }, [tracks, beatsPerBar]);

  const totalBars = Math.ceil(currentLength / beatsPerBar);
  const totalSeconds = (currentLength / bpm) * 60;
  const minutes = Math.floor(totalSeconds / 60);
  const seconds = Math.floor(totalSeconds % 60);

  const adjustBars = (delta) => {
    const newBars = Math.max(1, totalBars + delta);
    setTimelineLength(newBars * beatsPerBar);
  };

  const handleStretch = () => {
    if (stretchFactor !== 1) {
      stretchTimeline(stretchFactor);
      setStretchFactor(1);
    }
  };

  const handleTrimToContent = () => {
    let minBeat = Infinity, maxBeat = 0;
    tracks.forEach(t => {
      t.clips?.forEach(c => {
        minBeat = Math.min(minBeat, c.startBeat || 0);
        maxBeat = Math.max(maxBeat, (c.startBeat || 0) + (c.lengthBeats || 0));
      });
    });
    if (minBeat < Infinity) {
      trimTimeline(0, Math.ceil(maxBeat / beatsPerBar) * beatsPerBar);
    }
  };

  return (
    <div className="timeline-length-control">
      {/* Bar count with ± */}
      <div className="tlc-group">
        <button className="tlc-btn" onClick={() => adjustBars(-4)} title="Remove 4 bars">−4</button>
        <button className="tlc-btn" onClick={() => adjustBars(-1)} title="Remove 1 bar">−</button>
        <div className="tlc-display">
          <span className="tlc-bars">{totalBars}</span>
          <span className="tlc-unit">bars</span>
        </div>
        <button className="tlc-btn" onClick={() => adjustBars(1)} title="Add 1 bar">+</button>
        <button className="tlc-btn" onClick={() => adjustBars(4)} title="Add 4 bars">+4</button>
      </div>

      {/* Duration display */}
      <div className="tlc-duration">
        {minutes}:{seconds.toString().padStart(2, '0')}
      </div>

      {/* Stretch factor */}
      <div className="tlc-group">
        <span className="tlc-label">Stretch</span>
        <input
          type="range" min={0.25} max={4} step={0.05}
          value={stretchFactor}
          onChange={e => setStretchFactor(parseFloat(e.target.value))}
          className="tlc-slider"
          title={`${stretchFactor.toFixed(2)}×`}
        />
        <span className="tlc-value">{stretchFactor.toFixed(2)}×</span>
        <button
          className="btn btn-xs btn-ghost"
          onClick={handleStretch}
          disabled={stretchFactor === 1}
          title="Apply stretch"
        >
          ✓
        </button>
      </div>

      {/* Trim */}
      <button className="btn btn-xs btn-ghost" onClick={handleTrimToContent} title="Trim to content">
        ✂ Trim
      </button>
    </div>
  );
}
