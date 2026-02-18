import React, { useState, useCallback } from 'react';
import { useProjectStore } from '../../stores/projectStore';
import { useUIStore } from '../../stores/uiStore';

const COLORS = [
  '#FF6B6B', '#4ECDC4', '#45B7D1', '#96CEB4', '#FFEAA7',
  '#DDA0DD', '#98D8C8', '#F7DC6F', '#BB8FCE', '#85C1E9',
  '#F0B27A', '#82E0AA', '#F1948A', '#85929E', '#AED6F1',
  '#FAD7A0',
];

export default function SessionView() {
  const { tracks, bpm } = useProjectStore();
  const { selectedTrackId, setSelectedTrack } = useUIStore();
  const [scenes, setScenes] = useState([
    { id: 's1', name: 'Intro', clipStates: {} },
    { id: 's2', name: 'Verse', clipStates: {} },
    { id: 's3', name: 'Chorus', clipStates: {} },
    { id: 's4', name: 'Bridge', clipStates: {} },
    { id: 's5', name: 'Outro', clipStates: {} },
  ]);
  const [playingClips, setPlayingClips] = useState({});
  const [queuedClips, setQueuedClips] = useState({});

  const toggleClip = useCallback((trackId, sceneId) => {
    const key = `${trackId}-${sceneId}`;
    if (playingClips[key]) {
      setPlayingClips(prev => { const n = { ...prev }; delete n[key]; return n; });
    } else {
      // Queue clip to start on next beat
      setQueuedClips(prev => ({ ...prev, [key]: true }));
      setTimeout(() => {
        setQueuedClips(prev => { const n = { ...prev }; delete n[key]; return n; });
        setPlayingClips(prev => ({ ...prev, [key]: true }));
      }, (60 / bpm) * 1000); // Next beat
    }
  }, [playingClips, bpm]);

  const triggerScene = useCallback((sceneId) => {
    const newPlaying = {};
    tracks.forEach(t => {
      newPlaying[`${t.id}-${sceneId}`] = true;
    });
    setPlayingClips(newPlaying);
  }, [tracks]);

  const stopAll = () => {
    setPlayingClips({});
    setQueuedClips({});
  };

  const addScene = () => {
    setScenes(prev => [...prev, {
      id: `s${Date.now()}`,
      name: `Scene ${prev.length + 1}`,
      clipStates: {},
    }]);
  };

  return (
    <div className="session-view">
      <div className="session-header">
        <h3>🎬 Session View</h3>
        <div className="session-controls">
          <button className="btn btn-xs btn-danger" onClick={stopAll}>⏹ Stop All</button>
          <button className="btn btn-xs btn-ghost" onClick={addScene}>+ Scene</button>
        </div>
      </div>

      <div className="session-grid">
        {/* Track headers */}
        <div className="session-track-headers">
          <div className="session-corner"></div>
          {tracks.map(track => (
            <div
              key={track.id}
              className={`session-track-label ${selectedTrackId === track.id ? 'selected' : ''}`}
              onClick={() => setSelectedTrack(track.id)}
              style={{ borderColor: track.color }}
            >
              {track.name}
            </div>
          ))}
        </div>

        {/* Scene rows */}
        <div className="session-scenes">
          {scenes.map((scene, si) => (
            <div key={scene.id} className="session-scene-row">
              <div className="session-scene-trigger" onClick={() => triggerScene(scene.id)}>
                <span className="scene-name">{scene.name}</span>
                <span className="scene-play">▶</span>
              </div>
              {tracks.map((track, ti) => {
                const key = `${track.id}-${scene.id}`;
                const isPlaying = playingClips[key];
                const isQueued = queuedClips[key];
                return (
                  <div
                    key={key}
                    className={`session-cell ${isPlaying ? 'playing' : ''} ${isQueued ? 'queued' : ''}`}
                    onClick={() => toggleClip(track.id, scene.id)}
                    style={{
                      backgroundColor: isPlaying ? COLORS[ti % COLORS.length] + '40' : undefined,
                      borderColor: COLORS[ti % COLORS.length],
                    }}
                  >
                    <div className="session-clip-indicator"
                      style={{ background: isPlaying ? COLORS[ti % COLORS.length] : 'var(--bg-hover)' }}
                    />
                  </div>
                );
              })}
            </div>
          ))}
        </div>
      </div>
    </div>
  );
}
