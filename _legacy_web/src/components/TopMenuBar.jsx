import React, { useState, useRef, useEffect, useCallback } from 'react';
import { useProjectStore } from '../stores/projectStore';
import { useUIStore } from '../stores/uiStore';
import { audioEngine } from '../audio/AudioEngine';

const MENUS = {
  File: [
    { label: 'New Project', shortcut: 'Ctrl+N', action: 'newProject' },
    { label: 'Open Project...', shortcut: 'Ctrl+O', action: 'openProject' },
    { label: 'Open from Cloud...', action: 'openCloud' },
    { divider: true },
    { label: 'Save (Local)', shortcut: 'Ctrl+S', action: 'save' },
    { label: 'Save to Cloud...', action: 'saveCloud' },
    { label: 'Save As (Download)...', shortcut: 'Ctrl+Shift+S', action: 'saveAs' },
    { divider: true },
    { label: 'Export Audio...', shortcut: 'Ctrl+Shift+E', action: 'export' },
    { label: 'Export Project File...', action: 'exportProject' },
    { divider: true },
    { label: 'Templates...', action: 'templates' },
    { label: 'Project Settings...', action: 'settings' },
  ],
  Edit: [
    { label: 'Undo', shortcut: 'Ctrl+Z', action: 'undo' },
    { label: 'Redo', shortcut: 'Ctrl+Shift+Z', action: 'redo' },
    { label: 'Undo History', action: 'undoHistory' },
    { divider: true },
    { label: 'Cut', shortcut: 'Ctrl+X', action: 'cut' },
    { label: 'Copy', shortcut: 'Ctrl+C', action: 'copy' },
    { label: 'Paste', shortcut: 'Ctrl+V', action: 'paste' },
    { label: 'Delete', shortcut: 'Del', action: 'delete' },
    { divider: true },
    { label: 'Select All', shortcut: 'Ctrl+A', action: 'selectAll' },
    { label: 'Deselect All', shortcut: 'Ctrl+D', action: 'deselectAll' },
    { divider: true },
    { label: 'Quantize', shortcut: 'Q', action: 'quantize' },
    { label: 'Glue Selected Clips', shortcut: 'Ctrl+G', action: 'glue' },
  ],
  View: [
    { label: 'Arrangement', shortcut: 'F1', action: 'viewArrangement' },
    { label: 'Mixer', shortcut: 'F2', action: 'viewMixer' },
    { label: 'Piano Roll', shortcut: 'F3', action: 'viewPianoRoll' },
    { label: 'Browser', shortcut: 'F4', action: 'viewBrowser' },
    { divider: true },
    { label: 'STEM Separation', shortcut: 'F5', action: 'viewStemSeparation' },
    { label: 'Mastering', shortcut: 'F6', action: 'viewMastering' },
    { label: 'Autotune', shortcut: 'F7', action: 'viewAutotune' },
    { divider: true },
    { label: 'Zoom In', shortcut: 'Ctrl++', action: 'zoomIn' },
    { label: 'Zoom Out', shortcut: 'Ctrl+-', action: 'zoomOut' },
    { label: 'Zoom to Fit', shortcut: 'Ctrl+0', action: 'zoomFit' },
  ],
  Track: [
    { label: 'Add Audio Track', shortcut: 'Ctrl+T', action: 'addAudio' },
    { label: 'Add MIDI Track', shortcut: 'Ctrl+Shift+T', action: 'addMidi' },
    { label: 'Add Instrument Track', action: 'addInstrument' },
    { divider: true },
    { label: 'Duplicate Track', action: 'duplicateTrack' },
    { label: 'Remove Track', action: 'removeTrack' },
    { divider: true },
    { label: 'Add Track Folder', action: 'addFolder' },
    { label: 'Add Automation Lane', action: 'addAutomation' },
    { label: 'Freeze Track', action: 'freezeTrack' },
    { label: 'Unfreeze Track', action: 'unfreezeTrack' },
  ],
  Transport: [
    { label: 'Play / Stop', shortcut: 'Space', action: 'playStop' },
    { label: 'Record', shortcut: 'R', action: 'record' },
    { label: 'Loop', shortcut: 'L', action: 'loop' },
    { divider: true },
    { label: 'Metronome', shortcut: 'M', action: 'metronome' },
    { label: 'Tap Tempo', shortcut: 'T', action: 'tapTempo' },
    { divider: true },
    { label: 'Go to Start', shortcut: 'Home', action: 'goToStart' },
    { label: 'Go to End', shortcut: 'End', action: 'goToEnd' },
  ],
  Help: [
    { label: 'Keyboard Shortcuts', action: 'shortcuts' },
    { label: 'Customize Hotkeys...', action: 'hotkeys' },
    { label: 'Documentation', action: 'docs' },
    { divider: true },
    { label: 'About Orpheus', action: 'about' },
  ],
};

