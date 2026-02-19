// ============================================
// ORPHEUS DAW — Pseudo Cloud Service
// ============================================
// Simulates a cloud backend using LocalStorage for demonstration purposes.
// In a real app, this would make fetch() calls to a REST/GraphQL API.

const STORAGE_KEY = 'orpheus_cloud_projects';

export const CloudService = {

    // Simulate network delay
    _delay: (ms = 500) => new Promise(resolve => setTimeout(resolve, ms)),

    async listProjects() {
        await this._delay();
        try {
            const data = localStorage.getItem(STORAGE_KEY);
            return data ? JSON.parse(data) : [];
        } catch (e) {
            console.error('Cloud list error:', e);
            return [];
        }
    },

    async loadProject(id) {
        await this._delay(800);
        const projects = await this.listProjects();
        return projects.find(p => p.id === id) || null;
    },

    async saveProject(projectData) {
        await this._delay(1000);
        const projects = await this.listProjects();

        // Check if updating existing
        const index = projects.findIndex(p => p.id === projectData.id);

        const metadata = {
            id: projectData.id,
            name: projectData.name || 'Untitled Project',
            updatedAt: new Date().toISOString(),
            version: '1.0',
            data: projectData // In real app, might store large data separately (S3)
        };

        if (index >= 0) {
            projects[index] = metadata;
        } else {
            projects.push(metadata);
        }

        try {
            localStorage.setItem(STORAGE_KEY, JSON.stringify(projects));
            return true;
        } catch (e) {
            console.error('Cloud save error (Quota exceeded?):', e);
            alert('Failed to save to "Cloud" (LocalStorage quota exceeded).');
            return false;
        }
    },

    async deleteProject(id) {
        await this._delay();
        const projects = await this.listProjects();
        const filtered = projects.filter(p => p.id !== id);
        localStorage.setItem(STORAGE_KEY, JSON.stringify(filtered));
        return true;
    }
};
