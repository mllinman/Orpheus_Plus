import React from 'react';
import { useProjectStore } from '../../stores/projectStore';
import { useUIStore } from '../../stores/uiStore';

export default function UndoHistoryPanel() {
  const { undoStack, undoLabels, undo } = useProjectStore();
  const { showUndoHistory, toggleUndoHistory } = useUIStore();

  if (!showUndoHistory) return null;

  return (
    <div className="undo-history-panel">
      <div className="undo-history-header">
        <span>⟲ Undo History</span>
        <button className="btn btn-icon btn-xs" onClick={toggleUndoHistory}>✕</button>
      </div>
      <div className="undo-history-list">
        {undoLabels.length === 0 && (
          <div className="undo-history-empty">No actions yet</div>
        )}
        {[...undoLabels].reverse().map((label, i) => (
          <div
            key={i}
            className={`undo-history-item ${i === 0 ? 'latest' : ''}`}
            onClick={() => {
              // Undo to reach this state: undo (i + 1) times
              for (let j = 0; j <= i; j++) undo();
            }}
            title={`Click to revert to before "${label}"`}
          >
            <span className="undo-icon">{i === 0 ? '●' : '○'}</span>
            <span className="undo-label">{label}</span>
            <span className="undo-step">-{i + 1}</span>
          </div>
        ))}
        <div className="undo-history-item undo-origin">
          <span className="undo-icon">◆</span>
          <span className="undo-label">Session Start</span>
        </div>
      </div>
    </div>
  );
}
