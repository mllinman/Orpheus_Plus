const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('electronAPI', {
    readDir: (path) => ipcRenderer.invoke('read-dir', path),
    readFile: (path) => ipcRenderer.invoke('read-file', path),
    getPath: (name) => ipcRenderer.invoke('get-path', name)
});
