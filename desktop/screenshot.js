/**
 * Marketing screenshots of the Marquee UI. Serves the demo (VITE_ALLOW_MOCK=1) build with
 * a FAKE library list (so Settings shows generic folders, not real paths), then captures
 * each page with Electron. Run from desktop/:  npx electron screenshot.js
 */
const { app, BrowserWindow } = require('electron');
const express = require('express');
const path = require('path');
const fs = require('fs');

const DIST = path.join(__dirname, '..', 'web', 'dist');
const OUT = path.join(__dirname, '..', 'screenshots');
const PORT = 3999;

const PAGES = [
  ['01-home', '/'],
  ['02-movies', '/films'],
  ['03-series', '/series'],
  ['04-music', '/music?tab=albums'],
  ['05-books', '/audiobooks'],
  ['06-live', '/live'],
  ['07-settings', '/settings'],
];

// Fake libraries for the Settings screenshot (no real paths).
const FAKE_LIBS = [
  { path: '/media/Movies', name: 'Movies' },
  { path: '/media/TV Shows', name: 'TV Shows' },
  { path: '/media/Music', name: 'Music' },
  { path: '/media/Audiobooks', name: 'Audiobooks' },
];

const exApp = express();
exApp.get('/api/libraries', (_req, res) => res.json(FAKE_LIBS));
exApp.use('/api', (_req, res) => res.status(404).end()); // everything else -> client fixtures
exApp.use(express.static(DIST, { index: false }));
exApp.get('*', (_req, res) => res.sendFile(path.join(DIST, 'index.html')));

app.disableHardwareAcceleration();

app.whenReady().then(() => {
  const server = exApp.listen(PORT, async () => {
    if (!fs.existsSync(OUT)) fs.mkdirSync(OUT, { recursive: true });
    const win = new BrowserWindow({
      width: 1440,
      height: 900,
      show: false,
      backgroundColor: '#0b0c11',
      webPreferences: { backgroundThrottling: false },
    });
    win.webContents.setBackgroundThrottling(false);

    for (const [name, route] of PAGES) {
      await win.loadURL(`http://localhost:${PORT}${route}`);
      await new Promise((r) => setTimeout(r, 2200)); // fonts + fixtures + enter animation
      const img = await win.webContents.capturePage();
      fs.writeFileSync(path.join(OUT, `${name}.png`), img.toPNG());
      console.log('captured', name, '->', route);
    }
    win.destroy();
    server.close();
    app.quit();
  });
});
