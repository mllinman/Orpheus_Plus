import React, { useState, useEffect } from 'react';
import { useUIStore } from '../../stores/uiStore';
import { useProjectStore } from '../../stores/projectStore';
import { audioExporter } from '../../audio/AudioExporter';
import { pluginManager } from '../../audio/PluginManager';
import CloudProjectsModal from '../modals/CloudProjectsModal';

const SHORTCUT_GROUPS = [
  { title: 'Transport', items: [
    ['Space', 'Play / Stop'], ['R', 'Record'], ['L', 'Toggle Loop'], ['M', 'Metronome'],
    ['Home', 'Go to Start'], ['End', 'Go to End'],
  ]},
  { title: 'File', items: [
    ['Ctrl+N', 'New Project'], ['Ctrl+O', 'Open Project'], ['Ctrl+S', 'Save'],
    ['Ctrl+Shift+S', 'Save As'], ['Ctrl+Shift+E', 'Export Audio'],
  ]},
  { title: 'Edit', items: [
    ['Ctrl+Z', 'Undo'], ['Ctrl+Shift+Z', 'Redo'], ['Ctrl+X', 'Cut'],
    ['Ctrl+C', 'Copy'], ['Ctrl+V', 'Paste'], ['Delete', 'Delete Selection'],
  ]},
  { title: 'View', items: [
    ['F1', 'Arrangement'], ['F2', 'Mixer'], ['F3', 'Piano Roll'], ['F4', 'Browser'],
    ['F5', 'STEM Separation'], ['F6', 'Mastering'], ['F7', 'Autotune'],
    ['Ctrl++', 'Zoom In'], ['Ctrl+-', 'Zoom Out'],
  ]},
  { title: 'Tools', items: [
    ['1', 'Pointer'], ['2', 'Range'], ['3', 'Draw'], ['4', 'Split'], ['5', 'Erase'], ['6', 'Automation'],
  ]},
  { title: 'Tracks', items: [
    ['Ctrl+T', 'Add Audio Track'], ['Ctrl+Shift+T', 'Add MIDI Track'],
  ]},
];

const QUALITY_PRESETS = {
  cd: { label: 'CD Quality', sampleRate: 44100, bitDepth: 16 },
  streaming: { label: 'Streaming', sampleRate: 44100, bitDepth: 24 },
  hd: { label: 'HD', sampleRate: 96000, bitDepth: 24 },
  studio: { label: 'Studio Master', sampleRate: 192000, bitDepth: 32 },
};

export default function Modal() {
  const { activeModal, closeModal } = useUIStore();
  const projectName = useProjectStore(s => s.projectName);

  if (!activeModal) return null;

  return (
    <div className="modal-overlay" onClick={closeModal}>
      <div className="modal-content" onClick={(e) => e.stopPropagation()}>
        {activeModal === 'about' && <AboutContent closeModal={closeModal} />}
        {activeModal === 'settings' && <SettingsContent closeModal={closeModal} />}
        {activeModal === 'export' && <ExportContent closeModal={closeModal} projectName={projectName} />}
        {activeModal === 'shortcuts' && <ShortcutsContent closeModal={closeModal} />}
        {(activeModal === 'cloud' || activeModal === 'cloudSave') && (
            <CloudProjectsModal 
                closeModal={closeModal} 
                mode={activeModal === 'cloudSave' ? 'save' : 'load'} 
            />
        )}
      </div>
    </div>
  );
}

// ─── About ───
function AboutContent({ closeModal }) {
  return (
    <>
      <div className="modal-header">
        <h2>About Orpheus</h2>
        <button className="btn btn-icon btn-ghost" onClick={closeModal}>✕</button>
      </div>
      <div className="modal-body" style={{ textAlign: 'center', gap: 8 }}>
        <div style={{ fontSize: 48 }}>♪</div>
        <h3 style={{ margin: 0, color: 'var(--text-primary)' }}>Orpheus DAW</h3>
        <p className="text-secondary" style={{ margin: 0 }}>Version 1.0.0</p>
        <p className="text-muted" style={{ fontSize: 'var(--text-sm)', lineHeight: 1.5, maxWidth: 300 }}>
          Professional Digital Audio Workstation built with Web Audio API, React, and Zustand.
          Features real-time audio processing, MIDI editing, STEM separation, mastering, and autotune.
        </p>
        <div style={{ fontSize: 'var(--text-xs)', color: 'var(--text-muted)', marginTop: 8 }}>
          Built with ❤️ using Web Audio API
        </div>
      </div>
    </>
  );
}

