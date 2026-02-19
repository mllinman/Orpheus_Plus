import React, { useState } from 'react';
import { useProjectStore } from '../../stores/projectStore';
import { useUIStore } from '../../stores/uiStore';
import { audioBufferManager } from '../../audio/AudioBufferManager';
import { AIStemSeparator } from '../../audio/AIStemSeparator';

const STEM_TYPES = [
  { id: 'vocals', label: 'Vocals', icon: '🎤', color: '#e91e8a', desc: 'AI-isolated Vocals' },
  { id: 'drums',  label: 'Drums',  icon: '🥁', color: '#ff7043', desc: 'AI-isolated Drums' },
  { id: 'bass',   label: 'Bass',   icon: '🎸', color: '#29b6f6', desc: 'AI-isolated Bass' },
  { id: 'other',  label: 'Other',  icon: '🎹', color: '#66bb6a', desc: 'AI-isolated Accompaniment' },
];

export default function StemSeparationPanel() {
  const { processStems, addStemTracks, tracks } = useProjectStore();
  const { selectedClipId, selectedClipTrackId } = useUIStore();
  const [isProcessing, setIsProcessing] = useState(false);
  const [useAI, setUseAI] = useState(true);
  const [progress, setProgress] = useState(0);
  const [status, setStatus] = useState('');
  const [successMsg, setSuccessMsg] = useState('');

  const handleProcess = async () => {
    if (!selectedClipId || !selectedClipTrackId) return;
    setIsProcessing(true);
    setProgress(0);
    setSuccessMsg('');
    setStatus('Initializing...');
    
    try {
      if (useAI) {
        // ─── AI Processing (Client Side) ───
        const track = tracks.find(t => t.id === selectedClipTrackId);
        if (!clip) throw new Error('No audio clip selected');

        // Check if buffer exists (Demo clips have no bufferId, reloaded projects lose buffer)
        if (!clip.bufferId) {
            throw new Error('This clip has no audio data (it might be a demo clip). Please import a real audio file.');
        }
        
        const bufferEntry = audioBufferManager.getBuffer(clip.bufferId);
        if (!bufferEntry) {
            throw new Error('Audio buffer not found in memory. If you reloaded the page, please re-import your audio file.');
        }

        setStatus('Loading AI Model (this may take a moment)...');
        
        // Run Separation
        const stemsObj = await AIStemSeparator.separate(bufferEntry.buffer, {
            onProgress: (p) => {
                setProgress(p);
                if (p < 0.2) setStatus('Loading Model...');
                else if (p < 0.9) setStatus('Separating Stems...');
                else setStatus('Finalizing...');
            }
        });

        setStatus('Creating Tracks...');
        
        // Save buffers
        const resultIds = {};
        for (const [label, buffer] of Object.entries(stemsObj)) {
            const bufId = await audioBufferManager.addBuffer(buffer);
            // Map raw label to our types if needed, usually 'vocals', 'drums', 'bass', 'other' matches
            resultIds[label] = bufId;
        }

        addStemTracks(selectedClipTrackId, selectedClipId, resultIds);
        setSuccessMsg('AI Separation Complete!');

      } else {
        // ─── Local EQ Processing ───
        setStatus('Applying EQ Filters...');
        await new Promise(r => setTimeout(r, 500));
        processStems();
        setSuccessMsg('Created 4 EQ-isolated tracks (Preview only)');
      }
    } catch (err) {
      console.error(err);
      setStatus('Error');
      alert('Separation Failed: ' + err.message + '\n\nEnsure you have an active internet connection for the first run to download the model.');
    }
    
    setIsProcessing(false);
    setProgress(0);
  };

  const isValidSelection = !!selectedClipId && !!selectedClipTrackId;

  return (
    <div className="stem-panel">
      <div className="stem-panel-header">
        <div className="stem-title-row">
          <span className="stem-icon">🎚</span>
          <span className="stem-title">STEM SEPARATION</span>
        </div>
      </div>

      <div className="stem-content" style={{ padding: 20 }}>
        <p className="text-muted" style={{ marginBottom: 20 }}>
          Split the selected audio clip into four stems (Vocals, Drums, Bass, Other).
          AI separation runs entirely in your browser.
        </p>

        <div className="stem-types-grid" style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 10, marginBottom: 20 }}>
          {STEM_TYPES.map(stem => (
            <div key={stem.id} className="stem-type-card" style={{ 
              background: 'var(--bg-elevated)', padding: 15, borderRadius: 8,
              borderLeft: `3px solid ${stem.color}`
            }}>
              <div style={{ display: 'flex', alignItems: 'center', gap: 8, marginBottom: 5 }}>
                <span>{stem.icon}</span>
                <span className="mono" style={{ fontWeight: 600 }}>{stem.label}</span>
              </div>
              <div style={{ fontSize: 'var(--text-xs)', opacity: 0.7 }}>{stem.desc}</div>
            </div>
          ))}
        </div>

        {/* AI Toggle */}
        <div style={{ display: 'flex', alignItems: 'center', gap: 10, marginBottom: 20, background: 'var(--bg-surface)', padding: 10, borderRadius: 6 }}>
           <input 
             type="checkbox" 
             checked={useAI} 
             onChange={(e) => setUseAI(e.target.checked)}
             style={{ width: 16, height: 16, cursor: 'pointer' }}
           />
           <div style={{ flex: 1 }}>
             <div style={{ fontWeight: 500, fontSize: 'var(--text-sm)' }}>High Quality AI Separation</div>
             <div className="text-muted" style={{ fontSize: 'var(--text-xs)' }}>
               {useAI ? 'Uses Demucs AI (Client-side). Slower but high quality.' : 'Uses EQ Isolation. Instant preview but poor separation.'}
             </div>
           </div>
        </div>

        {isProcessing && (
            <div style={{ marginBottom: 15 }}>
                <div style={{ display: 'flex', justifyContent: 'space-between', marginBottom: 5, fontSize: 'var(--text-xs)' }}>
                    <span>{status}</span>
                    <span>{Math.round(progress * 100)}%</span>
                </div>
                <div style={{ height: 6, background: 'var(--bg-dark)', borderRadius: 3, overflow: 'hidden' }}>
                    <div style={{ height: '100%', background: 'var(--accent-primary)', width: `${progress * 100}%`, transition: 'width 0.2s' }} />
                </div>
            </div>
        )}

        <div className="stem-action-area" style={{ textAlign: 'center' }}>
          {!isValidSelection ? (
            <div className="alert-box" style={{ background: 'rgba(255,255,255,0.05)', padding: 10, borderRadius: 4 }}>
              ⚠ Please select an audio clip in the arrangement/mixer to proceed.
            </div>
          ) : (
            <button 
              className={`btn btn-primary ${isProcessing ? 'loading' : ''}`}
              onClick={handleProcess}
              disabled={isProcessing}
              style={{ width: '100%', padding: '12px', fontSize: 16 }}
            >
              {isProcessing ? 'Processing Audio...' : (useAI ? '✨ Process with AI' : 'Separate Stems (EQ)')}
            </button>
          )}
          
          {successMsg && (
            <div style={{ marginTop: 10, color: 'var(--accent-success)' }} className="fade-in">
              ✔ {successMsg}
            </div>
          )}
          
           {useAI && !isProcessing && (
              <div style={{ marginTop: 15, fontSize: '10px', opacity: 0.5 }}>
                  Powered by transformers.js & Demucs
              </div>
           )}
        </div>
      </div>
    </div>
  );
}