export default function TopMenuBar() {
  const [openMenu, setOpenMenu] = useState(null);
  const menuRef = useRef(null);
  const fileInputRef = useRef(null);
  const projectName = useProjectStore(s => s.projectName);
  const clipboard = useRef(null); // For cut/copy/paste

  // UI Store
  const {
    setActiveView, toggleMixer, togglePianoRoll, toggleBrowser,
    toggleStemSeparation, toggleMastering, toggleAutotune,
    zoomIn, zoomOut, setHorizontalZoom, setActiveModal,
    selectedTrackId, selectedClipId, selectedClipTrackId, clearSelection
  } = useUIStore();

  // Project Store
  const {
    addTrack, saveProject, exportProject, newProject,
    undo, redo, removeTrack, duplicateTrack, removeClip,
    importProject, addAutomationLane, tracks, bpm,
    isPlaying, setPlaying, setRecording, toggleLoop, setPlayheadPosition
  } = useProjectStore();

  // Close menu when clicking outside
  useEffect(() => {
    const handleClick = (e) => {
      if (menuRef.current && !menuRef.current.contains(e.target)) {
        setOpenMenu(null);
      }
    };
    // Use 'click' (not 'mousedown') so dropdown items fire their onMouseDown first
    document.addEventListener('click', handleClick);
    return () => document.removeEventListener('click', handleClick);
  }, []);

  // Tap tempo state
  const tapTimes = useRef([]);

  const handleAction = useCallback((action) => {
    setOpenMenu(null);
    const proj = useProjectStore.getState();
    const ui = useUIStore.getState();

    switch (action) {
      // ─── File ───
      case 'newProject':
        if (confirm('Create a new project? Unsaved changes will be lost.')) {
          audioEngine.stop();
          proj.setPlaying(false);
          proj.newProject();
        }
        break;

      case 'openProject':
        // Trigger hidden file input
        if (fileInputRef.current) fileInputRef.current.click();
        break;

      case 'save':
        proj.saveProject();
        break;

      case 'saveAs':
        // Download the project as a .orpheus file
        proj.exportProject();
        break;

      case 'openCloud':
        setActiveModal('cloud');
        break;

      case 'saveCloud':
        setActiveModal('cloudSave');
        break;

      case 'export':
        setActiveModal('export');
        break;

      case 'exportProject':
        proj.exportProject();
        break;

      case 'settings':
        setActiveModal('settings');
        break;

      // ─── Edit ───
      case 'undo':
        proj.undo();
        break;

      case 'redo':
        proj.redo();
        break;

      case 'cut':
        // Cut = copy + delete the selected clip
        if (ui.selectedClipId && ui.selectedClipTrackId) {
          const track = proj.tracks.find(t => t.id === ui.selectedClipTrackId);
          const clip = track?.clips.find(c => c.id === ui.selectedClipId);
          if (clip) {
            clipboard.current = JSON.parse(JSON.stringify(clip));
            proj.removeClip(ui.selectedClipTrackId, ui.selectedClipId);
            ui.clearSelection();
          }
        }
        break;

      case 'copy':
        if (ui.selectedClipId && ui.selectedClipTrackId) {
          const track = proj.tracks.find(t => t.id === ui.selectedClipTrackId);
          const clip = track?.clips.find(c => c.id === ui.selectedClipId);
          if (clip) {
            clipboard.current = JSON.parse(JSON.stringify(clip));
          }
        }
        break;

      case 'paste':
        if (clipboard.current) {
          const targetTrackId = ui.selectedTrackId || (proj.tracks[0]?.id);
          if (targetTrackId) {
            const newClip = {
              ...clipboard.current,
              id: Date.now().toString(36) + Math.random().toString(36).slice(2),
              startBeat: clipboard.current.startBeat + clipboard.current.lengthBeats,
            };
            proj.addClip(targetTrackId, newClip);
          }
        }
        break;

      case 'delete':
        if (ui.selectedClipId && ui.selectedClipTrackId) {
          proj.removeClip(ui.selectedClipTrackId, ui.selectedClipId);
          ui.clearSelection();
        } else if (ui.selectedTrackId) {
          proj.removeTrack(ui.selectedTrackId);
          ui.clearSelection();
        }
        break;

      case 'selectAll':
        // Select first track if none selected
        if (proj.tracks.length > 0 && !ui.selectedTrackId) {
          ui.setSelectedTrack(proj.tracks[0].id);
        }
        break;

      case 'deselectAll':
        ui.clearSelection();
        break;

      case 'quantize':
        proj.quantizeSelection();
        break;

      case 'glue':
        if (ui.selectedClipId && ui.selectedClipTrackId) {
          // Glue selected clip with adjacent clips
          proj.glueClips(ui.selectedClipTrackId, [ui.selectedClipId]);
        }
        break;

      case 'undoHistory':
        ui.toggleUndoHistory();
        break;

      // ─── View ───
      case 'viewArrangement':
        setActiveView('arrangement');
        break;
      case 'viewMixer':
        toggleMixer();
        break;
      case 'viewPianoRoll':
        togglePianoRoll();
        break;
      case 'viewBrowser':
        toggleBrowser();
        break;
      case 'viewStemSeparation':
        toggleStemSeparation();
        break;
      case 'viewMastering':
        toggleMastering();
        break;
      case 'viewAutotune':
        toggleAutotune();
        break;
      case 'zoomIn':
        zoomIn();
        break;
      case 'zoomOut':
        zoomOut();
        break;
      case 'zoomFit':
        ui.zoomToFit();
        break;

      // ─── Track ───
      case 'addAudio':
        proj.addTrack('audio');
        break;
      case 'addMidi':
        proj.addTrack('midi');
        break;
      case 'addInstrument':
        proj.addTrack('midi');
        break;
      case 'duplicateTrack':
        if (ui.selectedTrackId) {
          proj.duplicateTrack(ui.selectedTrackId);
        } else if (proj.tracks.length > 0) {
          proj.duplicateTrack(proj.tracks[proj.tracks.length - 1].id);
        }
        break;
      case 'removeTrack':
        if (ui.selectedTrackId) {
          proj.removeTrack(ui.selectedTrackId);
          ui.clearSelection();
        }
        break;
      case 'addAutomation':
        if (ui.selectedTrackId) {
          proj.addAutomationLane(ui.selectedTrackId);
        } else if (proj.tracks.length > 0) {
          proj.addAutomationLane(proj.tracks[0].id);
        }
        break;
      case 'addFolder':
        proj.addTrackFolder();
        break;
      case 'freezeTrack':
        if (ui.selectedTrackId) {
          proj.freezeTrack(ui.selectedTrackId);
        }
        break;
      case 'unfreezeTrack':
        if (ui.selectedTrackId) {
          proj.unfreezeTrack(ui.selectedTrackId);
        }
        break;
      case 'templates':
        ui.setActiveModal('templates');
        break;

      // ─── Transport ───
      case 'playStop':
        (async () => {
          if (proj.isPlaying) {
            audioEngine.stop();
            proj.setPlaying(false);
            proj.setPlayheadPosition(0);
          } else {
            await audioEngine.init();
            audioEngine.setBPM(proj.bpm);
            audioEngine.play();
            proj.setPlaying(true);
          }
        })();
        break;

      case 'record':
        (async () => {
          await audioEngine.init();
          audioEngine.toggleRecord();
          proj.setRecording(audioEngine.isRecording);
          if (!proj.isPlaying && audioEngine.isRecording) {
            audioEngine.play();
            proj.setPlaying(true);
          }
        })();
        break;

      case 'loop':
        proj.toggleLoop();
        audioEngine.toggleLoop();
        break;

      case 'metronome':
        audioEngine.toggleMetronome();
        break;

      case 'tapTempo': {
        const now = Date.now();
        tapTimes.current.push(now);
        // Keep only last 8 taps
        if (tapTimes.current.length > 8) tapTimes.current.shift();
        if (tapTimes.current.length >= 2) {
          const intervals = [];
          for (let i = 1; i < tapTimes.current.length; i++) {
            intervals.push(tapTimes.current[i] - tapTimes.current[i - 1]);
          }
          const avgInterval = intervals.reduce((a, b) => a + b, 0) / intervals.length;
          const tappedBpm = Math.round(60000 / avgInterval);
          if (tappedBpm >= 20 && tappedBpm <= 300) {
            proj.setBpm(tappedBpm);
            audioEngine.setBPM(tappedBpm);
          }
        }
        // Reset taps if >2 seconds gap
        setTimeout(() => {
          if (tapTimes.current.length > 0 && Date.now() - tapTimes.current[tapTimes.current.length - 1] > 2000) {
            tapTimes.current = [];
          }
        }, 2500);
        break;
      }

      case 'goToStart':
        audioEngine.seekTo(0);
        proj.setPlayheadPosition(0);
        break;

      case 'goToEnd': {
        // Find the last beat in the project
        let maxBeat = 32;
        proj.tracks.forEach(t => {
          t.clips.forEach(c => {
            const end = c.startBeat + c.lengthBeats;
            if (end > maxBeat) maxBeat = end;
          });
        });
        const endTime = audioEngine.beatToTime(maxBeat);
        audioEngine.seekTo(endTime);
        proj.setPlayheadPosition(maxBeat);
        break;
      }

      // ─── Help ───
      case 'shortcuts':
        setActiveModal('shortcuts');
        break;
      case 'docs':
        // Open docs in a new tab (placeholder)
        window.open('https://github.com', '_blank');
        break;
      case 'about':
        setActiveModal('about');
        break;
      case 'hotkeys':
        ui.setActiveModal('hotkeys');
        break;

      default:
        console.log('Unhandled action:', action);
    }
  }, []);

  // Handle file import
  const handleFileImport = async (e) => {
    const file = e.target.files?.[0];
    if (!file) return;
    try {
      await useProjectStore.getState().importProject(file);
    } catch (err) {
      alert('Failed to open project: ' + err.message);
    }
    // Reset input
    e.target.value = '';
  };

  return (
    <div className="top-menu-bar" ref={menuRef}>
      <div className="menu-logo">
        <img src="/orpheus-logo.png" alt="Orpheus" className="logo-img" />
      </div>
      <div className="menu-items">
        {Object.entries(MENUS).map(([menuName, items]) => (
          <div
            key={menuName}
            className={`menu-trigger ${openMenu === menuName ? 'open' : ''}`}
            onClick={(e) => { e.stopPropagation(); setOpenMenu(openMenu === menuName ? null : menuName); }}
            onMouseEnter={() => openMenu && setOpenMenu(menuName)}
          >
            {menuName}
            {openMenu === menuName && (
              <div className="dropdown-menu" onClick={(e) => e.stopPropagation()}>
                {items.map((item, i) =>
                  item.divider ? (
                    <div key={i} className="dropdown-divider" />
                  ) : (
                    <div
                      key={i}
                      className="dropdown-item"
                      onMouseDown={(e) => { e.stopPropagation(); handleAction(item.action); }}
                    >
                      <span>{item.label}</span>
                      {item.shortcut && <span className="shortcut">{item.shortcut}</span>}
                    </div>
                  )
                )}
              </div>
            )}
          </div>
        ))}
      </div>
      <div className="menu-project-name">{projectName}</div>
      <div className="menu-spacer" />
      <div className="menu-indicators">
        <span className="cpu-indicator" data-tooltip="CPU Load">
          <span className="indicator-label">CPU</span>
          <span className="indicator-bar"><span className="indicator-fill" style={{ width: '12%' }} /></span>
        </span>
      </div>

      {/* Hidden file input for Open Project */}
      <input
        ref={fileInputRef}
        type="file"
        accept=".orpheus,.json"
        style={{ display: 'none' }}
        onChange={handleFileImport}
      />
    </div>
  );
}