// ─── Settings (Advanced Audio) ───
function SettingsContent({ closeModal }) {
  const [activeSettingsTab, setActiveSettingsTab] = useState('audio');
  const [audioDevices, setAudioDevices] = useState({ output: [], input: [] });
  const [selectedOutput, setSelectedOutput] = useState('default');
  const [selectedInput, setSelectedInput] = useState('default');
  const [bufferSize, setBufferSize] = useState('512');
  const [sampleRate, setSampleRate] = useState('44100');
  const [midiDevices, setMidiDevices] = useState([]);
  const [vstPaths, setVstPaths] = useState(pluginManager.scanPaths);
  const [newPath, setNewPath] = useState('');
  const bpm = useProjectStore(s => s.bpm);
  const setBpm = useProjectStore(s => s.setBpm);
  const projectName = useProjectStore(s => s.projectName);
  const setProjectName = useProjectStore(s => s.setProjectName);
  const timeSignature = useProjectStore(s => s.timeSignature);
  const setTimeSignature = useProjectStore(s => s.setTimeSignature);

  // Enumerate audio devices
  useEffect(() => {
    async function enumerateDevices() {
      try {
        // Request permissions first  
        await navigator.mediaDevices.getUserMedia({ audio: true }).then(s => s.getTracks().forEach(t => t.stop())).catch(() => {});
        const devices = await navigator.mediaDevices.enumerateDevices();
        const output = devices.filter(d => d.kind === 'audiooutput');
        const input = devices.filter(d => d.kind === 'audioinput');
        setAudioDevices({ output, input });
      } catch (e) {
        console.log('Could not enumerate devices:', e);
      }
    }
    enumerateDevices();

    // Enumerate MIDI devices
    async function enumerateMIDI() {
      try {
        if (navigator.requestMIDIAccess) {
          const midi = await navigator.requestMIDIAccess();
          const inputs = Array.from(midi.inputs.values()).map(i => ({ id: i.id, name: i.name, type: 'input' }));
          const outputs = Array.from(midi.outputs.values()).map(o => ({ id: o.id, name: o.name, type: 'output' }));
          setMidiDevices([...inputs, ...outputs]);
        }
      } catch (e) {
        console.log('MIDI not available:', e);
      }
    }
    enumerateMIDI();
  }, []);

  const settingsTabs = [
    { id: 'audio', label: 'Audio' },
    { id: 'midi', label: 'MIDI' },
    { id: 'project', label: 'Project' },
    { id: 'plugins', label: 'Plugins' },
  ];

  return (
    <>
      <div className="modal-header">
        <h2>Settings</h2>
        <button className="btn btn-icon btn-ghost" onClick={closeModal}>✕</button>
      </div>
      <div className="modal-body" style={{ alignItems: 'stretch', gap: 0, padding: 0 }}>
        {/* Tab bar */}
        <div style={{ display: 'flex', borderBottom: '1px solid var(--border-subtle)' }}>
          {settingsTabs.map(tab => (
            <button
              key={tab.id}
              className={`btn btn-sm btn-ghost ${activeSettingsTab === tab.id ? 'active' : ''}`}
              onClick={() => setActiveSettingsTab(tab.id)}
              style={{
                flex: 1, borderRadius: 0, borderBottom: activeSettingsTab === tab.id ? '2px solid var(--accent-primary)' : '2px solid transparent',
                padding: '8px 12px',
              }}
            >
              {tab.label}
            </button>
          ))}
        </div>

        <div style={{ padding: '12px 16px', maxHeight: 400, overflowY: 'auto' }}>
          {activeSettingsTab === 'audio' && (
            <div className="settings-section">
              <SettingsRow label="Output Device">
                <select className="input input-sm" value={selectedOutput} onChange={(e) => setSelectedOutput(e.target.value)}>
                  {audioDevices.output.length > 0 ? (
                    audioDevices.output.map(d => <option key={d.deviceId} value={d.deviceId}>{d.label || `Output ${d.deviceId.slice(0, 8)}`}</option>)
                  ) : (
                    <option value="default">System Default</option>
                  )}
                </select>
              </SettingsRow>
              <SettingsRow label="Input Device">
                <select className="input input-sm" value={selectedInput} onChange={(e) => setSelectedInput(e.target.value)}>
                  {audioDevices.input.length > 0 ? (
                    audioDevices.input.map(d => <option key={d.deviceId} value={d.deviceId}>{d.label || `Input ${d.deviceId.slice(0, 8)}`}</option>)
                  ) : (
                    <option value="default">System Default</option>
                  )}
                </select>
              </SettingsRow>
              <SettingsRow label="Sample Rate">
                <select className="input input-sm" value={sampleRate} onChange={(e) => setSampleRate(e.target.value)}>
                  <option value="44100">44,100 Hz (CD)</option>
                  <option value="48000">48,000 Hz (Video)</option>
                  <option value="96000">96,000 Hz (HD)</option>
                  <option value="192000">192,000 Hz (Studio)</option>
                </select>
              </SettingsRow>
              <SettingsRow label="Buffer Size">
                <select className="input input-sm" value={bufferSize} onChange={(e) => setBufferSize(e.target.value)}>
                  <option value="128">128 samples (2.9ms)</option>
                  <option value="256">256 samples (5.8ms)</option>
                  <option value="512">512 samples (11.6ms)</option>
                  <option value="1024">1024 samples (23.2ms)</option>
                  <option value="2048">2048 samples (46.4ms)</option>
                </select>
              </SettingsRow>
              <div className="text-muted" style={{ fontSize: 'var(--text-xs)', marginTop: 8 }}>
                Lower buffer sizes reduce latency but increase CPU. Use 512 or higher if experiencing audio glitches.
              </div>
            </div>
          )}

          {activeSettingsTab === 'midi' && (
            <div className="settings-section">
              <h3 className="settings-heading">Connected MIDI Devices</h3>
              {midiDevices.length > 0 ? (
                midiDevices.map(device => (
                  <div key={device.id} className="settings-device-item">
                    <span style={{ color: device.type === 'input' ? 'var(--accent-green)' : 'var(--accent-blue)' }}>
                      {device.type === 'input' ? '→' : '←'}
                    </span>
                    <span>{device.name}</span>
                    <span className="text-muted" style={{ fontSize: 'var(--text-xs)' }}>{device.type}</span>
                  </div>
                ))
              ) : (
                <div className="text-muted" style={{ padding: 20, textAlign: 'center', fontSize: 'var(--text-sm)' }}>
                  No MIDI devices detected.<br />Connect a MIDI controller and refresh.
                </div>
              )}
            </div>
          )}

          {activeSettingsTab === 'project' && (
            <div className="settings-section">
              <SettingsRow label="Project Name">
                <input
                  className="input input-sm"
                  value={projectName}
                  onChange={(e) => setProjectName(e.target.value)}
                  style={{ width: 200 }}
                />
              </SettingsRow>
              <SettingsRow label="Tempo (BPM)">
                <input
                  type="number"
                  className="input input-sm"
                  value={bpm}
                  min={20}
                  max={300}
                  onChange={(e) => setBpm(parseInt(e.target.value) || 120)}
                  style={{ width: 80 }}
                />
              </SettingsRow>
              <SettingsRow label="Time Signature">
                <div style={{ display: 'flex', gap: 4, alignItems: 'center' }}>
                  <input
                    type="number"
                    className="input input-sm"
                    value={timeSignature[0]}
                    min={1}
                    max={16}
                    onChange={(e) => setTimeSignature([parseInt(e.target.value) || 4, timeSignature[1]])}
                    style={{ width: 50 }}
                  />
                  <span>/</span>
                  <select
                    className="input input-sm"
                    value={timeSignature[1]}
                    onChange={(e) => setTimeSignature([timeSignature[0], parseInt(e.target.value)])}
                    style={{ width: 50 }}
                  >
                    <option value={2}>2</option>
                    <option value={4}>4</option>
                    <option value={8}>8</option>
                    <option value={16}>16</option>
                  </select>
                </div>
              </SettingsRow>
            </div>
          )}

          {activeSettingsTab === 'plugins' && (
            <div className="settings-section">
              <h3 className="settings-heading">VST Plugin Scan Paths</h3>
              {vstPaths.map((path, i) => (
                <div key={i} className="settings-device-item">
                  <span className="text-secondary" style={{ flex: 1, fontSize: 'var(--text-sm)' }}>{path}</span>
                  <button className="btn btn-sm btn-ghost" onClick={() => {
                    pluginManager.removeScanPath(path);
                    setVstPaths([...pluginManager.scanPaths]);
                  }}>✕</button>
                </div>
              ))}
              <div style={{ display: 'flex', gap: 6, marginTop: 8 }}>
                <input
                  className="input input-sm"
                  placeholder="C:\Program Files\VSTPlugins"
                  value={newPath}
                  onChange={(e) => setNewPath(e.target.value)}
                  style={{ flex: 1 }}
                />
                <button className="btn btn-sm" onClick={() => {
                  if (newPath.trim()) {
                    pluginManager.addScanPath(newPath.trim());
                    setVstPaths([...pluginManager.scanPaths]);
                    setNewPath('');
                  }
                }}>Add</button>
              </div>
              <div className="text-muted" style={{ fontSize: 'var(--text-xs)', marginTop: 8 }}>
                Add paths where your VST/VST2/VST3 plugins are installed. You can also drag .dll/.vst3 files directly into Orpheus.
              </div>
              <h3 className="settings-heading" style={{ marginTop: 16 }}>Imported Plugins ({pluginManager.importedPlugins.length})</h3>
              {pluginManager.importedPlugins.length === 0 ? (
                <div className="text-muted" style={{ fontSize: 'var(--text-sm)' }}>No external plugins imported yet.</div>
              ) : (
                pluginManager.importedPlugins.map(p => (
                  <div key={p.id} className="settings-device-item">
                    <span>{p.name}</span>
                    <span className="text-muted" style={{ fontSize: 'var(--text-xs)' }}>{p.format}</span>
                    <button className="btn btn-sm btn-ghost" onClick={() => {
                      pluginManager.removePlugin(p.id);
                      setVstPaths([...pluginManager.scanPaths]); // Trigger re-render
                    }}>✕</button>
                  </div>
                ))
              )}
            </div>
          )}
        </div>
      </div>
    </>
  );
}

