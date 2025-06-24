const { app, BrowserWindow, ipcMain } = require('electron');
const path = require('path');
const { spawn } = require('child_process');

let bridgeProcess;

function createWindow () {
  const win = new BrowserWindow({
    width: 800,
    height: 600,
    webPreferences: {
      preload: path.join(__dirname, 'preload.js'),
      contextIsolation: true,
      nodeIntegration: false
    }
  });

  win.loadFile('renderer/index.html');
}

app.whenReady().then(() => {
  bridgeProcess = spawn(path.join(__dirname, 'simconnect-bridge.exe'));

  bridgeProcess.stdout.on('data', (data) => {
    console.log(`Bridge: ${data.toString().trim()}`);
  });

  bridgeProcess.stderr.on('data', (data) => {
    console.error(`Bridge Error: ${data.toString().trim()}`);
  });

  ipcMain.on('send-command', (event, cmd) => {
    console.log(`Sending to bridge: ${cmd}`);
    if (bridgeProcess && bridgeProcess.stdin.writable) {
      bridgeProcess.stdin.write(cmd + '\n');
    }
  });

  createWindow();
});

app.on('window-all-closed', () => {
  if (bridgeProcess) bridgeProcess.kill();
  if (process.platform !== 'darwin') app.quit();
});