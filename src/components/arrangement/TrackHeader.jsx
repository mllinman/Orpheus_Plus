import React, { useState, useRef, useEffect } from 'react';
import { useProjectStore } from '../../stores/projectStore';
import { useUIStore } from '../../stores/uiStore';
import { TRACK_COLORS } from '../../utils/helpers';

export default function TrackHeader({ track, index, height }) {
  const { toggleMute, toggleSolo, toggleArmed, setTrackVolume, setTrackPan, removeTrack, duplicateTrack, renameTrack, setTrackColor } = useProjectStore();
  const { selectedTrackId, setSelectedTrack } = useUIStore();
  const isSelected = selectedTrackId === track.id;
  const [editing, setEditing] = useState(false);
  const [editName, setEditName] = useState(track.name);
  const [showColorPicker, setShowColorPicker] = useState(false);
  const inputRef = useRef(null);

  const typeIcon = track.type === 'midi' ? '🎹' : '🎵';

  useEffect(() => {
    if (editing && inputRef.current) {
      inputRef.current.focus();
      inputRef.current.select();
    }
  }, [editing]);

  const handleRename = () => {
    if (editName.trim()) {
      renameTrack(track.id, editName.trim());
    }
    setEditing(false);
  };

  const handleDragStart = (e) => {
    e.dataTransfer.setData('application/orpheus-track-index', index);
    e.dataTransfer.effectAllowed = 'move';
    // Create a drag image? Browser default is usually fine for row dragging
  };

  const handleDragOver = (e) => {
    if (e.dataTransfer.types.includes('application/orpheus-track-index')) {
      e.preventDefault(); // Allow drop
      e.dataTransfer.dropEffect = 'move';
    }
  };

  const handleDrop = (e) => {
    const fromIndexStr = e.dataTransfer.getData('application/orpheus-track-index');
    if (fromIndexStr) {
      e.preventDefault();
      const fromIndex = parseInt(fromIndexStr, 10);
      if (fromIndex !== index) {
        useProjectStore.getState().moveTrack(fromIndex, index);
      }
    }
  };

  const handleContextMenu = (e) => {
    e.preventDefault();
    const { showContextMenu } = useUIStore.getState();
    showContextMenu(e.clientX, e.clientY, [
      { label: 'Duplicate Track', action: () => duplicateTrack(track.id) },
      { label: 'Remove Track', action: () => removeTrack(track.id), danger: true },
      { divider: true },
      { label: 'Rename...', action: () => { setEditing(true); setEditName(track.name); } },
      { label: 'Change Color', action: () => setShowColorPicker(true) },
    ]);
  };

  return (
    <div
      className={`track-header ${isSelected ? 'selected' : ''}`}
      draggable
      onDragStart={handleDragStart}
      onDragOver={handleDragOver}
      onDrop={handleDrop}
      style={{
        height,
        borderLeft: `3px solid ${track.color}`,
      }}
      onClick={() => setSelectedTrack(track.id)}
      onContextMenu={handleContextMenu}
    >
      <div className="track-header-top">
        <span className="track-type-icon">{typeIcon}</span>
        {editing ? (
          <input
            ref={inputRef}
            className="input input-sm"
            value={editName}
            onChange={(e) => setEditName(e.target.value)}
            onBlur={handleRename}
            onKeyDown={(e) => { if (e.key === 'Enter') handleRename(); if (e.key === 'Escape') setEditing(false); }}
            onClick={(e) => e.stopPropagation()}
            style={{ width: '100%', height: 20, fontSize: 11, padding: '0 4px' }}
          />
        ) : (
          <span className="track-name truncate" onDoubleClick={() => { setEditing(true); setEditName(track.name); }}>
            {track.name}
          </span>
        )}
      </div>

      {/* Color picker popup */}
      {showColorPicker && (
        <div className="color-picker-popup" onClick={(e) => e.stopPropagation()}>
          {TRACK_COLORS.map(c => (
            <button
              key={c}
              className="color-swatch"
              style={{ background: c, width: 16, height: 16, border: c === track.color ? '2px solid #fff' : '1px solid rgba(255,255,255,0.2)', borderRadius: 3, cursor: 'pointer', margin: 1, padding: 0 }}
              onClick={() => { setTrackColor(track.id, c); setShowColorPicker(false); }}
            />
          ))}
        </div>
      )}

      <div className="track-header-controls">
        <button
          className={`track-btn ${track.mute ? 'mute-active' : ''}`}
          onClick={(e) => { e.stopPropagation(); toggleMute(track.id); }}
          data-tooltip="Mute"
        >
          M
        </button>
        <button
          className={`track-btn ${track.solo ? 'solo-active' : ''}`}
          onClick={(e) => { e.stopPropagation(); toggleSolo(track.id); }}
          data-tooltip="Solo"
        >
          S
        </button>
        <button
          className={`track-btn ${track.armed ? 'arm-active' : ''}`}
          onClick={(e) => { e.stopPropagation(); toggleArmed(track.id); }}
          data-tooltip="Record Arm"
        >
          R
        </button>
      </div>

      <div className="track-header-faders">
        <input
          type="range"
          min="0"
          max="1"
          step="0.01"
          value={track.volume}
          onChange={(e) => setTrackVolume(track.id, parseFloat(e.target.value))}
          className="track-volume-slider"
          data-tooltip={`Vol: ${Math.round(track.volume * 100)}%`}
          onClick={(e) => e.stopPropagation()}
        />
        <input
          type="range"
          min="-1"
          max="1"
          step="0.01"
          value={track.pan}
          onChange={(e) => setTrackPan(track.id, parseFloat(e.target.value))}
          className="track-pan-slider"
          data-tooltip={`Pan: ${track.pan > 0 ? `R${Math.round(track.pan * 100)}` : track.pan < 0 ? `L${Math.round(-track.pan * 100)}` : 'C'}`}
          onClick={(e) => e.stopPropagation()}
        />
      </div>
    </div>
  );
}