function SettingsRow({ label, children }) {
  return (
    <div style={{
      display: 'flex', justifyContent: 'space-between', alignItems: 'center',
      padding: '6px 0', borderBottom: '1px solid var(--border-subtle)',
    }}>
      <span className="text-secondary" style={{ fontSize: 'var(--text-sm)' }}>{label}</span>
      {children}
    </div>
  );
}

// ─── Export (HD WAV) ───
function ExportContent({ closeModal, projectName }) {
  const [preset, setPreset] = useState('hd');
  const [sampleRate, setSampleRate] = useState(96000);
  const [bitDepth, setBitDepth] = useState(24);
  const [channels, setChannels] = useState(2);
  const [duration, setDuration] = useState(30);
  const [normalize, setNormalize] = useState(true);
  const [dither, setDither] = useState(false);
  const [chooseLocation, setChooseLocation] = useState(false);
  const [exporting, setExporting] = useState(false);
  const [progress, setProgress] = useState(0);
  const [exportDone, setExportDone] = useState(false);

  // Auto-calculate project duration
  useEffect(() => {
    const tracks = useProjectStore.getState().tracks;
    const bpm = useProjectStore.getState().bpm;
    let maxBeat = 0;
    
    tracks.forEach(t => {
        t.clips.forEach(c => {
            const end = c.startBeat + c.lengthBeats;
            if (end > maxBeat) maxBeat = end;
        });
    });
    
    // Add 1 bar buffer
    maxBeat += 4;
    
    const durationSec = Math.ceil((maxBeat / bpm) * 60);
    if (durationSec > 0) setDuration(durationSec);
  }, []);

  const applyPreset = (presetKey) => {
    setPreset(presetKey);
    const p = QUALITY_PRESETS[presetKey];
    if (p) {
      setSampleRate(p.sampleRate);
      setBitDepth(p.bitDepth);
    }
  };

  const handleExport = async () => {
    setExporting(true);
    setProgress(0);
    setExportDone(false);
    try {
      await audioExporter.exportWAV({
        sampleRate,
        bitDepth,
        channels,
        duration,
        normalize,
        dither,
        chooseLocation,
        fileName: projectName || 'Orpheus_Export',
        onProgress: (p) => setProgress(p),
      });
      setExportDone(true);
    } catch (err) {
      console.error('Export failed:', err);
    }
    setExporting(false);
  };

  return (
    <>
      <div className="modal-header">
        <h2>Export Audio</h2>
        <button className="btn btn-icon btn-ghost" onClick={closeModal}>✕</button>
      </div>
      <div className="modal-body" style={{ alignItems: 'stretch', gap: 10 }}>
        {/* Quality Presets */}
        <div>
          <label className="text-muted" style={{ fontSize: 'var(--text-xs)', display: 'block', marginBottom: 4 }}>QUALITY PRESET</label>
          <div style={{ display: 'flex', gap: 4 }}>
            {Object.entries(QUALITY_PRESETS).map(([key, val]) => (
              <button
                key={key}
                className={`btn btn-sm ${preset === key ? '' : 'btn-ghost'}`}
                onClick={() => applyPreset(key)}
                style={{ flex: 1, fontSize: 'var(--text-xs)' }}
              >
                {val.label}
              </button>
            ))}
          </div>
        </div>

        {/* Sample Rate */}
        <SettingsRow label="Sample Rate">
          <select value={sampleRate} onChange={(e) => setSampleRate(Number(e.target.value))} className="input input-sm">
            <option value={44100}>44,100 Hz</option>
            <option value={48000}>48,000 Hz</option>
            <option value={96000}>96,000 Hz</option>
            <option value={192000}>192,000 Hz</option>
          </select>
        </SettingsRow>

        {/* Bit Depth */}
        <SettingsRow label="Bit Depth">
          <select value={bitDepth} onChange={(e) => setBitDepth(Number(e.target.value))} className="input input-sm">
            <option value={16}>16-bit (CD)</option>
            <option value={24}>24-bit (HD)</option>
            <option value={32}>32-bit Float (Studio)</option>
          </select>
        </SettingsRow>

        {/* Duration (Min:Sec) */}
        <SettingsRow label="Duration">
          <div style={{ display: 'flex', gap: 4, alignItems: 'center' }}>
            <input
              type="number"
              className="input input-sm"
              value={Math.floor(duration / 60)}
              min={0}
              onChange={(e) => {
                const mins = Number(e.target.value);
                const secs = duration % 60;
                setDuration(mins * 60 + secs);
              }}
              style={{ width: 50, textAlign: 'right' }}
            />
            <span className="text-muted">:</span>
            <input
              type="number"
              className="input input-sm"
              value={duration % 60}
              min={0}
              max={59}
              onChange={(e) => {
                const secs = Number(e.target.value);
                const mins = Math.floor(duration / 60);
                setDuration(mins * 60 + secs);
              }}
              style={{ width: 50 }}
            />
            <span className="text-muted" style={{ fontSize: 'var(--text-xs)', marginLeft: 4 }}>
               ({duration}s)
            </span>
          </div>
        </SettingsRow>


        {/* Channels */}
        <SettingsRow label="Channels">
          <select value={channels} onChange={(e) => setChannels(Number(e.target.value))} className="input input-sm">
            <option value={1}>Mono</option>
            <option value={2}>Stereo</option>
          </select>
        </SettingsRow>

        {/* Options */}
        <div style={{ display: 'flex', gap: 16, paddingTop: 4 }}>
          <label style={{ display: 'flex', alignItems: 'center', gap: 6, cursor: 'pointer', fontSize: 'var(--text-sm)', color: 'var(--text-secondary)' }}>
            <input type="checkbox" checked={normalize} onChange={(e) => setNormalize(e.target.checked)} />
            Normalize
          </label>
          <label style={{ display: 'flex', alignItems: 'center', gap: 6, cursor: 'pointer', fontSize: 'var(--text-sm)', color: 'var(--text-secondary)' }}>
            <input type="checkbox" checked={dither} onChange={(e) => setDither(e.target.checked)} />
            Dither
          </label>
          {audioExporter.hasFilePicker && (
            <label style={{ display: 'flex', alignItems: 'center', gap: 6, cursor: 'pointer', fontSize: 'var(--text-sm)', color: 'var(--text-secondary)' }}>
              <input type="checkbox" checked={chooseLocation} onChange={(e) => setChooseLocation(e.target.checked)} />
              Choose Location
            </label>
          )}
        </div>

        {/* Progress */}
        {(exporting || exportDone) && (
          <div style={{ marginTop: 4 }}>
            <div className="stem-progress-bar" style={{ height: 6 }}>
              <div className="stem-progress-fill" style={{ width: `${progress * 100}%`, transition: 'width 200ms ease' }} />
            </div>
            <p className="text-muted text-sm" style={{ textAlign: 'center', marginTop: 4 }}>
              {exportDone ? '✓ Export complete!' : `Rendering... ${Math.round(progress * 100)}%`}
            </p>
          </div>
        )}

        {/* File info */}
        <div className="text-muted" style={{ fontSize: 'var(--text-xs)', borderTop: '1px solid var(--border-subtle)', paddingTop: 8 }}>
          Estimated file size: ~{((sampleRate * (bitDepth / 8) * channels * duration) / (1024 * 1024)).toFixed(1)} MB
          <br />Format: WAV ({bitDepth}-bit, {(sampleRate/1000).toFixed(1)}kHz, {channels === 1 ? 'Mono' : 'Stereo'})
        </div>

        {/* Actions */}
        <div style={{ display: 'flex', gap: 8, justifyContent: 'flex-end' }}>
          <button className="btn btn-sm btn-ghost" onClick={closeModal} disabled={exporting}>Cancel</button>
          <button
            className="btn btn-sm"
            onClick={handleExport}
            disabled={exporting}
          >
            {exporting ? 'Exporting...' : chooseLocation ? 'Export & Save As...' : 'Export WAV'}
          </button>
        </div>
      </div>
    </>
  );
}

