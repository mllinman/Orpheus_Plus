import React, { useState, useEffect } from 'react';
import { useUIStore } from '../../stores/uiStore';

export default function ContextMenu() {
  const { contextMenu, hideContextMenu } = useUIStore();

  useEffect(() => {
    if (!contextMenu) return;
    const handle = () => hideContextMenu();
    document.addEventListener('mousedown', handle);
    return () => document.removeEventListener('mousedown', handle);
  }, [contextMenu, hideContextMenu]);

  if (!contextMenu) return null;

  return (
    <div
      className="context-menu"
      style={{ position: 'fixed', left: contextMenu.x, top: contextMenu.y, zIndex: 'var(--z-dropdown)' }}
      onMouseDown={(e) => e.stopPropagation()}
    >
      {contextMenu.items.map((item, i) =>
        item.divider ? (
          <div key={i} className="dropdown-divider" />
        ) : (
          <div
            key={i}
            className={`dropdown-item ${item.danger ? 'danger' : ''}`}
            onClick={() => { item.action?.(); hideContextMenu(); }}
          >
            {item.label}
          </div>
        )
      )}
    </div>
  );
}
