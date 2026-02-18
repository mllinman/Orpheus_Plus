import React, { useState, useRef, useEffect } from 'react';
import { audioBufferManager } from '../../audio/AudioBufferManager';
import { pluginManager } from '../../audio/PluginManager';
import { useProjectStore } from '../../stores/projectStore';
import { useUIStore } from '../../stores/uiStore';
import { audioEngine } from '../../audio/AudioEngine';

const BROWSER_TABS = [
  { id: 'files', label: 'Files', icon: '📁' },
  { id: 'plugins', label: 'Plugins', icon: '🔌' },
  { id: 'instruments', label: 'Instruments', icon: '🎹' },
  { id: 'effects', label: 'Effects', icon: '🎛' },
  { id: 'samples', label: 'Samples', icon: '🎵' },
];

const BUILT_IN_SAMPLES = {
  'Kick_Deep.wav': { freq: 55, dur: 0.3, type: 'sine', env: 'kick' },
  'Kick_Punch.wav': { freq: 80, dur: 0.2, type: 'sine', env: 'kick' },
  'Snare_Tight.wav': { freq: 200, dur: 0.15, type: 'noise', env: 'snare' },
  'Snare_Fat.wav': { freq: 180, dur: 0.2, type: 'noise', env: 'snare' },
  'HiHat_Closed.wav': { freq: 8000, dur: 0.05, type: 'noise', env: 'hat' },
  'HiHat_Open.wav': { freq: 8000, dur: 0.2, type: 'noise', env: 'hat' },
  'Clap_1.wav': { freq: 1500, dur: 0.12, type: 'noise', env: 'clap' },
  'Sub_Bass_C1.wav': { freq: 32.7, dur: 1.0, type: 'sine', env: 'bass' },
  'Pad_Ambient.wav': { freq: 440, dur: 2.0, type: 'sine', env: 'pad' },
  'Lead_Saw.wav': { freq: 440, dur: 0.5, type: 'sawtooth', env: 'lead' },
};

const INSTRUMENT_ITEMS = [
  { name: 'Orpheus Synth', type: 'synth', icon: '🎹', children: [
    { name: 'Init Patch', preset: 'init' },
    { name: 'Deep Bass', preset: 'deep_bass' },
    { name: 'Warm Pad', preset: 'warm_pad' },
    { name: 'Pluck Lead', preset: 'pluck_lead' },
    { name: 'Super Saw', preset: 'super_saw' },
    { name: 'Acid Squelch', preset: 'acid' },
  ]},
  { name: 'Orpheus Sampler', type: 'sampler', icon: '🥁', children: [
    { name: 'Acoustic Kit', preset: 'acoustic' },
    { name: 'Electronic Kit', preset: 'electronic' },
    { name: 'TR-808 Kit', preset: 'tr808' },
    { name: 'Lo-Fi Kit', preset: 'lofi' },
  ]},
];

const SAMPLE_CATEGORIES = [
  { name: 'Drums', children: [
    'Kick_Deep.wav', 'Kick_Punch.wav', 'Snare_Tight.wav', 'Snare_Fat.wav',
    'HiHat_Closed.wav', 'HiHat_Open.wav', 'Clap_1.wav',
  ]},
  { name: 'Bass', children: ['Sub_Bass_C1.wav'] },
  { name: 'Synth', children: ['Pad_Ambient.wav', 'Lead_Saw.wav'] },
];