// ─── Keyboard Shortcuts ───
function ShortcutsContent({ closeModal }) {
  return (
    <>
      <div className="modal-header">
        <h2>Keyboard Shortcuts</h2>
        <button className="btn btn-icon btn-ghost" onClick={closeModal}>✕</button>
      </div>
      <div className="modal-body" style={{ alignItems: 'stretch', maxHeight: 420, overflowY: 'auto', gap: 12, paddingRight: 4 }}>
        {SHORTCUT_GROUPS.map(group => (
          <div key={group.title}>
            <h3 style={{
              fontSize: 'var(--text-xs)',
              color: 'var(--text-muted)',
              textTransform: 'uppercase',
              letterSpacing: 1.5,
              marginBottom: 4,
            }}>{group.title}</h3>
            <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: '2px 16px' }}>
              {group.items.map(([key, label]) => (
                <div key={key} style={{
                  display: 'flex', justifyContent: 'space-between',
                  padding: '3px 0', fontSize: 'var(--text-sm)',
                }}>
                  <span className="text-secondary">{label}</span>
                  <kbd style={{
                    background: 'var(--bg-active)',
                    padding: '1px 6px',
                    borderRadius: 3,
                    fontSize: 'var(--text-xs)',
                    fontFamily: 'var(--font-mono)',
                    color: 'var(--text-primary)',
                    border: '1px solid var(--border-subtle)',
                  }}>{key}</kbd>
                </div>
              ))}
            </div>
          </div>
        ))}
      </div>
    </>
  );
}
