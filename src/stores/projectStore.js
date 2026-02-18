// ============================================
// ORPHEUS DAW — Project Store (Zustand)
// ============================================

import { create } from 'zustand';
import { persist } from 'zustand/middleware';
import { uid, getTrackColor, generateWaveformData } from '../utils/helpers';
import { useUIStore } from './uiStore';
import { TrackExtender } from '../audio/TrackExtender';
import { audioBufferManager } from '../audio/AudioBufferManager';
import { CloudService } from '../utils/CloudService';
import { AudioCleaner } from '../audio/AudioCleaner';
import { AudioToMidi } from '../audio/AudioToMidi';
import { audioEngine } from '../audio/AudioEngine';

const createDefaultTrack = (index, type = 'audio', name) => ({
    id: uid(),
    name: name || `${type === 'audio' ? 'Audio' : type === 'midi' ? 'MIDI' : 'Inst'} ${index + 1}`,
    type,
    color: getTrackColor(index),
    volume: 0.75,
    pan: 0,
    mute: false,
    solo: false,
    armed: false,
    clips: [],
    effects: [],
    automationLanes: [],
    autotune: {
        enabled: false,
        key: 'C',
        scale: 'chromatic',
        speed: 0.5, // 0-1
        amount: 1.0, // 0-1
        humanize: 0.1, // 0-1
        retune: 0, // cents
        formant: true
    },
    height: 80,
    visible: true,
});

const createAudioClip = (trackId, startBeat, lengthBeats, name) => ({
    id: uid(),
    trackId,
    type: 'audio',
    name: name || 'Audio Clip',
    startBeat,
    lengthBeats,
    offset: 0,
    gain: 1,
    fadeIn: 0,
    fadeOut: 0,
    isReversed: false,
    waveformData: generateWaveformData(500, 'random'),
    color: null,
});

const createMidiClip = (trackId, startBeat, lengthBeats, name, notes = []) => ({
    id: uid(),
    trackId,
    type: 'midi',
    name: name || 'MIDI Clip',
    startBeat,
    lengthBeats,
    notes,
    color: null,
});

const createMidiNote = (pitch, startBeat, lengthBeats, velocity = 100) => ({
    id: uid(),
    pitch,
    startBeat,
    lengthBeats,
    velocity,
});

// Demo project
function createDemoProject() {
    const tracks = [];

    // Track 1: Drums (Audio)
    const drums = createDefaultTrack(0, 'audio', 'Drums');
    drums.clips = [
        { ...createAudioClip(drums.id, 0, 8, 'Drum Loop 1'), waveformData: generateWaveformData(500, 'drum') },
        { ...createAudioClip(drums.id, 8, 8, 'Drum Loop 2'), waveformData: generateWaveformData(500, 'drum') },
        { ...createAudioClip(drums.id, 16, 16, 'Drum Fill'), waveformData: generateWaveformData(500, 'drum') },
    ];
    tracks.push(drums);

    // Track 2: Bass (MIDI)
    const bass = createDefaultTrack(1, 'midi', 'Bass Synth');
    const bassNotes = [];
    const bassPattern = [36, 36, 38, 36, 40, 36, 38, 41];
    for (let bar = 0; bar < 4; bar++) {
        for (let i = 0; i < bassPattern.length; i++) {
            bassNotes.push(createMidiNote(bassPattern[i], bar * 8 + i, 0.9, 80 + Math.random() * 40));
        }
    }
    bass.clips = [createMidiClip(bass.id, 0, 32, 'Bass Line', bassNotes)];
    tracks.push(bass);

    // Track 3: Lead Synth (MIDI)
    const lead = createDefaultTrack(2, 'midi', 'Lead Melody');
    const leadNotes = [];
    const melody = [60, 64, 67, 72, 71, 67, 64, 60, 62, 64, 67, 69, 67, 64, 62, 60];
    for (let i = 0; i < melody.length; i++) {
        leadNotes.push(createMidiNote(melody[i], i * 2, 1.8, 70 + Math.random() * 50));
    }
    lead.clips = [createMidiClip(lead.id, 0, 32, 'Lead Melody', leadNotes)];
    lead.volume = 0.6;
    tracks.push(lead);

    // Track 4: Pad (Audio)
    const pad = createDefaultTrack(3, 'audio', 'Ambient Pad');
    pad.clips = [
        { ...createAudioClip(pad.id, 0, 32, 'Pad Texture'), waveformData: generateWaveformData(500, 'vocal') },
    ];
    pad.volume = 0.45;
    pad.pan = -0.2;
    tracks.push(pad);

    // Track 5: Vocals (Audio)
    const vox = createDefaultTrack(4, 'audio', 'Vocals');
    vox.clips = [
        { ...createAudioClip(vox.id, 8, 16, 'Verse'), waveformData: generateWaveformData(500, 'vocal') },
        { ...createAudioClip(vox.id, 28, 4, 'Ad-lib'), waveformData: generateWaveformData(500, 'vocal') },
    ];
    vox.volume = 0.7;
    tracks.push(vox);

    // Track 6: FX (Audio)
    const fx = createDefaultTrack(5, 'audio', 'FX / Risers');
    fx.clips = [
        { ...createAudioClip(fx.id, 14, 2, 'Riser'), waveformData: generateWaveformData(500, 'sine') },
        { ...createAudioClip(fx.id, 30, 2, 'Impact'), waveformData: generateWaveformData(500, 'drum') },
    ];
    fx.volume = 0.5;
    tracks.push(fx);

    return tracks;
}