export default function BrowserPanel() {
  const [activeTab, setActiveTab] = useState('files');
  const [searchQuery, setSearchQuery] = useState('');
  const [expanded, setExpanded] = useState({});
  const [importedFiles, setImportedFiles] = useState([]);
  const [pluginList, setPluginList] = useState(pluginManager.getAllPlugins());
  const [pluginExpanded, setPluginExpanded] = useState({});
  const [previewingId, setPreviewingId] = useState(null);
  
  // Native File Browser State
  const isElectron = !!window.electronAPI;
  const [currentPath, setCurrentPath] = useState('');
  const [dirEntries, setDirEntries] = useState([]);
  const [showProjectFiles, setShowProjectFiles] = useState(false);

  const fileInputRef = useRef(null);
  const pluginInputRef = useRef(null);

  // Subscribe to buffer manager changes
  useEffect(() => {
    const unsub1 = audioBufferManager.subscribe(() => {
      setImportedFiles(audioBufferManager.getAllBuffers());
    });
    const unsub2 = pluginManager.subscribe(() => {
      setPluginList(pluginManager.getAllPlugins());
    });
    setImportedFiles(audioBufferManager.getAllBuffers());
    
    // Initialize Native Path
    if (isElectron) {
      window.electronAPI.getPath('documents').then(docPath => {
        if (docPath) {
            setCurrentPath(docPath);
            loadDir(docPath);
        }
      });
    }

    return () => { unsub1(); unsub2(); };
  }, [isElectron]);

  const loadDir = async (path) => {
    if (!window.electronAPI) return;
    try {
      const entries = await window.electronAPI.readDir(path);
      // Filter & Sort
      const filtered = entries.filter(e => 
        e.isDirectory || /\.(wav|mp3|ogg|flac|aiff)$/i.test(e.name)
      ).sort((a, b) => {
         if (a.isDirectory && !b.isDirectory) return -1;
         if (!a.isDirectory && b.isDirectory) return 1;
         return a.name.localeCompare(b.name);
      });
      setDirEntries(filtered);
      setCurrentPath(path);
    } catch (err) {
      console.error('Failed to read dir', err);
    }
  };

  const navigateUp = () => {
    if (!currentPath) return;
    const sep = currentPath.includes('\\') ? '\\' : '/';
    const parts = currentPath.split(sep);
    parts.pop();
    const newPath = parts.join(sep) || sep;
    loadDir(newPath);
  };

  const toggleExpand = (name) => {
    setExpanded(prev => ({ ...prev, [name]: !prev[name] }));
  };

  const handleNativeDoubleClick = async (entry) => {
    if (entry.isDirectory) {
      loadDir(entry.path);
    } else {
      try {
        const id = await audioBufferManager.loadFromPath(entry.path);
        const bufferInfo = audioBufferManager.getBuffer(id);
        addToTimeline(bufferInfo);
      } catch (e) {
        console.error('Load failed', e);
      }
    }
  };

  const handleFileImport = async (e) => {
    const files = Array.from(e.target.files);
    await audioEngine.init();
    for (const file of files) {
      try {
        await audioBufferManager.loadFile(file);
      } catch (err) {
        console.error('Failed to import:', file.name, err);
      }
    }
    e.target.value = '';
  };

  const handlePluginImport = async (e) => {
    const files = Array.from(e.target.files);
    for (const file of files) {
      try {
        await pluginManager.importPlugin(file);
      } catch (err) {
        console.error('Failed to import plugin:', file.name, err);
      }
    }
    e.target.value = '';
  };

  const handleDragStart = (e, item) => {
    e.dataTransfer.setData('application/orpheus-item', JSON.stringify(item));
    e.dataTransfer.effectAllowed = 'copy';
  };

  const addToTimeline = (bufferInfo) => {
    if (!bufferInfo) return;
    const proj = useProjectStore.getState();
    let targetTrack = proj.tracks.find(t => t.type === 'audio');
    if (!targetTrack) {
      proj.addTrack('audio');
      targetTrack = useProjectStore.getState().tracks[useProjectStore.getState().tracks.length - 1];
    }

    const lengthBeats = Math.ceil(audioBufferManager.durationToBeats(bufferInfo.duration, proj.bpm));
    let startBeat = 0;
    targetTrack.clips.forEach(c => {
      const end = c.startBeat + c.lengthBeats;
      if (end > startBeat) startBeat = end;
    });

    proj.addClip(targetTrack.id, {
      id: `clip_${Date.now()}_${Math.random().toString(36).slice(2, 6)}`,
      trackId: targetTrack.id,
      type: 'audio',
      name: bufferInfo.fileName.replace(/\.[^.]+$/, ''),
      startBeat,
      lengthBeats,
      offset: 0,
      gain: 1,
      fadeIn: 0,
      fadeOut: 0,
      waveformData: bufferInfo.waveformData,
      bufferId: bufferInfo.id,
      color: null,
    });
  };

  const handlePreview = async (id) => {
    setPreviewingId(id);
    audioBufferManager.previewBuffer(id);
    setTimeout(() => setPreviewingId(null), 2000);
  };

  const generateAndPlaySample = async (sampleName) => {
    await audioEngine.init();
    const spec = BUILT_IN_SAMPLES[sampleName];
    if (!spec) return;
    const buffer = audioEngine.generateTone(spec.freq, spec.dur, spec.type === 'noise' ? 'sawtooth' : spec.type);
    audioEngine.playBuffer(buffer, 0, 0.5);
  };

  const filterMatch = (name) =>
    !searchQuery || name.toLowerCase().includes(searchQuery.toLowerCase());

  const getContent = () => {
    if (activeTab === 'files') {
      if (isElectron && !showProjectFiles) {
        return (
          <div className="browser-section" style={{ height: '100%', display: 'flex', flexDirection: 'column' }}>
            <div style={{ padding: '8px', borderBottom: '1px solid var(--border-color)', display: 'flex', gap: 6, alignItems: 'center' }}>
              <button className="btn btn-sm" onClick={navigateUp}>⬆</button>
              <div style={{ flex: 1, background: '#111', padding: '2px 6px', borderRadius: 4, overflow: 'hidden', whiteSpace: 'nowrap', textOverflow: 'ellipsis', fontSize: '11px', lineHeight: '20px' }}>
                {currentPath || '...'}
              </div>
              <button className="btn btn-sm" onClick={() => setShowProjectFiles(true)}>Project</button>
            </div>
            
            <div style={{ flex: 1, overflowY: 'auto' }}>
              {dirEntries.map((entry, i) => (
                 <div
                   key={entry.name + i}
                   className="browser-item"
                   style={{ padding: '4px 8px', display: 'flex', alignItems: 'center', gap: 6, cursor: 'pointer' }}
                   onDoubleClick={() => handleNativeDoubleClick(entry)}
                 >
                   <span style={{ opacity: 0.7 }}>{entry.isDirectory ? '📁' : '🎵'}</span>
                   <div className="truncate" style={{ flex: 1 }}>{entry.name}</div>
                   {!entry.isDirectory && (
                      <span className="text-muted" style={{ fontSize: '10px' }}>
                        {(entry.size / 1024 / 1024).toFixed(1)}MB
                      </span>
                   )}
                 </div>
              ))}
              {dirEntries.length === 0 && (
                <div style={{ padding: 20, textAlign: 'center', opacity: 0.5 }}>Empty Folder</div>
              )}
            </div>
          </div>
        );
      }

      return (
        <div className="browser-section">
          <div className="browser-actions" style={{ padding: '6px 8px', display: 'flex', gap: 6 }}>
            {isElectron && (
               <button className="btn btn-sm" onClick={() => setShowProjectFiles(false)}>
                 ⬅ Computer
               </button>
            )}
            <button
              className="btn btn-sm"
              onClick={() => fileInputRef.current?.click()}
              style={{ flex: 1 }}
            >
              + Import Files
            </button>
          </div>
          {importedFiles.length === 0 ? (
            <div className="browser-empty">
              <p className="text-muted" style={{ fontSize: 'var(--text-xs)', textAlign: 'center', padding: '20px 12px' }}>
                No files imported.<br />
                {isElectron ? 'Switch to "Computer" to browse disk.' : 'Drag & drop audio files here.'}
              </p>
            </div>
          ) : (
            importedFiles.filter(f => filterMatch(f.fileName)).map((file) => (
              <div
                key={file.id}
                className={`browser-item file-item ${previewingId === file.id ? 'previewing' : ''}`}
                style={{ paddingLeft: 8, display: 'flex', alignItems: 'center', gap: 6 }}
                draggable
                onDragStart={(e) => handleDragStart(e, { type: 'audio-buffer', bufferId: file.id, fileName: file.fileName })}
                onDoubleClick={() => addToTimeline(file)}
              >
                <button
                  className="btn-icon-tiny"
                  onClick={(e) => { e.stopPropagation(); handlePreview(file.id); }}
                  title="Preview"
                >
                  {previewingId === file.id ? '⏸' : '▶'}
                </button>
                <div style={{ flex: 1, minWidth: 0 }}>
                  <div className="browser-item-name truncate">{file.fileName}</div>
                  <div className="text-muted" style={{ fontSize: 'var(--text-xs)' }}>
                    {file.duration.toFixed(1)}s • {file.channels}ch
                  </div>
                </div>
                <button
                  className="btn-icon-tiny"
                  onClick={(e) => { e.stopPropagation(); addToTimeline(file); }}
                  title="Add to timeline"
                >
                  +
                </button>
              </div>
            ))
          )}
        </div>
      );
    }

    if (activeTab === 'plugins') {
      const categories = pluginManager.getCategories();
        return (
          <div className="browser-section">
            <div className="browser-actions" style={{ padding: '6px 8px', display: 'flex', gap: 6 }}>
              <button
                className="btn btn-sm"
                onClick={() => pluginInputRef.current?.click()}
                style={{ flex: 1 }}
              >
                + Import VST Plugin
              </button>
            </div>
            {/* Filter and categorize plugins code preserved from original but concise here for space if standard */}
            {categories.filter(cat => filterMatch(cat) ||
              pluginList.some(p => p.category === cat && filterMatch(p.name))
            ).map(category => (
              <div key={category}>
                <div
                  className="browser-category"
                  onClick={() => setPluginExpanded(prev => ({ ...prev, [category]: !prev[category] }))}
                >
                  <span className="arrow">{pluginExpanded[category] ? '▼' : '▶'}</span>
                  <span>{category}</span>
                </div>
                {pluginExpanded[category] && (
                  <div className="browser-category-content">
                    {pluginList
                      .filter(p => p.category === category && filterMatch(p.name))
                      .map(plugin => (
                        <div
                          key={plugin.id}
                          className="browser-item"
                          draggable
                          onDragStart={(e) => handleDragStart(e, { type: 'plugin', id: plugin.id, name: plugin.name })}
                        >
                          <span className="emoji">🔌</span>
                          <span>{plugin.name}</span>
                        </div>
                      ))}
                  </div>
                )}
              </div>
            ))}
          </div>
        );
    }

    if (activeTab === 'instruments') {
        return (
          <div className="browser-section">
            {INSTRUMENT_ITEMS.map(cat => (
              <div key={cat.name}>
                <div
                  className="browser-category"
                  onClick={() => toggleExpand(cat.name)}
                >
                  <span className="arrow">{expanded[cat.name] ? '▼' : '▶'}</span>
                  <span>{cat.name}</span>
                </div>
                {expanded[cat.name] && (
                  <div className="browser-category-content">
                    {cat.children.map(item => (
                      <div
                        key={item.name}
                        className="browser-item"
                        draggable
                        onDragStart={(e) => handleDragStart(e, { type: 'instrument', name: item.name, preset: item.preset })}
                      >
                        <span className="emoji">{cat.icon}</span>
                        <span>{item.name}</span>
                      </div>
                    ))}
                  </div>
                )}
              </div>
            ))}
          </div>
        );
    }

    if (activeTab === 'samples') {
        return (
          <div className="browser-section">
            {SAMPLE_CATEGORIES.map(cat => (
              <div key={cat.name}>
                <div
                  className="browser-category"
                  onClick={() => toggleExpand(cat.name)}
                >
                  <span className="arrow">{expanded[cat.name] ? '▼' : '▶'}</span>
                  <span>{cat.name}</span>
                </div>
                {expanded[cat.name] && (
                  <div className="browser-category-content">
                    {cat.children.map(sample => (
                      <div
                        key={sample}
                        className="browser-item"
                        draggable
                        onDragStart={(e) => handleDragStart(e, { type: 'sample', name: sample })}
                        onClick={() => generateAndPlaySample(sample)}
                      >
                        <span className="emoji">🎵</span>
                        <span>{sample.replace('.wav', '')}</span>
                      </div>
                    ))}
                  </div>
                )}
              </div>
            ))}
          </div>
        );
    }

    return null;
  };

  return (
    <div className="browser-panel">
      <div className="browser-tabs">
        {BROWSER_TABS.map(tab => (
          <button
            key={tab.id}
            className={`browser-tab ${activeTab === tab.id ? 'active' : ''}`}
            onClick={() => setActiveTab(tab.id)}
            title={tab.label}
          >
            {tab.icon}
          </button>
        ))}
      </div>
      <div className="browser-search">
        <input
          type="text"
          placeholder="Search..."
          value={searchQuery}
          onChange={(e) => setSearchQuery(e.target.value)}
        />
      </div>
      <div className="browser-content">
        {getContent()}
      </div>
      <input type="file" ref={fileInputRef} onChange={handleFileImport} multiple accept="audio/*,.wav,.mp3" style={{ display: 'none' }} />
      <input type="file" ref={pluginInputRef} onChange={handlePluginImport} multiple accept=".js,.wasm" style={{ display: 'none' }} />
    </div>
  );
}
