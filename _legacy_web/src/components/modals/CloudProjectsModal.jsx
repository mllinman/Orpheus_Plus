import React, { useEffect, useState } from 'react';
import { useProjectStore } from '../../stores/projectStore';

export default function CloudProjectsModal({ closeModal, mode = 'load' }) {
    const { cloudProjects, fetchProjects, loadFromCloud, saveToCloud, projectName, setProjectName, isSaving, isLoading } = useProjectStore();
    const [localName, setLocalName] = useState(projectName);
    const [activeTab, setActiveTab] = useState(mode);

    useEffect(() => {
        fetchProjects();
    }, []);

    const handleLoad = async (id) => {
        await loadFromCloud(id);
        closeModal();
    };

    const handleSave = async () => {
        if (!localName.trim()) return;
        setProjectName(localName); // Update store name
        const success = await saveToCloud();
        if (success) {
            alert('Project saved to cloud!');
            closeModal();
        }
    };

    const formatDate = (dateStr) => {
        return new Date(dateStr).toLocaleDateString() + ' ' + new Date(dateStr).toLocaleTimeString();
    };

    return (
        <>
            <div className="modal-header">
                <h2>Cloud Projects</h2>
                <button className="btn btn-icon btn-ghost" onClick={closeModal}>✕</button>
            </div>
            
            <div className="modal-body" style={{ padding: 0, gap: 0 }}>
                {/* Tabs */}
                <div style={{ display: 'flex', borderBottom: '1px solid var(--border-subtle)' }}>
                    <button 
                        className={`btn btn-ghost ${activeTab === 'load' ? 'active' : ''}`}
                        style={{ flex: 1, borderRadius: 0, borderBottom: activeTab === 'load' ? '2px solid var(--accent-primary)' : '2px solid transparent' }}
                        onClick={() => setActiveTab('load')}
                    >
                        Open Project
                    </button>
                    <button 
                        className={`btn btn-ghost ${activeTab === 'save' ? 'active' : ''}`}
                        style={{ flex: 1, borderRadius: 0, borderBottom: activeTab === 'save' ? '2px solid var(--accent-primary)' : '2px solid transparent' }}
                        onClick={() => setActiveTab('save')}
                    >
                        Save Project
                    </button>
                </div>

                <div style={{ padding: 20 }}>
                    {activeTab === 'load' && (
                        <div style={{ display: 'flex', flexDirection: 'column', gap: 10 }}>
                            {cloudProjects.length === 0 ? (
                                <div className="text-muted" style={{ textAlign: 'center', padding: 20 }}>
                                    No cloud projects found.
                                </div>
                            ) : (
                                <div style={{ maxHeight: 300, overflowY: 'auto', display: 'flex', flexDirection: 'column', gap: 8 }}>
                                    {cloudProjects.map(p => (
                                        <div key={p.id} className="settings-device-item" style={{ justifyContent: 'space-between' }}>
                                            <div style={{ display: 'flex', flexDirection: 'column' }}>
                                                <span style={{ fontWeight: 500 }}>{p.name}</span>
                                                <span className="text-muted" style={{ fontSize: 'var(--text-xs)' }}>{formatDate(p.updatedAt)}</span>
                                            </div>
                                            <button 
                                                className="btn btn-sm btn-primary" 
                                                onClick={() => handleLoad(p.id)}
                                                disabled={isLoading}
                                            >
                                                {isLoading ? 'Loading...' : 'Open'}
                                            </button>
                                        </div>
                                    ))}
                                </div>
                            )}
                        </div>
                    )}

                    {activeTab === 'save' && (
                        <div style={{ display: 'flex', flexDirection: 'column', gap: 16 }}>
                            <div>
                                <label className="text-secondary" style={{ display: 'block', marginBottom: 8 }}>Project Name</label>
                                <input 
                                    className="input" 
                                    style={{ width: '100%' }}
                                    value={localName}
                                    onChange={(e) => setLocalName(e.target.value)}
                                    placeholder="My Awesome Song"
                                />
                            </div>
                            <button 
                                className="btn btn-primary" 
                                onClick={handleSave}
                                disabled={isSaving || !localName.trim()}
                            >
                                {isSaving ? 'Saving...' : 'Save to Cloud'}
                            </button>
                            <p className="text-muted" style={{ fontSize: 'var(--text-xs)' }}>
                                Saving will create a new version of the project in the database.
                            </p>
                        </div>
                    )}
                </div>
            </div>
        </>
    );
}
