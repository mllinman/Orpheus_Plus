// ============================================
// ORPHEUS DAW — UI Store (Zustand)
// ============================================

import { create } from 'zustand';
import { persist } from 'zustand/middleware';

export const useUIStore = create(
    persist(
        (set, get) => ({
            // View modes
            activeView: 'arrangement', // arrangement | mixer | pianoroll
            showBrowser: false,
            showMixer: true,
            showPianoRoll: false,
            showStemSeparation: false,
            showMastering: false,
            showAutotune: false,

            // Panel sizes
            browserWidth: 250,
            mixerHeight: 280,
            pianoRollHeight: 300,
            trackHeaderWidth: 220,

            // Zoom
            horizontalZoom: 40, // pixels per beat
            verticalZoom: 1,

            // Selection
            selectedTrackId: null,
            selectedClipId: null,
            selectedClipTrackId: null,
            selectedNotes: [],

            // Tool
            activeTool: 'pointer', // pointer | range | draw | split | erase | automation | mute | smart | razor | stretch | slip
            snapEnabled: true,
            snapValue: 1, // beats (1 = quarter note)

            // Context menu
            contextMenu: null, // { x, y, items }

            // Modal
            activeModal: null, // 'settings' | 'export' | 'about' | 'hotkeys' | 'templates' | null

            // Panels
            showUndoHistory: false,
            showSessionView: false,

            // Track Icons
            trackIcons: {}, // { trackId: emoji }

            // Custom Hotkeys
            customHotkeys: {}, // { keyCombo: actionName }

            // Playhead animation
            isAnimating: false,

            // Actions
            setActiveView: (view) => set({ activeView: view }),
            toggleBrowser: () => set((s) => ({ showBrowser: !s.showBrowser })),
            toggleMixer: () => set((s) => ({ showMixer: !s.showMixer })),
            togglePianoRoll: () => set((s) => ({ showPianoRoll: !s.showPianoRoll })),
            toggleStemSeparation: () => set((s) => ({ showStemSeparation: !s.showStemSeparation })),
            toggleMastering: () => set((s) => ({ showMastering: !s.showMastering })),
            toggleAutotune: () => set((s) => ({ showAutotune: !s.showAutotune })),

            setBrowserWidth: (w) => set({ browserWidth: Math.max(200, Math.min(500, w)) }),
            setMixerHeight: (h) => set({ mixerHeight: Math.max(150, Math.min(500, h)) }),
            setPianoRollHeight: (h) => set({ pianoRollHeight: Math.max(150, Math.min(600, h)) }),
            setTrackHeaderWidth: (w) => set({ trackHeaderWidth: Math.max(150, Math.min(400, w)) }),

            setHorizontalZoom: (z) => set({ horizontalZoom: Math.max(10, Math.min(200, z)) }),
            setVerticalZoom: (z) => set({ verticalZoom: Math.max(0.5, Math.min(3, z)) }),
            zoomIn: () => set((s) => ({ horizontalZoom: Math.min(200, s.horizontalZoom * 1.2) })),
            zoomOut: () => set((s) => ({ horizontalZoom: Math.max(10, s.horizontalZoom / 1.2) })),

            setSelectedTrack: (id) => set({ selectedTrackId: id }),
            setSelectedClip: (trackId, clipId) => set({ selectedClipId: clipId, selectedClipTrackId: trackId }),
            clearSelection: () => set({ selectedTrackId: null, selectedClipId: null, selectedClipTrackId: null, selectedNotes: [] }),

            setActiveTool: (tool) => set({ activeTool: tool }),
            setSnapEnabled: (v) => set({ snapEnabled: v }),
            setSnapValue: (v) => set({ snapValue: v }),

            showContextMenu: (x, y, items) => set({ contextMenu: { x, y, items } }),
            hideContextMenu: () => set({ contextMenu: null }),

            setActiveModal: (modal) => set({ activeModal: modal }),
            closeModal: () => set({ activeModal: null }),

            setIsAnimating: (v) => set({ isAnimating: v }),

            // Phase 24 toggles
            toggleUndoHistory: () => set(s => ({ showUndoHistory: !s.showUndoHistory })),
            toggleSessionView: () => set(s => ({ showSessionView: !s.showSessionView })),

            setTrackIcon: (trackId, emoji) => set(s => ({ trackIcons: { ...s.trackIcons, [trackId]: emoji } })),

            setCustomHotkey: (keyCombo, action) => set(s => {
                const updated = { ...s.customHotkeys, [keyCombo]: action };
                localStorage.setItem('orpheus_hotkeys', JSON.stringify(updated));
                return { customHotkeys: updated };
            }),

            removeCustomHotkey: (keyCombo) => set(s => {
                const updated = { ...s.customHotkeys };
                delete updated[keyCombo];
                localStorage.setItem('orpheus_hotkeys', JSON.stringify(updated));
                return { customHotkeys: updated };
            }),

            loadCustomHotkeys: () => {
                const saved = localStorage.getItem('orpheus_hotkeys');
                if (saved) set({ customHotkeys: JSON.parse(saved) });
            },

            zoomToFit: () => {
                // Calculate optimal zoom to show all clips
                const store = require('./projectStore').useProjectStore?.getState?.();
                if (!store) return;
                let maxBeat = 32;
                for (const t of store.tracks) {
                    for (const c of t.clips) {
                        maxBeat = Math.max(maxBeat, c.startBeat + c.lengthBeats);
                    }
                }
                // Assume ~1200px viewport width
                const targetZoom = Math.max(10, Math.min(200, 1200 / maxBeat));
                set({ horizontalZoom: targetZoom });
            },
        }),
        {
            name: 'orpheus-ui',
            version: 1,
            partialize: (state) => {
                // Exclude ephemeral state
                const { contextMenu, isAnimating, ...persistable } = state;
                return persistable;
            },
        }
    )
);
