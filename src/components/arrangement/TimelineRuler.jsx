import React, { useRef, useEffect } from 'react';
import { useUIStore } from '../../stores/uiStore';
import { useProjectStore } from '../../stores/projectStore';

export default function TimelineRuler({ totalBeats, pixelsPerBeat }) {
  const canvasRef = useRef(null);
  const { isLooping, loopStart, loopEnd } = useProjectStore();
  const setPlayheadPosition = useProjectStore(s => s.setPlayheadPosition);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    const parent = canvas.parentElement;
    canvas.width = parent.scrollWidth || totalBeats * pixelsPerBeat;
    canvas.height = 28;

    ctx.clearRect(0, 0, canvas.width, canvas.height);

    // Background
    ctx.fillStyle = '#12121e';
    ctx.fillRect(0, 0, canvas.width, canvas.height);

    // Loop region
    if (isLooping) {
      ctx.fillStyle = 'rgba(108, 92, 231, 0.12)';
      ctx.fillRect(loopStart * pixelsPerBeat, 0, (loopEnd - loopStart) * pixelsPerBeat, canvas.height);
      // Loop boundaries
      ctx.strokeStyle = 'rgba(108, 92, 231, 0.5)';
      ctx.lineWidth = 1;
      ctx.beginPath();
      ctx.moveTo(loopStart * pixelsPerBeat, 0);
      ctx.lineTo(loopStart * pixelsPerBeat, canvas.height);
      ctx.moveTo(loopEnd * pixelsPerBeat, 0);
      ctx.lineTo(loopEnd * pixelsPerBeat, canvas.height);
      ctx.stroke();
    }

    // Draw bars and beats
    const beatsPerBar = 4;
    for (let beat = 0; beat <= totalBeats; beat++) {
      const x = beat * pixelsPerBeat;
      const isBar = beat % beatsPerBar === 0;
      const barNum = Math.floor(beat / beatsPerBar) + 1;

      if (isBar) {
        // Bar line
        ctx.strokeStyle = '#3a3a55';
        ctx.lineWidth = 1;
        ctx.beginPath();
        ctx.moveTo(x, 0);
        ctx.lineTo(x, canvas.height);
        ctx.stroke();

        // Bar number
        ctx.fillStyle = '#a0a0b8';
        ctx.font = '11px Inter, sans-serif';
        ctx.fillText(barNum.toString(), x + 4, 12);
      } else if (pixelsPerBeat >= 20) {
        // Beat tick
        ctx.strokeStyle = '#1e1e32';
        ctx.lineWidth = 1;
        ctx.beginPath();
        ctx.moveTo(x, 18);
        ctx.lineTo(x, canvas.height);
        ctx.stroke();
      }

      // Sub-beats (16th notes)
      if (pixelsPerBeat >= 50) {
        for (let sub = 1; sub < 4; sub++) {
          const sx = x + (sub * pixelsPerBeat / 4);
          ctx.strokeStyle = '#151524';
          ctx.lineWidth = 0.5;
          ctx.beginPath();
          ctx.moveTo(sx, 22);
          ctx.lineTo(sx, canvas.height);
          ctx.stroke();
        }
      }
    }

    // Bottom border
    ctx.strokeStyle = '#2a2a42';
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.moveTo(0, canvas.height - 0.5);
    ctx.lineTo(canvas.width, canvas.height - 0.5);
    ctx.stroke();

  }, [totalBeats, pixelsPerBeat, isLooping, loopStart, loopEnd]);

  // Handle Scrubbing
  useEffect(() => {
    const onMove = (e) => {
      if (!isDragging.current) return;
      handleScrub(e);
    };
    const onUp = () => {
      if (isDragging.current) {
        isDragging.current = false;
        document.body.style.cursor = 'default';
      }
    };
    window.addEventListener('mousemove', onMove);
    window.addEventListener('mouseup', onUp);
    return () => {
      window.removeEventListener('mousemove', onMove);
      window.removeEventListener('mouseup', onUp);
    };
  }, [pixelsPerBeat]);

  const isDragging = useRef(false);

  const handleScrub = (e) => {
    if (!canvasRef.current) return;
    const rect = canvasRef.current.getBoundingClientRect();
    const x = e.clientX - rect.left + (canvasRef.current.parentElement.scrollLeft || 0);
    const beat = Math.max(0, x / pixelsPerBeat);
    const time = (beat / useProjectStore.getState().bpm) * 60;
    setPlayheadPosition(time);
  };

  const handleMouseDown = (e) => {
    isDragging.current = true;
    document.body.style.cursor = 'ew-resize';
    handleScrub(e);
  };

  return (
    <div className="timeline-ruler" onMouseDown={handleMouseDown}>
      <canvas ref={canvasRef} className="timeline-canvas" />
    </div>
  );
}
