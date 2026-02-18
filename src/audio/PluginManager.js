// ============================================
// ORPHEUS DAW — Plugin Manager
// ============================================
// Manages all built-in and imported plugins (VST/VST2/VST3 metadata)

import { EFFECT_TYPES } from './EffectsProcessor';

const BUILT_IN_PLUGINS = [
    // Dynamics
    { id: 'comp', name: 'Orpheus Compressor', type: 'effect', category: 'Dynamics', format: 'Built-in', version: '1.0' },
    { id: 'limiter', name: 'Orpheus Limiter', type: 'effect', category: 'Dynamics', format: 'Built-in', version: '1.0' },
    { id: 'gate', name: 'Orpheus Gate', type: 'effect', category: 'Dynamics', format: 'Built-in', version: '1.0' },

    // EQ & Filter
    { id: 'eq', name: 'Orpheus EQ 4-Band', type: 'effect', category: 'EQ', format: 'Built-in', version: '1.0' },
    { id: 'filter', name: 'Orpheus Filter', type: 'effect', category: 'EQ', format: 'Built-in', version: '1.0' },

    // Modulation
    { id: 'chorus', name: 'Orpheus Chorus', type: 'effect', category: 'Modulation', format: 'Built-in', version: '1.0' },
    { id: 'flanger', name: 'Orpheus Flanger', type: 'effect', category: 'Modulation', format: 'Built-in', version: '1.0' },
    { id: 'phaser', name: 'Orpheus Phaser', type: 'effect', category: 'Modulation', format: 'Built-in', version: '1.0' },

    // Reverb & Delay
    { id: 'reverb', name: 'Orpheus Reverb', type: 'effect', category: 'Reverb & Delay', format: 'Built-in', version: '1.0' },
    { id: 'delay', name: 'Orpheus Delay', type: 'effect', category: 'Reverb & Delay', format: 'Built-in', version: '1.0' },

    // Utility
    { id: 'gain', name: 'Orpheus Utility', type: 'effect', category: 'Utility', format: 'Built-in', version: '1.0' },
    { id: 'stereo_width', name: 'Orpheus Stereo Widener', type: 'effect', category: 'Utility', format: 'Built-in', version: '1.0' },

    // Instruments
    { id: 'synth', name: 'Orpheus Synth', type: 'instrument', category: 'Synthesizer', format: 'Built-in', version: '1.0' },
    { id: 'sampler', name: 'Orpheus Sampler', type: 'instrument', category: 'Sampler', format: 'Built-in', version: '1.0' },
    { id: 'drum_machine', name: 'Orpheus Drum Machine', type: 'instrument', category: 'Drums', format: 'Built-in', version: '1.0' },

    // Mastering
    { id: 'mastering', name: 'Orpheus Mastering Suite', type: 'effect', category: 'Mastering', format: 'Built-in', version: '1.0' },
    { id: 'pitch_correct', name: 'Orpheus Pitch Corrector', type: 'effect', category: 'Pitch', format: 'Built-in', version: '1.0' },
    { id: 'stem_sep', name: 'Orpheus STEM Separator', type: 'effect', category: 'Utility', format: 'Built-in', version: '1.0' },
];

class PluginManager {
    constructor() {
        this.plugins = [...BUILT_IN_PLUGINS];
        this.importedPlugins = JSON.parse(localStorage.getItem('orpheus_imported_plugins') || '[]');
        this.scanPaths = JSON.parse(localStorage.getItem('orpheus_vst_paths') || '[]');
        this.listeners = new Set();
    }

    /**
     * Import a VST/VST2/VST3 plugin file
     * In a web context, we store the file metadata and map to built-in equivalents
     */
    async importPlugin(file) {
        const ext = file.name.split('.').pop().toLowerCase();
        const validExtensions = ['dll', 'vst', 'vst3', 'component', 'so'];

        if (!validExtensions.includes(ext)) {
            throw new Error(`Unsupported plugin format: .${ext}. Supported: .dll, .vst, .vst3, .component`);
        }

        // Extract plugin metadata
        const pluginEntry = {
            id: `vst_${Date.now()}_${Math.random().toString(36).slice(2, 6)}`,
            name: file.name.replace(/\.[^.]+$/, ''),
            type: 'effect', // Default, could detect from file
            category: 'External',
            format: ext.toUpperCase(),
            version: '—',
            fileSize: file.size,
            fileName: file.name,
            importedAt: new Date().toISOString(),
            // Web Audio can't natively load VSTs, so we map to closest built-in
            mappedTo: this._findClosestBuiltIn(file.name),
        };

        this.importedPlugins.push(pluginEntry);
        this._saveImported();
        this._notify();

        return pluginEntry;
    }

    _findClosestBuiltIn(fileName) {
        const lower = fileName.toLowerCase();
        if (lower.includes('comp')) return 'comp';
        if (lower.includes('eq') || lower.includes('equaliz')) return 'eq';
        if (lower.includes('reverb') || lower.includes('verb')) return 'reverb';
        if (lower.includes('delay')) return 'delay';
        if (lower.includes('chorus')) return 'chorus';
        if (lower.includes('limit')) return 'limiter';
        if (lower.includes('synth') || lower.includes('serum') || lower.includes('massive')) return 'synth';
        if (lower.includes('drum') || lower.includes('battery')) return 'drum_machine';
        if (lower.includes('sampl') || lower.includes('kontakt')) return 'sampler';
        return 'gain'; // Fallback
    }

    /**
     * Remove an imported plugin
     */
    removePlugin(id) {
        this.importedPlugins = this.importedPlugins.filter(p => p.id !== id);
        this._saveImported();
        this._notify();
    }

    /**
     * Get all plugins (built-in + imported)
     */
    getAllPlugins() {
        return [...this.plugins, ...this.importedPlugins];
    }

    /**
     * Get plugins by category
     */
    getByCategory(category) {
        return this.getAllPlugins().filter(p => p.category === category);
    }

    /**
     * Get all categories
     */
    getCategories() {
        const cats = new Set(this.getAllPlugins().map(p => p.category));
        return Array.from(cats);
    }

    /**
     * Add a VST scan path
     */
    addScanPath(path) {
        if (!this.scanPaths.includes(path)) {
            this.scanPaths.push(path);
            localStorage.setItem('orpheus_vst_paths', JSON.stringify(this.scanPaths));
            this._notify();
        }
    }

    removeScanPath(path) {
        this.scanPaths = this.scanPaths.filter(p => p !== path);
        localStorage.setItem('orpheus_vst_paths', JSON.stringify(this.scanPaths));
        this._notify();
    }

    _saveImported() {
        localStorage.setItem('orpheus_imported_plugins', JSON.stringify(this.importedPlugins));
    }

    subscribe(fn) {
        this.listeners.add(fn);
        return () => this.listeners.delete(fn);
    }

    _notify() {
        for (const fn of this.listeners) fn();
    }
}

export const pluginManager = new PluginManager();
