import React, { useState } from 'react';
import { useProjectStore } from '../../stores/projectStore';
import { useUIStore } from '../../stores/uiStore';

export default function TemplateManager() {
  const { saveTemplate, loadTemplate, getTemplates, deleteTemplate } = useProjectStore();
  const { closeModal } = useUIStore();
  const [templateName, setTemplateName] = useState('');
  const templates = getTemplates();

  const handleSave = () => {
    if (!templateName.trim()) return;
    saveTemplate(templateName.trim());
    setTemplateName('');
  };

  return (
    <div className="modal-overlay" onClick={closeModal}>
      <div className="modal template-manager" onClick={e => e.stopPropagation()}>
        <div className="modal-header">
          <h2>📋 Templates</h2>
          <button className="btn btn-icon" onClick={closeModal}>✕</button>
        </div>
        <div className="modal-body">
          <div className="template-save-row">
            <input
              className="input"
              placeholder="Template name..."
              value={templateName}
              onChange={e => setTemplateName(e.target.value)}
              onKeyDown={e => e.key === 'Enter' && handleSave()}
            />
            <button className="btn btn-sm btn-accent" onClick={handleSave}>
              Save Current
            </button>
          </div>
          <div className="template-list">
            {templates.length === 0 && (
              <div className="template-empty">No saved templates. Save your current track layout as a template for instant recall.</div>
            )}
            {templates.map((tmpl, i) => (
              <div key={i} className="template-row">
                <div className="template-info">
                  <span className="template-name">{tmpl.name}</span>
                  <span className="template-meta">
                    {tmpl.tracks?.length || 0} tracks · {tmpl.bpm} BPM · {new Date(tmpl.savedAt).toLocaleDateString()}
                  </span>
                </div>
                <div className="template-actions">
                  <button className="btn btn-xs btn-ghost" onClick={() => { loadTemplate(i); closeModal(); }}>Load</button>
                  <button className="btn btn-xs btn-ghost" onClick={() => deleteTemplate(i)}>✕</button>
                </div>
              </div>
            ))}
          </div>
        </div>
      </div>
    </div>
  );
}