export const useProjectStore = create(
    persist(
        (set, get) => ({
            // Project metadata
            projectName: 'Untitled Project',
            bpm: 128,
            timeSignature: [4, 4],
            sampleRate: 44100,
            key: 'C',
            scale: 'major',

            // Tracks
            tracks: createDemoProject(),

            // Track Folders
            trackFolders: [], // { id, name, color, trackIds[], collapsed }

            // Transport
            isPlaying: false,
            isRecording: false,
            isLooping: true,
            loopStart: 0,
            loopEnd: 32,
            playheadPosition: 0,
            masterVolume: 0.8,

            // Undo/redo
            undoStack: [],
            redoStack: [],
            undoLabels: [],  // Label for each undo entry
            _maxHistory: 50,

            // Auto-save
            _autoSaveTimer: null,
            lastAutoSave: null,

            // Push current state to undo stack before making changes
            _pushUndo: (label = 'Edit') => {
                const state = get();
                const snapshot = {
                    tracks: JSON.parse(JSON.stringify(state.tracks)),
                    trackFolders: JSON.parse(JSON.stringify(state.trackFolders)),
                    projectName: state.projectName,
                    bpm: state.bpm,
                    timeSignature: state.timeSignature,
                };
                set(prev => ({
                    undoStack: [...prev.undoStack.slice(-prev._maxHistory), snapshot],
                    undoLabels: [...prev.undoLabels.slice(-prev._maxHistory), label],
                    redoStack: [],
                }));
            },

            undo: () => {
                const { undoStack, undoLabels, tracks, trackFolders, projectName, bpm, timeSignature } = get();
                if (undoStack.length === 0) return;
                const current = { tracks: JSON.parse(JSON.stringify(tracks)), trackFolders: JSON.parse(JSON.stringify(trackFolders)), projectName, bpm, timeSignature };
                const prev = undoStack[undoStack.length - 1];
                set({
                    ...prev,
                    undoStack: undoStack.slice(0, -1),
                    undoLabels: undoLabels.slice(0, -1),
                    redoStack: [...get().redoStack, current],
                });
            },

            redo: () => {
                const { redoStack, tracks, trackFolders, projectName, bpm, timeSignature } = get();
                if (redoStack.length === 0) return;
                const current = { tracks: JSON.parse(JSON.stringify(tracks)), trackFolders: JSON.parse(JSON.stringify(trackFolders)), projectName, bpm, timeSignature };
                const next = redoStack[redoStack.length - 1];
                set({
                    ...next,
                    redoStack: redoStack.slice(0, -1),
                    undoStack: [...get().undoStack, current],
                });
            },
            // Actions
            setProjectName: (name) => set({ projectName: name }),
            setBpm: (bpm) => set({ bpm: Math.max(20, Math.min(300, bpm)) }),
            setTimeSignature: (ts) => set({ timeSignature: ts }),
            setMasterVolume: (v) => set({ masterVolume: v }),

            setPlaying: (v) => set({ isPlaying: v }),
            setRecording: (v) => set({ isRecording: v }),
            toggleLoop: () => set((s) => ({ isLooping: !s.isLooping })),
            setPlayheadPosition: (pos) => set({ playheadPosition: pos }),
            setLoopRange: (start, end) => set({ loopStart: start, loopEnd: end }),

            addTrack: (type = 'audio') => set((state) => {
                get()._pushUndo();
                const track = createDefaultTrack(state.tracks.length, type);
                return { tracks: [...state.tracks, track] };
            }),

            removeTrack: (id) => {
                get()._pushUndo();
                set((state) => ({ tracks: state.tracks.filter(t => t.id !== id) }));
            },

            duplicateTrack: (id) => set((state) => {
                const source = state.tracks.find(t => t.id === id);
                if (!source) return state;
                const copy = {
                    ...source,
                    id: uid(),
                    name: `${source.name} (Copy)`,
                    clips: source.clips.map(c => ({ ...c, id: uid() }))
                };
                const idx = state.tracks.findIndex(t => t.id === id);
                const tracks = [...state.tracks];
                tracks.splice(idx + 1, 0, copy);
                return { tracks };
            }),

            updateTrack: (id, updates) => set((state) => ({
                tracks: state.tracks.map(t => t.id === id ? { ...t, ...updates } : t)
            })),

            toggleMute: (id) => set((state) => ({
                tracks: state.tracks.map(t => t.id === id ? { ...t, mute: !t.mute } : t)
            })),

            toggleSolo: (id) => set((state) => ({
                tracks: state.tracks.map(t => t.id === id ? { ...t, solo: !t.solo } : t)
            })),

            toggleArmed: (id) => set((state) => ({
                tracks: state.tracks.map(t => t.id === id ? { ...t, armed: !t.armed } : t)
            })),

            setTrackVolume: (id, volume) => set((state) => ({
                tracks: state.tracks.map(t => t.id === id ? { ...t, volume } : t)
            })),

            setTrackPan: (id, pan) => set((state) => ({
                tracks: state.tracks.map(t => t.id === id ? { ...t, pan } : t)
            })),

            addTrackEffect: (trackId, effect) => {
                get()._pushUndo();
                set((state) => ({
                    tracks: state.tracks.map(t =>
                        t.id === trackId ? { ...t, effects: [...t.effects, { ...effect, id: uid() }] } : t
                    )
                }));
            },

            removeTrackEffect: (trackId, effectId) => {
                get()._pushUndo();
                set((state) => ({
                    tracks: state.tracks.map(t =>
                        t.id === trackId ? { ...t, effects: t.effects.filter(e => e.id !== effectId) } : t
                    )
                }));
            },

            renameTrack: (trackId, name) => set((state) => ({
                tracks: state.tracks.map(t => t.id === trackId ? { ...t, name } : t)
            })),

            setTrackColor: (trackId, color) => set((state) => ({
                tracks: state.tracks.map(t => t.id === trackId ? { ...t, color } : t)
            })),

            setTrackAutotune: (trackId, params) => set((state) => ({
                tracks: state.tracks.map(t => t.id === trackId ? { ...t, autotune: { ...t.autotune, ...params } } : t)
            })),

            moveTrack: (fromIndex, toIndex) => {
                set((state) => {
                    if (fromIndex < 0 || fromIndex >= state.tracks.length || toIndex < 0 || toIndex >= state.tracks.length) return state;
                    const newTracks = [...state.tracks];
                    const [movedTrack] = newTracks.splice(fromIndex, 1);
                    newTracks.splice(toIndex, 0, movedTrack);
                    return { tracks: newTracks };
                });
            },

            quantizeSelection: (grid = 0.25) => {
                const state = get();
                const { selectedClipId, selectedClipTrackId } = useUIStore.getState();

                if (!selectedClipId || !selectedClipTrackId) return;

                const trackIndex = state.tracks.findIndex(t => t.id === selectedClipTrackId);
                if (trackIndex === -1) return;

                const track = state.tracks[trackIndex];
                const clipIndex = track.clips.findIndex(c => c.id === selectedClipId);
                if (clipIndex === -1) return;

                state._pushUndo();

                const clip = { ...track.clips[clipIndex] };

                // Quantize Clip Start
                clip.startBeat = Math.round(clip.startBeat / grid) * grid;

                // If MIDI, quantize notes
                if (clip.type === 'midi' && clip.notes) {
                    clip.notes = clip.notes.map(note => ({
                        ...note,
                        startBeat: Math.round(note.startBeat / grid) * grid,
                        lengthBeats: Math.max(grid, Math.round(note.lengthBeats / grid) * grid)
                    }));
                }

                const newTracks = [...state.tracks];
                newTracks[trackIndex] = {
                    ...track,
                    clips: [
                        ...track.clips.slice(0, clipIndex),
                        clip,
                        ...track.clips.slice(clipIndex + 1)
                    ]
                };

                set({ tracks: newTracks });
            },


            // ─── Stem Separation (Real-time Filtering) ───
            processStems: () => {
                const state = get();
                const { selectedClipId, selectedClipTrackId } = useUIStore.getState();

                if (!selectedClipId || !selectedClipTrackId) return;

                const track = state.tracks.find(t => t.id === selectedClipTrackId);
                if (!track) return;

                const clip = track.clips.find(c => c.id === selectedClipId);
                if (!clip) return;

                state._pushUndo();
                const baseTrackIndex = state.tracks.findIndex(t => t.id === selectedClipTrackId);
                if (baseTrackIndex === -1) return;

                // Helper to add stem track
                const addStemTrack = (name, eqSettings) => {
                    const newTrackId = uid();
                    const newClip = { ...clip, id: uid(), bufferId: clip.bufferId }; // Clone clip

                    // Create EQ effect
                    const eqEffect = {
                        id: uid(),
                        type: 'eq',
                        active: true,
                        params: {
                            low: 0, mid: 0, high: 0,
                            lowFreq: 100, midFreq: 1000, highFreq: 5000,
                            ...eqSettings
                        }
                    };

                    return {
                        id: newTrackId,
                        name: `${name} (${clip.name})`,
                        type: 'audio',
                        volume: 0.8,
                        pan: 0,
                        muted: false,
                        soloed: false,
                        color: getTrackColor(state.tracks.length),
                        clips: [newClip],
                        effects: [eqEffect]
                    };
                };

                const vocalTrack = addStemTrack('Vocals', { low: -24, mid: 3, high: 2, lowFreq: 300 }); // HPF approximation via low shelf cut
                const drumTrack = addStemTrack('Drums', { low: 4, mid: -3, high: 4, lowFreq: 120, midFreq: 500, highFreq: 8000 });
                const bassTrack = addStemTrack('Bass', { low: 6, mid: -24, high: -24, lowFreq: 250 }); // LPF approx
                const otherTrack = addStemTrack('Other', { low: -6, mid: 4, high: -6, midFreq: 1500 }); // Band focused

                const newTracks = [...state.tracks];
                // Insert stems after the source track
                newTracks.splice(baseTrackIndex + 1, 0, vocalTrack, drumTrack, bassTrack, otherTrack);

                set({ tracks: newTracks });
                set({ tracks: newTracks });
            },

            addStemTracks: (baseTrackId, originalClipId, stems) => {
                const state = get();
                const baseTrackIndex = state.tracks.findIndex(t => t.id === baseTrackId);
                if (baseTrackIndex === -1) return;

                const track = state.tracks[baseTrackIndex];
                const clip = track.clips.find(c => c.id === originalClipId);
                if (!clip) return;

                state._pushUndo(); // Save state

                const newTracks = Object.entries(stems).map(([label, bufferId], i) => {
                    const newTrackId = uid();
                    return {
                        id: newTrackId,
                        name: label,
                        type: 'audio',
                        volume: 0.8,
                        pan: 0,
                        color: getTrackColor(state.tracks.length + i),
                        mute: false,
                        solo: false,
                        clips: [{
                            ...clip,
                            id: uid(),
                            trackId: newTrackId,
                            bufferId,
                            name: label
                            // We keep original startBeat/length to match sync
                        }],
                        effects: []
                    };
                });

                const updatedTracks = [...state.tracks];
                updatedTracks.splice(baseTrackIndex + 1, 0, ...newTracks);
                set({ tracks: updatedTracks });
            },

            addClip: (trackId, clip) => set((state) => ({
                tracks: state.tracks.map(t =>
                    t.id === trackId ? { ...t, clips: [...t.clips, clip] } : t
                )
            })),

            removeClip: (trackId, clipId) => set((state) => ({
                tracks: state.tracks.map(t =>
                    t.id === trackId ? { ...t, clips: t.clips.filter(c => c.id !== clipId) } : t
                )
            })),

            updateClip: (trackId, clipId, updates) => set((state) => ({
                tracks: state.tracks.map(t =>
                    t.id === trackId ? {
                        ...t,
                        clips: t.clips.map(c => c.id === clipId ? { ...c, ...updates } : c)
                    } : t
                )
            })),

            // Advanced Editing
            splitClip: (trackId, clipId, splitBeat) => {
                const state = get();
                const track = state.tracks.find(t => t.id === trackId);
                if (!track) return;
                const clip = track.clips.find(c => c.id === clipId);
                if (!clip) return;

                // validation
                if (splitBeat <= clip.startBeat || splitBeat >= clip.startBeat + clip.lengthBeats) return;

                state._pushUndo();

                const firstLength = splitBeat - clip.startBeat;
                const secondLength = clip.lengthBeats - firstLength;

                // First part (matches original start, shorter length)
                const leftClip = {
                    ...clip,
                    lengthBeats: firstLength,
                    // fadeIn/Out logic: keep fadeIn, reset fadeOut? 
                    // For non-destructive split, we usually want to keep defaults unless user set them.
                    // If user had a fadeOut on the original clip, it should probably move to the right clip?
                    // For now, simple split.
                    fadeOut: 0 // Remove fade out from left part
                };

                // Second part (starts at split, offset increases)
                const rightClip = {
                    ...clip,
                    id: uid(),
                    startBeat: splitBeat,
                    lengthBeats: secondLength,
                    offset: clip.offset + (firstLength / state.bpm) * 60, // Add time offset
                    fadeIn: 0, // Remove fade in from right part
                    fadeOut: clip.fadeOut // Keep original fade out
                };

                set(s => ({
                    tracks: s.tracks.map(t => t.id === trackId ? {
                        ...t,
                        clips: t.clips.map(c => c.id === clipId ? leftClip : c).concat(rightClip)
                    } : t)
                }));
            },

            reverseClip: (trackId, clipId) => {
                const state = get();
                state._pushUndo();
                set(s => ({
                    tracks: s.tracks.map(t => t.id === trackId ? {
                        ...t,
                        clips: t.clips.map(c => c.id === clipId ? { ...c, isReversed: !c.isReversed } : c)
                    } : t)
                }));
            },

            // MIDI note editing
            addNote: (trackId, clipId, note) => set((state) => ({
                tracks: state.tracks.map(t =>
                    t.id === trackId ? {
                        ...t,
                        clips: t.clips.map(c =>
                            c.id === clipId ? { ...c, notes: [...(c.notes || []), note] } : c
                        )
                    } : t
                )
            })),

            removeNote: (trackId, clipId, noteId) => set((state) => ({
                tracks: state.tracks.map(t =>
                    t.id === trackId ? {
                        ...t,
                        clips: t.clips.map(c =>
                            c.id === clipId ? { ...c, notes: (c.notes || []).filter(n => n.id !== noteId) } : c
                        )
                    } : t
                )
            })),

            updateNote: (trackId, clipId, noteId, updates) => set((state) => ({
                tracks: state.tracks.map(t =>
                    t.id === trackId ? {
                        ...t,
                        clips: t.clips.map(c =>
                            c.id === clipId ? {
                                ...c,
                                notes: (c.notes || []).map(n => n.id === noteId ? { ...n, ...updates } : n)
                            } : c
                        )
                    } : t
                )
            })),

            // Automation
            updateTrackAutomation: (trackId, laneIndex, param, points) => set((state) => ({
                tracks: state.tracks.map(t => {
                    if (t.id !== trackId) return t;
                    const lanes = [...(t.automationLanes || [])];
                    while (lanes.length <= laneIndex) lanes.push({ param: 'volume', points: [] });
                    lanes[laneIndex] = { param, points };
                    return { ...t, automationLanes: lanes };
                })
            })),

            addAutomationLane: (trackId) => {
                get()._pushUndo();
                set((state) => ({
                    tracks: state.tracks.map(t =>
                        t.id === trackId
                            ? { ...t, automationLanes: [...(t.automationLanes || []), { param: 'volume', points: [{ beat: 0, value: 0.75 }, { beat: 32, value: 0.75 }] }] }
                            : t
                    )
                }));
            },

            removeAutomationLane: (trackId, laneIndex) => {
                get()._pushUndo();
                set((state) => ({
                    tracks: state.tracks.map(t =>
                        t.id === trackId
                            ? { ...t, automationLanes: (t.automationLanes || []).filter((_, i) => i !== laneIndex) }
                            : t
                    )
                }));
            },

            // ─── Track Folders ───
            addTrackFolder: (name = 'New Folder') => {
                get()._pushUndo('Add Folder');
                set(prev => ({
                    trackFolders: [...prev.trackFolders, { id: uid(), name, color: '#666', trackIds: [], collapsed: false }]
                }));
            },

            removeTrackFolder: (folderId) => {
                get()._pushUndo('Remove Folder');
                set(prev => ({
                    trackFolders: prev.trackFolders.filter(f => f.id !== folderId)
                }));
            },

            toggleFolderCollapse: (folderId) => {
                set(prev => ({
                    trackFolders: prev.trackFolders.map(f =>
                        f.id === folderId ? { ...f, collapsed: !f.collapsed } : f
                    )
                }));
            },

            addTrackToFolder: (folderId, trackId) => {
                set(prev => ({
                    trackFolders: prev.trackFolders.map(f =>
                        f.id === folderId ? { ...f, trackIds: [...f.trackIds.filter(id => id !== trackId), trackId] } : f
                    )
                }));
            },

            removeTrackFromFolder: (folderId, trackId) => {
                set(prev => ({
                    trackFolders: prev.trackFolders.map(f =>
                        f.id === folderId ? { ...f, trackIds: f.trackIds.filter(id => id !== trackId) } : f
                    )
                }));
            },

            // ─── Glue / Merge Clips ───
            glueClips: (trackId, clipIds) => {
                get()._pushUndo('Glue Clips');
                set(state => {
                    const track = state.tracks.find(t => t.id === trackId);
                    if (!track) return state;
                    const toGlue = track.clips.filter(c => clipIds.includes(c.id)).sort((a, b) => a.startBeat - b.startBeat);
                    if (toGlue.length < 2) return state;
                    const first = toGlue[0];
                    const last = toGlue[toGlue.length - 1];
                    const merged = {
                        ...first,
                        id: uid(),
                        name: first.name + ' (merged)',
                        lengthBeats: (last.startBeat + last.lengthBeats) - first.startBeat,
                    };
                    return {
                        tracks: state.tracks.map(t =>
                            t.id === trackId ? { ...t, clips: [...t.clips.filter(c => !clipIds.includes(c.id)), merged] } : t
                        )
                    };
                });
            },

            // ─── Duplicate Clip ───
            duplicateClip: (trackId, clipId) => {
                get()._pushUndo('Duplicate Clip');
                set(state => {
                    const track = state.tracks.find(t => t.id === trackId);
                    if (!track) return state;
                    const clip = track.clips.find(c => c.id === clipId);
                    if (!clip) return state;
                    const dup = { ...JSON.parse(JSON.stringify(clip)), id: uid(), startBeat: clip.startBeat + clip.lengthBeats };
                    return {
                        tracks: state.tracks.map(t =>
                            t.id === trackId ? { ...t, clips: [...t.clips, dup] } : t
                        )
                    };
                });
            },

            // ─── Freeze / Flatten ───
            freezeTrack: (trackId) => {
                get()._pushUndo('Freeze Track');
                set(state => ({
                    tracks: state.tracks.map(t =>
                        t.id === trackId ? { ...t, frozen: true } : t
                    )
                }));
            },

            unfreezeTrack: (trackId) => {
                set(state => ({
                    tracks: state.tracks.map(t =>
                        t.id === trackId ? { ...t, frozen: false } : t
                    )
                }));
            },

            // ─── Templates ───
            saveTemplate: (name) => {
                const state = get();
                const template = {
                    name,
                    bpm: state.bpm,
                    timeSignature: state.timeSignature,
                    tracks: state.tracks.map(t => ({ ...t, clips: [] })), // No clip data
                    trackFolders: state.trackFolders,
                    savedAt: new Date().toISOString(),
                };
                const templates = JSON.parse(localStorage.getItem('orpheus_templates') || '[]');
                templates.push(template);
                localStorage.setItem('orpheus_templates', JSON.stringify(templates));
                return template;
            },

            loadTemplate: (index) => {
                const templates = JSON.parse(localStorage.getItem('orpheus_templates') || '[]');
                if (!templates[index]) return;
                const tmpl = templates[index];
                get()._pushUndo('Load Template');
                set({
                    bpm: tmpl.bpm,
                    timeSignature: tmpl.timeSignature,
                    tracks: tmpl.tracks,
                    trackFolders: tmpl.trackFolders || [],
                });
            },

            getTemplates: () => {
                return JSON.parse(localStorage.getItem('orpheus_templates') || '[]');
            },

            deleteTemplate: (index) => {
                const templates = JSON.parse(localStorage.getItem('orpheus_templates') || '[]');
                templates.splice(index, 1);
                localStorage.setItem('orpheus_templates', JSON.stringify(templates));
            },

            // ─── Auto-Save ───
            startAutoSave: () => {
                const timer = setInterval(() => {
                    const state = get();
                    if (state.tracks.length > 0) {
                        state.saveProject();
                        set({ lastAutoSave: new Date().toISOString() });
                    }
                }, 5000);
                set({ _autoSaveTimer: timer });
            },

            stopAutoSave: () => {
                const timer = get()._autoSaveTimer;
                if (timer) clearInterval(timer);
                set({ _autoSaveTimer: null });
            },

            // ─── Slip Editing ───
            slipEdit: (trackId, clipId, offsetDelta) => {
                get()._pushUndo('Slip Edit');
                set(state => ({
                    tracks: state.tracks.map(t =>
                        t.id === trackId ? {
                            ...t,
                            clips: t.clips.map(c => c.id === clipId ? { ...c, offset: Math.max(0, (c.offset || 0) + offsetDelta) } : c)
                        } : t
                    )
                }));
            },

            // ─── Project Key/Scale ───
            setKey: (key) => set({ key }),
            setScale: (scale) => set({ scale }),

            // Serialization
            saveProject: () => {
                const state = get();
                const data = {
                    projectName: state.projectName,
                    bpm: state.bpm,
                    timeSignature: state.timeSignature,
                    tracks: state.tracks,
                    loopStart: state.loopStart,
                    loopEnd: state.loopEnd,
                };
                const json = JSON.stringify(data);
                localStorage.setItem('orpheus_project', json);
                return json;
            },

            loadProject: () => {
                const json = localStorage.getItem('orpheus_project');
                if (!json) return false;
                try {
                    const data = JSON.parse(json);
                    set(data);
                    return true;
                } catch {
                    return false;
                }
            },

            exportProject: () => {
                const state = get();
                const data = {
                    projectName: state.projectName,
                    bpm: state.bpm,
                    timeSignature: state.timeSignature,
                    tracks: state.tracks.map(t => ({
                        ...t,
                        clips: t.clips.map(c => ({
                            ...c,
                            waveformData: undefined // Don't export waveform data
                        }))
                    })),
                };
                const blob = new Blob([JSON.stringify(data, null, 2)], { type: 'application/json' });
                const url = URL.createObjectURL(blob);
                const a = document.createElement('a');
                a.href = url;
                a.download = `${state.projectName}.orpheus`;
                a.click();
                URL.revokeObjectURL(url);
            },

            newProject: () => set({
                projectName: 'Untitled Project',
                bpm: 120,
                timeSignature: [4, 4],
                tracks: [createDefaultTrack(0, 'audio')],
                playheadPosition: 0,
                isPlaying: false,
                isRecording: false,
                undoStack: [],
                redoStack: [],
            }),

            // Import project from file
            importProject: (file) => {
                return new Promise((resolve, reject) => {
                    const reader = new FileReader();
                    reader.onload = (e) => {
                        try {
                            const data = JSON.parse(e.target.result);
                            set({
                                ...data,
                                isPlaying: false,
                                isRecording: false,
                                playheadPosition: 0,
                                undoStack: [],
                                redoStack: [],
                            });
                            resolve(true);
                        } catch {
                            reject(new Error('Invalid project file'));
                        }
                    };
                    reader.onerror = () => reject(new Error('Failed to read file'));
                    reader.readAsText(file);
                });
            },

            // ─── Cloud Storage ───
            isSaving: false,
            isLoading: false,
            cloudProjects: [],

            saveToCloud: async () => {
                const state = get();
                set({ isSaving: true });
                try {
                    const data = {
                        name: state.projectName,
                        data: {
                            tracks: state.tracks,
                            bpm: state.bpm,
                            timeSignature: state.timeSignature,
                            loopStart: state.loopStart,
                            loopEnd: state.loopEnd
                        }
                    };

                    const res = await fetch('/api/projects', {
                        method: 'POST',
                        headers: { 'Content-Type': 'application/json' },
                        body: JSON.stringify(data)
                    });

                    if (!res.ok) throw new Error('Failed to save');
                    const saved = await res.json();
                    // Refresh list
                    get().fetchProjects();
                    return true;
                } catch (err) {
                    console.error(err);
                    return false;
                } finally {
                    set({ isSaving: false });
                }
            },

            fetchProjects: async () => {
                try {
                    const res = await fetch('/api/projects');
                    if (res.ok) {
                        const projects = await res.json();
                        set({ cloudProjects: projects });
                    }
                } catch (err) {
                    console.error(err);
                }
            },

            loadFromCloud: async (id) => {
                set({ isLoading: true });
                try {
                    const res = await fetch(`/api/projects/${id}`);
                    if (!res.ok) throw new Error('Failed to load');
                    const project = await res.json();
                    const data = project.data;

                    set({
                        projectName: project.name,
                        tracks: data.tracks,
                        bpm: data.bpm,
                        timeSignature: data.timeSignature,
                        loopStart: data.loopStart,
                        loopEnd: data.loopEnd,
                        undoStack: [],
                        redoStack: [],
                        isPlaying: false
                    });
                } catch (err) {
                    console.error(err);
                } finally {
                    set({ isLoading: false });
                }
            },

            // ─── Phase 25: Clip Gain Automation ───
            setClipGainAutomation: (trackId, clipId, points) => {
                get()._pushUndo('Edit Clip Gain');
                set(state => ({
                    tracks: state.tracks.map(t =>
                        t.id === trackId ? {
                            ...t,
                            clips: t.clips.map(c =>
                                c.id === clipId ? { ...c, gainAutomation: points } : c
                            )
                        } : t
                    )
                }));
            },

            addClipGainPoint: (trackId, clipId, beatOffset, gain) => {
                get()._pushUndo('Add Gain Point');
                set(state => ({
                    tracks: state.tracks.map(t =>
                        t.id === trackId ? {
                            ...t,
                            clips: t.clips.map(c => {
                                if (c.id !== clipId) return c;
                                const points = [...(c.gainAutomation || []), { beat: beatOffset, value: Math.max(0, Math.min(2, gain)) }];
                                points.sort((a, b) => a.beat - b.beat);
                                return { ...c, gainAutomation: points };
                            })
                        } : t
                    )
                }));
            },

            // ─── Normalize Clip (loudness) ───
            normalizeClip: (trackId, clipId, targetDb = -14) => {
                get()._pushUndo('Normalize');
                set(state => ({
                    tracks: state.tracks.map(t =>
                        t.id === trackId ? {
                            ...t,
                            clips: t.clips.map(c => {
                                if (c.id !== clipId) return c;
                                // Simple gain normalization: adjust gain to reach target loudness
                                // In a real DAW, this would analyze the buffer for LUFS
                                const currentGain = c.gain || 1;
                                const targetGain = Math.pow(10, targetDb / 20);
                                return { ...c, gain: targetGain, normalized: true, normalizeTarget: targetDb };
                            })
                        } : t
                    )
                }));
            },

            // ─── Batch Processing ───
            batchProcess: (trackId, clipIds, operation) => {
                get()._pushUndo(`Batch ${operation}`);
                set(state => ({
                    tracks: state.tracks.map(t => {
                        if (t.id !== trackId) return t;
                        return {
                            ...t,
                            clips: t.clips.map(c => {
                                if (!clipIds.includes(c.id)) return c;
                                switch (operation) {
                                    case 'normalize':
                                        return { ...c, gain: Math.pow(10, -14 / 20), normalized: true };
                                    case 'reverse':
                                        return { ...c, reversed: !c.reversed };
                                    case 'fadeIn':
                                        return { ...c, fadeIn: Math.min(c.lengthBeats * 0.25, 2) };
                                    case 'fadeOut':
                                        return { ...c, fadeOut: Math.min(c.lengthBeats * 0.25, 2) };
                                    case 'mute':
                                        return { ...c, muted: !c.muted };
                                    case 'resetGain':
                                        return { ...c, gain: 1, gainAutomation: [] };
                                    default:
                                        return c;
                                }
                            })
                        };
                    })
                }));
            },

            // ─── Transient Slicing ───
            sliceAtTransients: (trackId, clipId, transientBeats) => {
                get()._pushUndo('Slice at Transients');
                set(state => ({
                    tracks: state.tracks.map(t => {
                        if (t.id !== trackId) return t;
                        const clip = t.clips.find(c => c.id === clipId);
                        if (!clip || !transientBeats || transientBeats.length === 0) return t;

                        // Sort transient positions
                        const sorted = [...transientBeats].sort((a, b) => a - b);
                        const newClips = [];
                        let prevBeat = 0;

                        for (const beat of sorted) {
                            if (beat <= prevBeat || beat >= clip.lengthBeats) continue;
                            newClips.push({
                                ...clip,
                                id: uid(),
                                startBeat: clip.startBeat + prevBeat,
                                lengthBeats: beat - prevBeat,
                                offset: (clip.offset || 0) + prevBeat,
                            });
                            prevBeat = beat;
                        }
                        // Last slice
                        if (prevBeat < clip.lengthBeats) {
                            newClips.push({
                                ...clip,
                                id: uid(),
                                startBeat: clip.startBeat + prevBeat,
                                lengthBeats: clip.lengthBeats - prevBeat,
                                offset: (clip.offset || 0) + prevBeat,
                            });
                        }

                        return {
                            ...t,
                            clips: [...t.clips.filter(c => c.id !== clipId), ...newClips]
                        };
                    })
                }));
            },

            // ─── Clip Color ───
            setClipColor: (trackId, clipId, color) => {
                set(state => ({
                    tracks: state.tracks.map(t =>
                        t.id === trackId ? {
                            ...t,
                            clips: t.clips.map(c =>
                                c.id === clipId ? { ...c, color } : c
                            )
                        } : t
                    )
                }));
            },

            // ─── Clip Mute ───
            toggleClipMute: (trackId, clipId) => {
                set(state => ({
                    tracks: state.tracks.map(t =>
                        t.id === trackId ? {
                            ...t,
                            clips: t.clips.map(c =>
                                c.id === clipId ? { ...c, muted: !c.muted } : c
                            )
                        } : t
                    )
                }));
            },

            // ─── Phase 26: Humanize ───
            humanize: (trackId, clipId, amount = 0.3) => {
                get()._pushUndo('Humanize');
                set(state => ({
                    tracks: state.tracks.map(t => {
                        if (t.id !== trackId) return t;
                        return {
                            ...t,
                            clips: t.clips.map(c => {
                                if (c.id !== clipId || !c.notes) return c;
                                return {
                                    ...c,
                                    notes: c.notes.map(n => ({
                                        ...n,
                                        startBeat: n.startBeat + (Math.random() - 0.5) * amount * 0.25,
                                        velocity: Math.max(1, Math.min(127,
                                            n.velocity + Math.floor((Math.random() - 0.5) * amount * 30)
                                        )),
                                    })),
                                };
                            })
                        };
                    })
                }));
            },

            // ─── Randomize Notes ───
            randomizeNotes: (trackId, clipId, options = {}) => {
                const { velocityRange = 20, timingRange = 0.1, pitchRange = 0 } = options;
                get()._pushUndo('Randomize');
                set(state => ({
                    tracks: state.tracks.map(t => {
                        if (t.id !== trackId) return t;
                        return {
                            ...t,
                            clips: t.clips.map(c => {
                                if (c.id !== clipId || !c.notes) return c;
                                return {
                                    ...c,
                                    notes: c.notes.map(n => ({
                                        ...n,
                                        velocity: Math.max(1, Math.min(127,
                                            n.velocity + Math.floor((Math.random() - 0.5) * velocityRange)
                                        )),
                                        startBeat: n.startBeat + (Math.random() - 0.5) * timingRange,
                                        pitch: n.pitch + Math.floor((Math.random() - 0.5) * pitchRange),
                                    })),
                                };
                            })
                        };
                    })
                }));
            },

            // ─── Note Repeat ───
            noteRepeat: (trackId, clipId, noteId, interval = 0.25, count = 4) => {
                get()._pushUndo('Note Repeat');
                set(state => ({
                    tracks: state.tracks.map(t => {
                        if (t.id !== trackId) return t;
                        return {
                            ...t,
                            clips: t.clips.map(c => {
                                if (c.id !== clipId || !c.notes) return c;
                                const sourceNote = c.notes.find(n => n.id === noteId);
                                if (!sourceNote) return c;
                                const newNotes = [];
                                for (let i = 1; i <= count; i++) {
                                    newNotes.push({
                                        ...sourceNote,
                                        id: uid(),
                                        startBeat: sourceNote.startBeat + interval * i,
                                    });
                                }
                                return { ...c, notes: [...c.notes, ...newNotes] };
                            })
                        };
                    })
                }));
            },

            // ─── Chord Detection ───
            detectChords: (notes) => {
                // Basic chord detection from a set of simultaneous notes
                if (!notes || notes.length < 2) return null;
                const pitches = notes.map(n => n.pitch % 12).sort((a, b) => a - b);
                const unique = [...new Set(pitches)];
                if (unique.length < 2) return null;

                const NOTE_NAMES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
                const root = unique[0];
                const intervals = unique.slice(1).map(p => (p - root + 12) % 12);

                const chordTypes = {
                    '3,7': 'min',
                    '4,7': 'maj',
                    '3,6': 'dim',
                    '4,8': 'aug',
                    '4,7,11': 'maj7',
                    '4,7,10': '7',
                    '3,7,10': 'min7',
                    '3,6,9': 'dim7',
                    '4,7,9': 'maj6',
                    '3,7,9': 'min6',
                    '2,7': 'sus2',
                    '5,7': 'sus4',
                };

                const key = intervals.join(',');
                const type = chordTypes[key] || '';
                return NOTE_NAMES[root] + type;
            },

            // ─── Phase 30: Automation Curves ───
            setAutomationCurve: (trackId, paramId, points) => {
                get()._pushUndo('Edit Automation');
                set(state => ({
                    tracks: state.tracks.map(t =>
                        t.id === trackId ? {
                            ...t,
                            automation: {
                                ...t.automation,
                                [paramId]: points, // Array of { beat, value, curve: 'linear'|'bezier' }
                            }
                        } : t
                    )
                }));
            },

            copyAutomation: (trackId, paramId) => {
                const state = get();
                const track = state.tracks.find(t => t.id === trackId);
                if (!track?.automation?.[paramId]) return null;
                return JSON.parse(JSON.stringify(track.automation[paramId]));
            },

            pasteAutomation: (trackId, paramId, points) => {
                get()._pushUndo('Paste Automation');
                set(state => ({
                    tracks: state.tracks.map(t =>
                        t.id === trackId ? {
                            ...t,
                            automation: { ...t.automation, [paramId]: points }
                        } : t
                    )
                }));
            },

            // ─── AI Auto-Level (Phase 30) ───
            autoLevel: () => {
                get()._pushUndo('Auto Level');
                set(state => {
                    // Simple auto-leveling: normalize all track volumes relative to each other
                    const trackCount = state.tracks.filter(t => !t.muted).length;
                    if (trackCount === 0) return state;
                    const targetVolume = 0.7 / Math.sqrt(trackCount);
                    return {
                        tracks: state.tracks.map(t => ({
                            ...t,
                            volume: t.muted ? t.volume : targetVolume,
                        }))
                    };
                });
            },

            // ─── Export Config (Phase 32) ───
            setExportConfig: (config) => set({ exportConfig: config }),

            exportStems: () => {
                // Mark tracks for stem export
                const state = get();
                return state.tracks.map(t => ({
                    trackId: t.id,
                    name: t.name,
                    clips: t.clips,
                    volume: t.volume,
                    pan: t.pan,
                }));
            },

            // ─── Project Archiving (Phase 32) ───
            archiveProject: () => {
                const state = get();
                return {
                    version: 2,
                    timestamp: new Date().toISOString(),
                    projectName: state.projectName,
                    bpm: state.bpm,
                    timeSignature: state.timeSignature,
                    tracks: JSON.parse(JSON.stringify(state.tracks)),
                    trackFolders: JSON.parse(JSON.stringify(state.trackFolders)),
                    key: state.key,
                    scale: state.scale,
                };
            },

            // ─── Cloud & Persistence ───
            cloudProjects: [],
            isLoading: false,
            isSaving: false,

            fetchProjects: async () => {
                set({ isLoading: true });
                try {
                    const projects = await CloudService.listProjects();
                    set({ cloudProjects: projects });
                } catch (e) { console.error(e); }
                set({ isLoading: false });
            },

            loadFromCloud: async (id) => {
                set({ isLoading: true });
                try {
                    const project = await CloudService.loadProject(id);
                    if (project && project.data) {
                        get()._pushUndo('Load from Cloud');

                        // For now, we assume simple data structure compatibility
                        const data = project.data;
                        set({
                            projectName: data.name || data.projectName || 'Untitled',
                            bpm: data.bpm,
                            key: data.key,
                            scale: data.scale,
                            timeSignature: data.timeSignature,
                            tracks: data.tracks, // Note: Buffer IDs must be valid in current session or re-hydrated
                            trackFolders: data.trackFolders || [],
                        });

                        // In a real app we'd need to re-download audio files here given the bufferIds
                        // For this local-storage mock, we assume buffers are still in AudioBufferManager memory
                        // which is true if we haven't refreshed the page. 
                        // A full implementation would serialize buffers to IndexedDB.
                    }
                } catch (e) {
                    console.error("Failed to load project", e);
                }
                set({ isLoading: false });
            },

            saveToCloud: async () => {
                set({ isSaving: true });
                const state = get();
                const projectData = {
                    id: state.tracks.length > 0 ? (state.tracks[0].id + '_proj') : uid(),
                    name: state.projectName,
                    bpm: state.bpm,
                    key: state.key,
                    scale: state.scale,
                    timeSignature: state.timeSignature,
                    tracks: state.tracks,
                    trackFolders: state.trackFolders,
                };

                const success = await CloudService.saveProject(projectData);
                if (success) {
                    await get().fetchProjects();
                }
                set({ isSaving: false });
                return success;
            },

            // ─── Version History (Phase 33) ───
            saveVersion: (label = 'Manual Save') => {
                const archive = get().archiveProject();
                const versions = JSON.parse(localStorage.getItem('orpheus_versions') || '[]');
                versions.push({ label, data: archive, timestamp: archive.timestamp });
                if (versions.length > 20) versions.shift();
                localStorage.setItem('orpheus_versions', JSON.stringify(versions));
            },

            getVersionHistory: () => {
                return JSON.parse(localStorage.getItem('orpheus_versions') || '[]');
            },

            restoreVersion: (index) => {
                const versions = JSON.parse(localStorage.getItem('orpheus_versions') || '[]');
                if (!versions[index]) return;
                get()._pushUndo('Restore Version');
                const data = versions[index].data;
                set({
                    tracks: data.tracks,
                    trackFolders: data.trackFolders || [],
                    bpm: data.bpm,
                    projectName: data.projectName,
                    key: data.key,
                    scale: data.scale,
                });
            },

            // ─── Tempo/Key Changes Mid-Project (Phase 34) ───
            tempoChanges: [],
            addTempoChange: (beat, bpm) => {
                set(state => ({
                    tempoChanges: [...state.tempoChanges, { beat, bpm }].sort((a, b) => a.beat - b.beat)
                }));
            },

            keyChanges: [],
            addKeyChange: (beat, key, scale) => {
                set(state => ({
                    keyChanges: [...state.keyChanges, { beat, key, scale }].sort((a, b) => a.beat - b.beat)
                }));
            },

            // ─── Audio Cleanup ───
            cleanAudioClip: async (trackId, clipId, options) => {
                const state = get();
                const track = state.tracks.find(t => t.id === trackId);
                const clip = track?.clips.find(c => c.id === clipId);
                if (!clip || !clip.bufferId) return;

                const entry = audioBufferManager.getBuffer(clip.bufferId);
                if (!entry) return;

                get()._pushUndo('Clean Audio');

                try {
                    // Use main context or fallback
                    const ctx = audioEngine.context || new AudioContext();

                    const { cleaned, stats } = await AudioCleaner.clean(
                        ctx,
                        entry.buffer,
                        options
                    );

                    // Save new buffer
                    const newId = await audioBufferManager.addBuffer(cleaned);

                    set(state => ({
                        tracks: state.tracks.map(t =>
                            t.id === trackId ? {
                                ...t,
                                clips: t.clips.map(c =>
                                    c.id === clipId ? {
                                        ...c,
                                        bufferId: newId,
                                        cleaned: true,
                                        cleanOptions: options,
                                        cleanedAt: new Date().toISOString(),
                                    } : c
                                )
                            } : t
                        )
                    }));
                    return stats;
                } catch (e) {
                    console.error("Audio cleanup failed", e);
                    return { error: e.message };
                }
            },

            // ─── Audio-to-MIDI (single clip) ───
            convertToMidi: async (trackId, clipId, options) => {
                get()._pushUndo('Convert to MIDI');
                const state = get();
                const track = state.tracks.find(t => t.id === trackId);
                const clip = track?.clips?.find(c => c.id === clipId);
                if (!clip || !clip.bufferId) throw new Error('Clip not found');

                const entry = audioBufferManager.getBuffer(clip.bufferId);
                if (!entry) return;

                try {
                    const ctx = audioEngine.context || new AudioContext();
                    const { tracks } = await AudioToMidi.convert(
                        ctx,
                        entry.buffer,
                        { ...options, bpm: state.bpm }
                    );

                    if (tracks && tracks.length > 0) {
                        const newTracks = tracks.map((midiTrack, i) => ({
                            id: uid(),
                            name: midiTrack.name,
                            type: 'midi',
                            volume: 0.8,
                            pan: 0,
                            muted: false,
                            solo: false,
                            color: getTrackColor(state.tracks.length + i + 1),
                            clips: [{
                                id: uid(),
                                startBeat: clip.startBeat,
                                lengthBeats: clip.lengthBeats,
                                notes: midiTrack.notes,
                                type: 'midi',
                                name: `${clip.name} (MIDI)`,
                                sourceClipId: clipId,
                            }],
                        }));

                        set(prev => ({
                            tracks: [...prev.tracks, ...newTracks]
                        }));
                    }
                } catch (e) {
                    console.error("Audio to MIDI failed", e);
                }
            },

            // ─── Audio-to-MIDI (full track with stem separation) ───
            convertTrackToMidi: async (trackId, options) => {
                get()._pushUndo('Convert Track to MIDI (Stems)');
                const state = get();
                const track = state.tracks.find(t => t.id === trackId);
                // For simplicity, find the first audio clip
                const clip = track?.clips.find(c => c.type === 'audio' && c.bufferId);

                if (!track || !clip) return;

                const entry = audioBufferManager.getBuffer(clip.bufferId);
                if (!entry) return;

                set({ isLoading: true });

                try {
                    const ctx = audioEngine.context || new AudioContext();
                    const { tracks, vocalBuffer } = await AudioToMidi.convert(
                        ctx,
                        entry.buffer,
                        { ...options, bpm: state.bpm, mode: 'stems' }
                    );

                    const newTracks = [];

                    // Add MIDI tracks for stems
                    if (tracks) {
                        tracks.forEach((stem, i) => {
                            newTracks.push({
                                id: uid(),
                                name: `${track.name} - ${stem.name}`,
                                type: 'midi',
                                volume: 0.8,
                                pan: 0,
                                muted: false,
                                solo: false,
                                color: getTrackColor(state.tracks.length + i + 1),
                                clips: [{
                                    id: uid(),
                                    startBeat: clip.startBeat,
                                    lengthBeats: clip.lengthBeats,
                                    notes: stem.notes,
                                    type: 'midi',
                                    name: stem.name,
                                    sourceClipId: clip.id
                                }]
                            });
                        });
                    }

                    // Add Vocal Audio Track if present
                    if (vocalBuffer) {
                        const vocalBufferId = await audioBufferManager.addBuffer(vocalBuffer);
                        newTracks.push({
                            id: uid(),
                            name: `${track.name} - Vocals`,
                            type: 'audio',
                            volume: 0.8,
                            pan: 0,
                            muted: false,
                            solo: false,
                            color: '#FF69B4',
                            clips: [{
                                id: uid(),
                                startBeat: clip.startBeat,
                                lengthBeats: clip.lengthBeats,
                                bufferId: vocalBufferId,
                                type: 'audio',
                                name: 'Vocals',
                            }]
                        });
                    }

                    set(prev => ({
                        tracks: [...prev.tracks, ...newTracks],
                        isLoading: false
                    }));

                } catch (e) {
                    console.error("Stem conversion failed", e);
                    set({ isLoading: false });
                }
            },

            // ─── Timeline Length ───
            timelineLength: 64, // Default: 16 bars of 4/4

            setTimelineLength: (totalBeats) => {
                set({ timelineLength: Math.max(4, totalBeats) });
            },

            stretchTimeline: (factor) => {
                get()._pushUndo('Stretch Timeline');
                set(state => ({
                    tracks: state.tracks.map(t => ({
                        ...t,
                        clips: t.clips.map(c => ({
                            ...c,
                            startBeat: (c.startBeat || 0) * factor,
                            lengthBeats: (c.lengthBeats || 1) * factor,
                            notes: c.notes?.map(n => ({
                                ...n,
                                startBeat: n.startBeat * factor,
                                lengthBeats: n.lengthBeats * factor,
                            })),
                        }))
                    })),
                    timelineLength: Math.ceil((state.timelineLength || 64) * factor),
                }));
            },

            trimTimeline: (startBeat, endBeat) => {
                get()._pushUndo('Trim Timeline');
                set(state => ({
                    tracks: state.tracks.map(t => ({
                        ...t,
                        clips: t.clips
                            .filter(c => {
                                const clipEnd = (c.startBeat || 0) + (c.lengthBeats || 0);
                                return clipEnd > startBeat && (c.startBeat || 0) < endBeat;
                            })
                            .map(c => ({
                                ...c,
                                startBeat: Math.max(0, (c.startBeat || 0) - startBeat),
                            }))
                    })),
                    timelineLength: endBeat - startBeat,
                }));
            },

            // ─── Track Extend ───
            extendTrack: async (trackId, clipId, targetBars) => {
                const state = get();
                const track = state.tracks.find(t => t.id === trackId);
                const clip = track?.clips.find(c => c.id === clipId);

                if (!track || !clip || clip.type !== 'audio' || !clip.bufferId) return;

                // get buffer
                const entry = audioBufferManager.getBuffer(clip.bufferId);
                if (!entry || !entry.buffer) {
                    console.warn('Buffer not found for extension');
                    return;
                }

                // Push undo state before async op
                get()._pushUndo('Extend Track');

                const beatsPerBar = (state.timeSignature?.[0] || 4);
                // Calculate total target length (original + extension)
                // The UI passes "targetBars" as the NEW total length or extension amount? 
                // In the UI I passed `Math.ceil(clip.lengthBeats / 4) * 2` which is total bars * 2. 
                // So targetBars represents the TOTAL desired length in bars.

                // TrackExtender.extend takes options.
                // We want to extend TO a specific length.

                const currentBars = clip.lengthBeats / beatsPerBar;
                // The UI argument I named 'targetBars' in the store action, but usage in UI was:
                // extendTrack(..., existingBars * 2) 

                // Let's assume targetBars is the TOTAL desired bar count
                const targetDurationSeconds = (targetBars * beatsPerBar * 60) / state.bpm;

                // AudioContext access - we need a context to create buffers.
                // We can use a temporary OfflineAudioContext or try to access the main one.
                // audioBufferManager might have a context or we create one.
                const offlineCtx = new OfflineAudioContext(
                    entry.buffer.numberOfChannels,
                    Math.ceil(targetDurationSeconds * entry.buffer.sampleRate),
                    entry.buffer.sampleRate
                );

                console.log(`Extending clip ${clipId} to ${targetBars} bars...`);

                try {
                    const extendedBuffer = await TrackExtender.extend(offlineCtx, entry.buffer, {
                        targetLengthSeconds: targetDurationSeconds,
                        bpm: state.bpm,
                        beatsPerBar: beatsPerBar,
                        variation: 0.2
                    });

                    // Save new buffer
                    const newBufferId = await audioBufferManager.addBuffer(extendedBuffer);

                    set(prev => ({
                        tracks: prev.tracks.map(t => {
                            if (t.id !== trackId) return t;
                            return {
                                ...t,
                                clips: t.clips.map(c => {
                                    if (c.id !== clipId) return c;
                                    return {
                                        ...c,
                                        bufferId: newBufferId, // Update to new buffer
                                        lengthBeats: targetBars * beatsPerBar,
                                        extended: true,
                                        extendedBars: targetBars,
                                    };
                                })
                            };
                        })
                    }));
                    console.log('Extension complete');
                } catch (err) {
                    console.error('Track extension failed:', err);
                }
            },

            // ─── Volume Automation per Track ───
            updateTrackAutomation: (trackId, param, points) => {
                get()._pushUndo('Update Automation');
                set(state => ({
                    tracks: state.tracks.map(t =>
                        t.id === trackId ? {
                            ...t,
                            [`${param}Automation`]: points,
                        } : t
                    )
                }));
            },

            // ─── Assign Instrument to MIDI Note ───
            setClipInstrument: (trackId, clipId, instrumentId, instrumentName) => {
                set(state => ({
                    tracks: state.tracks.map(t =>
                        t.id === trackId ? {
                            ...t,
                            clips: t.clips.map(c =>
                                c.id === clipId ? { ...c, instrumentId, instrumentName } : c
                            )
                        } : t
                    )
                }));
            },
        }),
        {
            name: 'orpheus-project',
            version: 1,
            partialize: (state) => {
                // Exclude transient/non-serializable state
                const { undoStack, redoStack, undoLabels, _autoSaveTimer, _maxHistory, ...persistable } = state;
                return persistable;
            },
        }
    )
);
