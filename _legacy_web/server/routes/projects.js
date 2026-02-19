import express from 'express';
import prisma from '../db.js';

const router = express.Router();

// List all projects
router.get('/', async (req, res) => {
    try {
        const projects = await prisma.project.findMany({
            orderBy: { updatedAt: 'desc' },
            select: { id: true, name: true, updatedAt: true } // Don't fetch full data list
        });
        res.json(projects);
    } catch (error) {
        res.status(500).json({ error: error.message });
    }
});

// Get single project
router.get('/:id', async (req, res) => {
    try {
        const project = await prisma.project.findUnique({
            where: { id: req.params.id }
        });
        if (!project) return res.status(404).json({ error: 'Project not found' });
        res.json(project);
    } catch (error) {
        res.status(500).json({ error: error.message });
    }
});

// Create project
router.post('/', async (req, res) => {
    try {
        const { name, data, userId } = req.body;

        // Create default user if none exists (for demo)
        let user = await prisma.user.findFirst();
        if (!user) {
            user = await prisma.user.create({
                data: { email: 'demo@orpheus.app', name: 'Demo User' }
            });
        }

        const project = await prisma.project.create({
            data: {
                name,
                data,
                userId: userId || user.id
            }
        });
        res.json(project);
    } catch (error) {
        res.status(500).json({ error: error.message });
    }
});

// Update project
router.put('/:id', async (req, res) => {
    try {
        const { name, data } = req.body;
        const project = await prisma.project.update({
            where: { id: req.params.id },
            data: { name, data }
        });
        res.json(project);
    } catch (error) {
        res.status(500).json({ error: error.message });
    }
});

// Delete project
router.delete('/:id', async (req, res) => {
    try {
        await prisma.project.delete({
            where: { id: req.params.id }
        });
        res.json({ success: true });
    } catch (error) {
        res.status(500).json({ error: error.message });
    }
});

export default router;
