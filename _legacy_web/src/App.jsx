import React, { useEffect } from 'react';
import './styles/global.css';
import './styles/components.css';
import './styles/layout.css';
import TopMenuBar from './components/TopMenuBar';
import Toolbar from './components/Toolbar';
import TransportBar from './components/TransportBar';
import ArrangementView from './components/arrangement/ArrangementView';
import MixerView from './components/mixer/MixerView';
import PianoRollView from './components/pianoroll/PianoRollView';
import BrowserPanel from './components/browser/BrowserPanel';
import StemSeparationPanel from './components/stems/StemSeparationPanel';
import MasteringPanel from './components/mastering/MasteringPanel';
import AutotunePanel from './components/autotune/AutotunePanel';
import ContextMenu from './components/shared/ContextMenu';
import Modal from './components/shared/Modal';
import DragDropOverlay from './components/shared/DragDropOverlay';
import UndoHistoryPanel from './components/shared/UndoHistoryPanel';
import HotkeyEditor from './components/modals/HotkeyEditor';
import TemplateManager from './components/modals/TemplateManager';
import { useUIStore } from './stores/uiStore';
import { useProjectStore } from './stores/projectStore';
import { setupKeyboardShortcuts } from './utils/keyboardShortcuts';
import { audioEngine } from './audio/AudioEngine';

export default function App() {
  const {
    showBrowser, showMixer, showPianoRoll,
    showStemSeparation, showMastering, showAutotune,
    browserWidth, mixerHeight, pianoRollHeight,
    activeModal
  } = useUIStore();

  useEffect(() => {
    setupKeyboardShortcuts();
    audioEngine.setStoreRef(() => useProjectStore.getState());
    // Start auto-save
    useProjectStore.getState().startAutoSave();
    // Load saved hotkeys
    useUIStore.getState().loadCustomHotkeys();
    return () => useProjectStore.getState().stopAutoSave();
  }, []);

  const hasRightPanel = showStemSeparation || showMastering || showAutotune;

  return (
    <div className="app-shell">
      {/* Top Menu */}
      <TopMenuBar />

      {/* Toolbar */}
      <Toolbar />

      {/* Main Content Area */}
      <div className="main-content">
        {/* Browser Panel (Left) */}
        {showBrowser && (
          <div className="browser-container" style={{ width: browserWidth }}>
            <BrowserPanel />
          </div>
        )}

        {/* Center: Arrangement + Bottom Panels */}
        <div className="center-content">
          {/* Arrangement View */}
          <div className="arrangement-container">
            <ArrangementView />
          </div>

          {/* Piano Roll (Bottom Panel) */}
          {showPianoRoll && (
            <>
              <div className="resize-handle resize-handle-h" />
              <div className="pianoroll-container" style={{ height: pianoRollHeight }}>
                <PianoRollView />
              </div>
            </>
          )}

          {/* Mixer (Bottom Panel) */}
          {showMixer && (
            <>
              <div className="resize-handle resize-handle-h" />
              <div className="mixer-container" style={{ height: mixerHeight }}>
                <MixerView />
              </div>
            </>
          )}
        </div>

        {/* Right Panels: STEM / Mastering / Autotune */}
        {hasRightPanel && (
          <div className="right-panels">
            {showStemSeparation && <StemSeparationPanel />}
            {showMastering && <MasteringPanel />}
            {showAutotune && <AutotunePanel />}
          </div>
        )}
      </div>

      {/* Transport Bar */}
      <TransportBar />

      {/* Overlays */}
      <ContextMenu />
      <Modal />
      <DragDropOverlay />
      <UndoHistoryPanel />

      {/* Modal Editors */}
      {activeModal === 'hotkeys' && <HotkeyEditor />}
      {activeModal === 'templates' && <TemplateManager />}
    </div>
  );
}
