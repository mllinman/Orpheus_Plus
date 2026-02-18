import React, { useRef, useEffect } from 'react';
import { useProjectStore } from '../../stores/projectStore';
import { audioEngine } from '../../audio/AudioEngine';

export default function Playhead({ pixelsPerBeat, totalHeight }) {
  const lineRef = useRef(null);
  const animRef = useRef(null);
  const isPlaying = useProjectStore(s => s.isPlaying);
  const bpm = useProjectStore(s => s.bpm);

  useEffect(() => {
    const animate = () => {
      if (lineRef.current) {
        const currentTime = audioEngine.currentTime;
        const currentBeat = (currentTime / 60) * bpm;
        const x = currentBeat * pixelsPerBeat;
        lineRef.current.style.transform = `translateX(${x}px)`;
        lineRef.current.style.display = x > 0 || isPlaying ? 'block' : 'block';
      }
      animRef.current = requestAnimationFrame(animate);
    };
    animRef.current = requestAnimationFrame(animate);
    return () => cancelAnimationFrame(animRef.current);
  }, [isPlaying, bpm, pixelsPerBeat]);

  return (
    <div
      ref={lineRef}
      className="playhead-line"
      style={{ height: totalHeight }}
    >
      <div className="playhead-head" />
    </div>
  );
}
