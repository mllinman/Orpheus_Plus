import React, { useState, useCallback } from 'react';
import { useUIStore } from '../../stores/uiStore';

const AVAILABLE_ACTIONS = [
  { id: 'playStop', label: 'Play / Stop' },
  { id: 'record', label: 'Record' },
  { id: 'loop', label: 'Toggle Loop' },
  { id: 'undo', label: 'Undo' },
  { id: 'redo', label: 'Redo' },
  { id: 'save', label: 'Save Project' },
  { id: 'newProject', label: 'New Project' },
  { id: 'quantize', label: 'Quantize Selection' },
  { id: 'duplicate', label: 'Duplicate Track' },
  { id: 'addTrack', label: 'Add Track' },
  { id: 'removeTrack', label: 'Remove Track' },
  { id: 'viewArrangement', label: 'Show Arrangement' },
  { id: 'viewMixer', label: 'Show Mixer' },
  { id: 'viewPianoRoll', label: 'Show Piano Roll' },
  { id: 'viewBrowser', label: 'Show Browser' },
  { id: 'toggleStemSeparation', label: 'Toggle STEM Panel' },
  { id: 'toggleMastering', label: 'Toggle Mastering' },
  { id: 'toggleAutotune', label: 'Toggle Autotune' },
  { id: 'zoomIn', label: 'Zoom In' },
  { id: 'zoomOut', label: 'Zoom Out' },
  { id: 'zoomToFit', label: 'Zoom to Fit' },
  { id: 'selectAll', label: 'Select All' },
  { id: 'toolPointer', label: 'Pointer Tool' },
  { id: 'toolDraw', label: 'Draw Tool' },
  { id: 'toolSplit', label: 'Split Tool' },
  { id: 'toolErase', label: 'Erase Tool' },
  { id: 'toolSmart', label: 'Smart Tool' },
  { id: 'toolRazor', label: 'Razor Tool' },
  { id: 'freeze', label: 'Freeze Track' },
  { id: 'glue', label: 'Glue Clips' },
];

export default function HotkeyEditor() {
  const { customHotkeys, setCustomHotkey, removeCustomHotkey, closeModal } = useUIStore();
  const [recording, setRecording] = useState(null); // action ID being recorded
  const [searchFilter, setSearchFilter] = useState('');

  const handleKeyCapture = useCallback((e) => {
    if (!recording) return;
    e.preventDefault();
    e.stopPropagation();

    const parts = [];
    if (e.ctrlKey) parts.push('Ctrl');
    if (e.shiftKey) parts.push('Shift');
    if (e.altKey) parts.push('Alt');
    if (e.metaKey) parts.push('Meta');

    const key = e.key;
    if (!['Control', 'Shift', 'Alt', 'Meta'].includes(key)) {
      parts.push(key.length === 1 ? key.toUpperCase() : key);
      const combo = parts.join('+');
      setCustomHotkey(combo, recording);
      setRecording(null);
    }
  }, [recording, setCustomHotkey]);

  const currentBindings = Object.entries(customHotkeys).reduce((acc, [combo, action]) => {
    acc[action] = combo;
    return acc;
  }, {});

  const filteredActions = AVAILABLE_ACTIONS.filter(a =>
    a.label.toLowerCase().includes(searchFilter.toLowerCase()) ||
    a.id.toLowerCase().includes(searchFilter.toLowerCase())
  );

  return (
    <div className="modal-overlay" onClick={closeModal}>
      <div className="modal hotkey-editor" onClick={e => e.stopPropagation()} onKeyDown={handleKeyCapture} tabIndex={0}>
        <div className="modal-header">
          <h2>⌨ Customize Hotkeys</h2>
          <button className="btn btn-icon" onClick={closeModal}>✕</button>
        </div>
        <div className="modal-body">
          <input
            className="input hotkey-search"
            placeholder="Search actions..."
            value={searchFilter}
            onChange={e => setSearchFilter(e.target.value)}
            autoFocus
          />
          <div className="hotkey-list">
            {filteredActions.map(action => (
              <div key={action.id} className={`hotkey-row ${recording === action.id ? 'recording' : ''}`}>
                <span className="hotkey-action-label">{action.label}</span>
                <div className="hotkey-binding">
                  {recording === action.id ? (
                    <span className="hotkey-recording">Press keys...</span>
                  ) : (
                    <span className="hotkey-combo">{currentBindings[action.id] || '—'}</span>
                  )}
                  <button
                    className="btn btn-xs btn-ghost"
                    onClick={() => setRecording(recording === action.id ? null : action.id)}
                  >
                    {recording === action.id ? 'Cancel' : 'Set'}
                  </button>
                  {currentBindings[action.id] && (
                    <button
                      className="btn btn-xs btn-ghost"
                      onClick={() => removeCustomHotkey(currentBindings[action.id])}
                    >
                      ✕
                    </button>
                  )}
                </div>
              </div>
            ))}
          </div>
        </div>
      </div>
    </div>
  );
}
