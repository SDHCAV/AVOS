//main process, opens desktop window & loads react

import { app, BrowserWindow } from 'electron';

   function createWindow() {
     const win = new BrowserWindow({
       width: 1280,
       height: 800,
       webPreferences: { contextIsolation: true, nodeIntegration: false },
     });
     win.loadURL('http://localhost:5173');
   }

   app.whenReady().then(createWindow);