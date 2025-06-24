
const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('api', {
  sendCommand: (cmd) => ipcRenderer.send('send-command', cmd)
});