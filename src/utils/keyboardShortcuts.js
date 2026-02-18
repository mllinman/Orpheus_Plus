// ============================================
// ORPHEUS DAW — Keyboard Shortcuts
// ============================================

import { audioEngine } from '../audio/AudioEngine';
import { useProjectStore } from '../stores/projectStore';
import { useUIStore } from '../stores/uiStore';

// Clipboard storage for cut/copy/paste
let clipboard = null;

export function setupKeyboardShortcuts() {
    document.addEventListener('keydown', (e) => {
        // Don't handle shortcuts when typing in inputs
        if (e.target.tagName === 'INPUT' || e.target.tagName === 'TEXTAREA' || e.target.isContentEditable) {
            return;
        }

        const ctrl = e.ctrlKey || e.metaKey;
        const shift = e.shiftKey;

        switch (e.code) {
            // ─── Transport ───
            case 'Space':
                e.preventDefault();
                if (useProjectStore.getState().isPlaying) {
                    audioEngine.stop();
                    useProjectStore.getState().setPlaying(false);
                    useProjectStore.getState().setPlayheadPosition(0);
                } else {
                    audioEngine.init().then(() => {
                        audioEngine.setBPM(useProjectStore.getState().bpm);
                        audioEngine.play();
                        useProjectStore.getState().setPlaying(true);
                    });
                }
                break;

            case 'KeyR':
                if (!ctrl) {
                    e.preventDefault();
                    audioEngine.init().then(() => {
                        audioEngine.toggleRecord();
                        useProjectStore.getState().setRecording(audioEngine.isRecording);
                        if (!useProjectStore.getState().isPlaying && audioEngine.isRecording) {
                            audioEngine.play();
                            useProjectStore.getState().setPlaying(true);
                        }
                    });
                }
                break;

            case 'KeyL':
                if (!ctrl) {
                    e.preventDefault();
                    useProjectStore.getState().toggleLoop();
                    audioEngine.toggleLoop();
                }
                break;

            case 'KeyM':
                if (!ctrl && !shift) {
                    e.preventDefault();
                    audioEngine.toggleMetronome();
                }
                break;

            case 'Home':
                e.preventDefault();
                audioEngine.seekTo(0);
                useProjectStore.getState().setPlayheadPosition(0);
                break;

            case 'End': {
                e.preventDefault();
                let maxBeat = 32;
                useProjectStore.getState().tracks.forEach(t => {
                    t.clips.forEach(c => {
                        const end = c.startBeat + c.lengthBeats;
                        if (end > maxBeat) maxBeat = end;
                    });
                });
                const endTime = audioEngine.beatToTime(maxBeat);
                audioEngine.seekTo(endTime);
                useProjectStore.getState().setPlayheadPosition(maxBeat);
                break;
            }

            // ─── Zoom ───
            case 'Equal':
            case 'NumpadAdd':
                if (ctrl) {
                    e.preventDefault();
                    useUIStore.getState().zoomIn();
                }
                break;

            case 'Minus':
            case 'NumpadSubtract':
                if (ctrl) {
                    e.preventDefault();
                    useUIStore.getState().zoomOut();
                }
                break;

            case 'Digit0':
                if (ctrl) {
                    e.preventDefault();
                    useUIStore.getState().setHorizontalZoom(40); // Zoom to fit
                }
                break;

            // ─── Undo/Redo ───
            case 'KeyZ':
                if (ctrl && shift) {
                    e.preventDefault();
                    useProjectStore.getState().redo();
                } else if (ctrl) {
                    e.preventDefault();
                    useProjectStore.getState().undo();
                }
                break;

            // ─── Cut/Copy/Paste ───
            case 'KeyX':
                if (ctrl) {
                    e.preventDefault();
                    const ui1 = useUIStore.getState();
                    const proj1 = useProjectStore.getState();
                    if (ui1.selectedClipId && ui1.selectedClipTrackId) {
                        const track = proj1.tracks.find(t => t.id === ui1.selectedClipTrackId);
                        const clip = track?.clips.find(c => c.id === ui1.selectedClipId);
                        if (clip) {
                            clipboard = JSON.parse(JSON.stringify(clip));
                            proj1.removeClip(ui1.selectedClipTrackId, ui1.selectedClipId);
                            ui1.clearSelection();
                        }
                    }
                }
                break;

            case 'KeyC':
                if (ctrl) {
                    e.preventDefault();
                    const ui2 = useUIStore.getState();
                    const proj2 = useProjectStore.getState();
                    if (ui2.selectedClipId && ui2.selectedClipTrackId) {
                        const track = proj2.tracks.find(t => t.id === ui2.selectedClipTrackId);
                        const clip = track?.clips.find(c => c.id === ui2.selectedClipId);
                        if (clip) {
                            clipboard = JSON.parse(JSON.stringify(clip));
                        }
                    }
                }
                break;

            case 'KeyV':
                if (ctrl) {
                    e.preventDefault();
                    if (clipboard) {
                        const ui3 = useUIStore.getState();
                        const proj3 = useProjectStore.getState();
                        const targetTrackId = ui3.selectedTrackId || (proj3.tracks[0]?.id);
                        if (targetTrackId) {
                            const newClip = {
                                ...clipboard,
                                id: Date.now().toString(36) + Math.random().toString(36).slice(2),
                                startBeat: clipboard.startBeat + clipboard.lengthBeats,
                            };
                            proj3.addClip(targetTrackId, newClip);
                        }
                    }
                }
                break;

            case 'KeyA':
                if (ctrl) {
                    e.preventDefault();
                    const proj4 = useProjectStore.getState();
                    if (proj4.tracks.length > 0) {
                        useUIStore.getState().setSelectedTrack(proj4.tracks[0].id);
                    }
                }
                break;

            case 'KeyD':
                if (ctrl && !shift) {
                    e.preventDefault();
                    useUIStore.getState().clearSelection();
                }
                break;

            // ─── File Operations ───
            case 'KeyN':
                if (ctrl) {
                    e.preventDefault();
                    if (confirm('Create a new project? Unsaved changes will be lost.')) {
                        audioEngine.stop();
                        useProjectStore.getState().setPlaying(false);
                        useProjectStore.getState().newProject();
                    }
                }
                break;

            case 'KeyO':
                if (ctrl) {
                    e.preventDefault();
                    // Trigger file input — find and click it
                    const fileInput = document.querySelector('input[type="file"][accept*=".orpheus"]');
                    if (fileInput) fileInput.click();
                }
                break;

            case 'KeyS':
                if (ctrl && shift) {
                    e.preventDefault();
                    useProjectStore.getState().exportProject(); // Save As = download file
                } else if (ctrl) {
                    e.preventDefault();
                    useProjectStore.getState().saveProject();
                }
                break;

            case 'KeyE':
                if (ctrl && shift) {
                    e.preventDefault();
                    useUIStore.getState().setActiveModal('export');
                }
                break;

            // ─── Views ───
            case 'F1':
                e.preventDefault();
                useUIStore.getState().setActiveView('arrangement');
                break;
            case 'F2':
                e.preventDefault();
                useUIStore.getState().toggleMixer();
                break;
            case 'F3':
                e.preventDefault();
                useUIStore.getState().togglePianoRoll();
                break;
            case 'F4':
                e.preventDefault();
                useUIStore.getState().toggleBrowser();
                break;
            case 'F5':
                e.preventDefault();
                useUIStore.getState().toggleStemSeparation();
                break;
            case 'F6':
                e.preventDefault();
                useUIStore.getState().toggleMastering();
                break;
            case 'F7':
                e.preventDefault();
                useUIStore.getState().toggleAutotune();
                break;

            // ─── Tools ───
            case 'Digit1':
                if (!ctrl) useUIStore.getState().setActiveTool('pointer');
                break;
            case 'Digit2':
                if (!ctrl) useUIStore.getState().setActiveTool('range');
                break;
            case 'Digit3':
                if (!ctrl) useUIStore.getState().setActiveTool('draw');
                break;
            case 'Digit4':
                if (!ctrl) useUIStore.getState().setActiveTool('split');
                break;
            case 'Digit5':
                if (!ctrl) useUIStore.getState().setActiveTool('erase');
                break;
            case 'Digit6':
                if (!ctrl) useUIStore.getState().setActiveTool('automation');
                break;
            case 'Digit7':
                if (!ctrl) useUIStore.getState().setActiveTool('mute');
                break;

            // ─── Delete ───
            case 'Delete':
            case 'Backspace': {
                e.preventDefault();
                const ui = useUIStore.getState();
                const proj = useProjectStore.getState();
                if (ui.selectedClipId && ui.selectedClipTrackId) {
                    proj.removeClip(ui.selectedClipTrackId, ui.selectedClipId);
                    ui.clearSelection();
                } else if (ui.selectedTrackId) {
                    proj.removeTrack(ui.selectedTrackId);
                    ui.clearSelection();
                }
                break;
            }

            // ─── Add tracks ───
            case 'KeyT':
                if (ctrl && shift) {
                    e.preventDefault();
                    useProjectStore.getState().addTrack('midi');
                } else if (ctrl) {
                    e.preventDefault();
                    useProjectStore.getState().addTrack('audio');
                }
                break;

            // ─── Tap Tempo ───
            case 'KeyT':
                if (!ctrl && !shift) {
                    e.preventDefault();
                    if (!window._tapTimes) window._tapTimes = [];
                    const tapNow = Date.now();
                    window._tapTimes.push(tapNow);
                    if (window._tapTimes.length > 8) window._tapTimes.shift();
                    if (window._tapTimes.length >= 2) {
                        const intervals = [];
                        for (let ti = 1; ti < window._tapTimes.length; ti++) {
                            intervals.push(window._tapTimes[ti] - window._tapTimes[ti - 1]);
                        }
                        const avg = intervals.reduce((a, b) => a + b, 0) / intervals.length;
                        const tBpm = Math.round(60000 / avg);
                        if (tBpm >= 20 && tBpm <= 300) {
                            useProjectStore.getState().setBpm(tBpm);
                            audioEngine.setBPM(tBpm);
                        }
                    }
                    setTimeout(() => {
                        if (window._tapTimes.length > 0 && Date.now() - window._tapTimes[window._tapTimes.length - 1] > 2000) {
                            window._tapTimes = [];
                        }
                    }, 2500);
                }
                break;

            // ─── Escape ───
            case 'Escape':
                useUIStore.getState().hideContextMenu();
                useUIStore.getState().closeModal();
                useUIStore.getState().clearSelection();
                break;
        }
    });
}
