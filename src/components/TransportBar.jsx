import React, { useState, useCallback, useRef, useEffect } from 'react';
import { useProjectStore } from '../stores/projectStore';
import { audioEngine } from '../audio/AudioEngine';
import { formatBarsBeats, formatTime } from '../utils/helpers';

export default function TransportBar() {
  const {
    isPlaying, isRecording, isLooping, bpm, timeSignature,
    masterVolume, playheadPosition,
    setPlaying, setRecording, toggleLoop, setBpm, setMasterVolume, setPlayheadPosition
  } = useProjectStore();

  const [displayTime, setDisplayTime] = useState(0);
  const [showTimecode, setShowTimecode] = useState(false);
  const [editingBpm, setEditingBpm] = useState(false);
  const [bpmInput, setBpmInput] = useState(String(bpm));
  const bpmRef = useRef(null);
  const animRef = useRef(null);

  useEffect(() => {
    const animate = () => {
      if (isPlaying) {
        setDisplayTime(audioEngine.currentTime);
      }
      animRef.current = requestAnimationFrame(animate);
    };
    animRef.current = requestAnimationFrame(animate);
    return () => cancelAnimationFrame(animRef.current);
  }, [isPlaying]);

  const handlePlay = useCallback(async () => {
    if (isPlaying) {
      audioEngine.pause();
      setPlaying(false);
    } else {
      await audioEngine.init();
      audioEngine.setBPM(bpm);
      audioEngine.play();
      setPlaying(true);
    }
  }, [isPlaying, bpm, setPlaying]);

  const handleStop = useCallback(() => {
    audioEngine.stop();
    setPlaying(false);
    setRecording(false);
    setDisplayTime(0);
    setPlayheadPosition(0);
  }, [setPlaying, setRecording, setPlayheadPosition]);

  const handleRecord = useCallback(async () => {
    await audioEngine.init();
    audioEngine.toggleRecord();
    setRecording(audioEngine.isRecording);
    if (!isPlaying && audioEngine.isRecording) {
      audioEngine.play();
      setPlaying(true);
    }
  }, [isPlaying, setRecording, setPlaying]);

  const handleLoop = useCallback(() => {
    toggleLoop();
    audioEngine.toggleLoop();
  }, [toggleLoop]);

  const handleBpmSubmit = () => {
    const val = parseInt(bpmInput);
    if (val >= 20 && val <= 300) {
      setBpm(val);
      audioEngine.setBPM(val);
    }
    setEditingBpm(false);
  };

  const handleBpmWheel = (e) => {
    e.preventDefault();
    const delta = e.deltaY > 0 ? -1 : 1;
    const newBpm = Math.max(20, Math.min(300, bpm + delta));
    setBpm(newBpm);
    audioEngine.setBPM(newBpm);
    setBpmInput(String(newBpm));
  };

  const bbt = formatBarsBeats(displayTime, bpm, timeSignature);
  const tc = formatTime(displayTime);

  return (
    <div className="transport-bar">
      <div className="transport-left">
        <div className="transport-position" onClick={() => setShowTimecode(!showTimecode)}>
          <div className="position-label">{showTimecode ? 'TIMECODE' : 'BARS'}</div>
          <div className="position-value mono">{showTimecode ? tc : bbt}</div>
        </div>
      </div>

      <div className="transport-center">
        <button
          className={`transport-btn ${isLooping ? 'active' : ''}`}
          onClick={handleLoop}
          data-tooltip="Loop (L)"
        >
          ⟳
        </button>
        <button className="transport-btn" onClick={handleStop} data-tooltip="Stop">
          ■
        </button>
        <button
          className={`transport-btn play-btn ${isPlaying ? 'playing' : ''}`}
          onClick={handlePlay}
          data-tooltip="Play / Pause (Space)"
        >
          {isPlaying ? '⏸' : '▶'}
        </button>
        <button
          className={`transport-btn ${isRecording ? 'recording' : ''}`}
          onClick={handleRecord}
          data-tooltip="Record (R)"
        >
          ●
        </button>
        <button
          className={`transport-btn ${audioEngine.metronomeEnabled ? 'active' : ''}`}
          onClick={() => audioEngine.toggleMetronome()}
          data-tooltip="Metronome (M)"
        >
          🔔
        </button>
      </div>

      <div className="transport-right">
        <div className="tempo-control" onWheel={handleBpmWheel}>
          <div className="tempo-label">BPM</div>
          {editingBpm ? (
            <input
              ref={bpmRef}
              className="input input-sm mono tempo-input"
              value={bpmInput}
              onChange={(e) => setBpmInput(e.target.value)}
              onBlur={handleBpmSubmit}
              onKeyDown={(e) => e.key === 'Enter' && handleBpmSubmit()}
              autoFocus
              style={{ width: 52 }}
            />
          ) : (
            <div
              className="tempo-value mono"
              onClick={() => { setEditingBpm(true); setBpmInput(String(bpm)); }}
            >
              {bpm}
            </div>
          )}
        </div>

        <div className="time-sig-control">
          <div className="tempo-label">TIME</div>
          <div className="tempo-value mono">{timeSignature[0]}/{timeSignature[1]}</div>
        </div>

        <div className="separator" />

        <div className="master-vol">
          <div className="tempo-label">MASTER</div>
          <input
            type="range"
            min="0"
            max="1"
            step="0.01"
            value={masterVolume}
            onChange={(e) => {
              const v = parseFloat(e.target.value);
              setMasterVolume(v);
              audioEngine.setMasterVolume(v);
            }}
            className="master-fader"
          />
          <span className="master-db mono">{masterVolume > 0 ? (20 * Math.log10(masterVolume)).toFixed(1) : '-∞'} dB</span>
        </div>
      </div>
    </div>
  );
}
