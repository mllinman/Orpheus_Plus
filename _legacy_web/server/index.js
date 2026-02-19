import express from 'express';
import path from 'path';
import cors from 'cors';
import prisma from './db.js';
import projectRoutes from './routes/projects.js';
import audioRoutes from './routes/audio.js';
import { fileURLToPath } from 'url';
import 'dotenv/config';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const app = express();
const PORT = process.env.PORT || 3000;

app.use(cors());
app.use(express.json({ limit: '50mb' }));

// Serve Uploads
app.use('/uploads', express.static(path.join(__dirname, 'uploads')));

// API Routes
app.use('/api/projects', projectRoutes);
app.use('/api/audio', audioRoutes);

app.get('/api/health', (req, res) => {
    res.json({ status: 'ok', timestamp: new Date() });
});

app.get('/api/users', async (req, res) => {
    try {
        const users = await prisma.user.findMany();
        res.json(users);
    } catch (error) {
        res.status(500).json({ error: error.message });
    }
});

// Serve static files in production
if (process.env.NODE_ENV === 'production') {
    app.use(express.static(path.join(__dirname, '../dist')));

    app.get(/.*/, (req, res) => {
        res.sendFile(path.join(__dirname, '../dist/index.html'));
    });
}

app.listen(PORT, () => {
    console.log(`Server running on port ${PORT}`);
});
