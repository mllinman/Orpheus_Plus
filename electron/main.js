const { app, BrowserWindow, ipcMain, screen } = require('electron');
const path = require('path');
const fs = require('fs');

// Handle creating/removing shortcuts on Windows when installing/uninstalling.
if (require('electron-squirrel-startup')) {
    app.quit();
}

let mainWindow;

function createWindow() {
    const { width, height } = screen.getPrimaryDisplay().workAreaSize;

    mainWindow = new BrowserWindow({
        width: Math.min(1440, width),
        height: Math.min(900, height),
        minWidth: 1024,
        minHeight: 768,
        frame: true, // We have a custom top bar but native frame is safer for dragging unless we implement custom drag regions
        webPreferences: {
            preload: path.join(__dirname, 'preload.js'),
            nodeIntegration: false,
            contextIsolation: true,
            sandbox: false // Needed for some complex audio features? Usually safe to keep reliable.
        },
        backgroundColor: '#121212',
        icon: path.join(__dirname, '../public/icon.png') // Assuming icon exists, if not it uses default
    });

    // Load the app
    const isDev = !app.isPackaged;
    if (isDev) {
        mainWindow.loadURL('http://localhost:5173');
        // mainWindow.webContents.openDevTools();
    } else {
        mainWindow.loadFile(path.join(__dirname, '../dist/index.html'));
    }
}

app.whenReady().then(() => {
    createWindow();

    app.on('activate', function () {
        if (BrowserWindow.getAllWindows().length === 0) createWindow();
    });
});

app.on('window-all-closed', function () {
    if (process.platform !== 'darwin') app.quit();
});

// ─── IPC Handlers for File System Access ───

// Read directory contents
ipcMain.handle('read-dir', async (event, dirPath) => {
    try {
        const entries = await fs.promises.readdir(dirPath, { withFileTypes: true });
        return entries.map(entry => ({
            name: entry.name,
            isDirectory: entry.isDirectory(),
            path: path.join(dirPath, entry.name),
            size: entry.isFile() ? fs.statSync(path.join(dirPath, entry.name)).size : 0
        }));
    } catch (error) {
        console.error('Failed to read dir:', error);
        throw error;
    }
});

// Read file as ArrayBuffer (for audio)
ipcMain.handle('read-file', async (event, filePath) => {
    try {
        const buffer = await fs.promises.readFile(filePath);
        return buffer;
    } catch (error) {
        console.error('Failed to read file:', error);
        throw error;
    }
});

// Get common paths
ipcMain.handle('get-path', async (event, name) => {
    return app.getPath(name);
});
