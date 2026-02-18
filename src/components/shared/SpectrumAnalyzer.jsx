import React, { useRef, useEffect } from 'react';

export default function SpectrumAnalyzer({ analyzerNode, width = 300, height = 120 }) {
  const canvasRef = useRef(null);
  const rafRef = useRef(null);

  useEffect(() => {
    if (!analyzerNode || !canvasRef.current) return;

    const canvas = canvasRef.current;
    const ctx = canvas.getContext('2d');
    const bufferLength = analyzerNode.frequencyBinCount;
    const dataArray = new Uint8Array(bufferLength);

    const draw = () => {
      analyzerNode.getByteFrequencyData(dataArray);

      ctx.clearRect(0, 0, canvas.width, canvas.height);

      // Background grid
      ctx.strokeStyle = 'rgba(255,255,255,0.05)';
      ctx.lineWidth = 0.5;
      for (let i = 0; i < 5; i++) {
        const y = (i / 5) * canvas.height;
        ctx.beginPath();
        ctx.moveTo(0, y);
        ctx.lineTo(canvas.width, y);
        ctx.stroke();
      }

      // Frequency bars with gradient coloring
      const barWidth = canvas.width / bufferLength * 2.5;
      let x = 0;

      for (let i = 0; i < bufferLength; i++) {
        const barHeight = (dataArray[i] / 255) * canvas.height;
        const hue = (i / bufferLength) * 240; // blue to red
        const saturation = 80;
        const lightness = 40 + (dataArray[i] / 255) * 20;

        ctx.fillStyle = `hsla(${hue}, ${saturation}%, ${lightness}%, 0.8)`;
        ctx.fillRect(x, canvas.height - barHeight, barWidth - 1, barHeight);

        x += barWidth;
        if (x > canvas.width) break;
      }

      // Peak line
      ctx.strokeStyle = 'rgba(255,255,255,0.3)';
      ctx.lineWidth = 1;
      ctx.beginPath();
      x = 0;
      for (let i = 0; i < bufferLength; i++) {
        const y = canvas.height - (dataArray[i] / 255) * canvas.height;
        if (i === 0) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
        x += barWidth;
        if (x > canvas.width) break;
      }
      ctx.stroke();

      rafRef.current = requestAnimationFrame(draw);
    };

    draw();
    return () => { if (rafRef.current) cancelAnimationFrame(rafRef.current); };
  }, [analyzerNode]);

  return (
    <div className="spectrum-analyzer" style={{ width, height }}>
      <canvas ref={canvasRef} width={width * 2} height={height * 2}
        style={{ width: '100%', height: '100%' }} />
      <div className="spectrum-labels">
        <span>20</span><span>100</span><span>1k</span><span>5k</span><span>20k</span>
      </div>
    </div>
  );
}
