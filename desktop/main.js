/**
 * Nedflix Desktop - Electron Main Process
 * Windows standalone version without authentication
 */

const { app, BrowserWindow, ipcMain, globalShortcut } = require('electron');
const path = require('path');
const express = require('express');
const fs = require('fs');

// Configuration
const PORT = 3000;
const MEDIA_PATHS = process.env.NEDFLIX_MEDIA_PATHS || 'C:\\Videos;D:\\Movies;D:\\TV Shows';

let mainWindow;
let server;

// Create Express server (embedded)
function createServer() {
    const expressApp = express();

    // Static files
    expressApp.use(express.static(path.join(__dirname, 'public')));
    expressApp.use(express.json());

    // Parse media paths from environment or config
    const mediaPaths = MEDIA_PATHS.split(';').filter(p => p.trim());

    // API: Get libraries (directories)
    expressApp.get('/api/libraries', (req, res) => {
        const libraries = mediaPaths.map(p => ({
            path: p,
            name: path.basename(p)
        })).filter(lib => {
            try {
                return fs.existsSync(lib.path) && fs.statSync(lib.path).isDirectory();
            } catch {
                return false;
            }
        });
        res.json(libraries);
    });

    // API: Browse directory
    expressApp.get('/api/browse', (req, res) => {
        const requestedPath = req.query.path;

        if (!requestedPath) {
            return res.status(400).json({ error: 'Path required' });
        }

        // Security: Ensure path is within allowed media paths
        const normalizedPath = path.normalize(requestedPath);
        const isAllowed = mediaPaths.some(mp => normalizedPath.startsWith(path.normalize(mp)));

        if (!isAllowed) {
            return res.status(403).json({ error: 'Access denied' });
        }

        try {
            const items = fs.readdirSync(normalizedPath, { withFileTypes: true });

            const fileList = items.map(item => {
                const fullPath = path.join(normalizedPath, item.name);
                let stats;
                try {
                    stats = fs.statSync(fullPath);
                } catch {
                    return null;
                }

                const isDir = item.isDirectory();
                const isVideo = !isDir && /\.(mp4|webm|ogg|avi|mkv|mov|m4v|wmv)$/i.test(item.name);
                const isAudio = !isDir && /\.(mp3|m4a|flac|wav|aac|ogg|wma|opus|aiff)$/i.test(item.name);

                return {
                    name: item.name,
                    path: fullPath,
                    isDirectory: isDir,
                    isVideo: isVideo,
                    isAudio: isAudio,
                    size: stats.size
                };
            }).filter(Boolean);

            // Sort: folders first, then by name
            fileList.sort((a, b) => {
                if (a.isDirectory && !b.isDirectory) return -1;
                if (!a.isDirectory && b.isDirectory) return 1;
                return a.name.localeCompare(b.name);
            });

            const parentPath = path.dirname(normalizedPath);
            const canGoUp = mediaPaths.some(mp => parentPath.startsWith(path.normalize(mp)));

            res.json({
                currentPath: normalizedPath,
                parentPath: canGoUp ? parentPath : null,
                canGoUp,
                items: fileList
            });
        } catch (error) {
            res.status(500).json({ error: error.message });
        }
    });

    // API: Stream video
    expressApp.get('/api/video', (req, res) => {
        const videoPath = req.query.path;

        if (!videoPath) {
            return res.status(400).send('Video path required');
        }

        const normalizedPath = path.normalize(videoPath);
        const isAllowed = mediaPaths.some(mp => normalizedPath.startsWith(path.normalize(mp)));

        if (!isAllowed) {
            return res.status(403).send('Access denied');
        }

        try {
            const stat = fs.statSync(normalizedPath);
            const fileSize = stat.size;
            const range = req.headers.range;

            if (range) {
                const parts = range.replace(/bytes=/, '').split('-');
                const start = parseInt(parts[0], 10);
                const end = parts[1] ? parseInt(parts[1], 10) : fileSize - 1;
                const chunkSize = end - start + 1;

                const file = fs.createReadStream(normalizedPath, { start, end });
                const ext = path.extname(normalizedPath).toLowerCase();
                const mimeTypes = {
                    '.mp4': 'video/mp4',
                    '.webm': 'video/webm',
                    '.mkv': 'video/x-matroska',
                    '.avi': 'video/x-msvideo',
                    '.mov': 'video/quicktime',
                    '.m4v': 'video/mp4',
                    '.wmv': 'video/x-ms-wmv'
                };

                res.writeHead(206, {
                    'Content-Range': `bytes ${start}-${end}/${fileSize}`,
                    'Accept-Ranges': 'bytes',
                    'Content-Length': chunkSize,
                    'Content-Type': mimeTypes[ext] || 'video/mp4'
                });

                file.pipe(res);
            } else {
                res.writeHead(200, {
                    'Content-Length': fileSize,
                    'Content-Type': 'video/mp4'
                });
                fs.createReadStream(normalizedPath).pipe(res);
            }
        } catch (error) {
            res.status(500).send('Error streaming video');
        }
    });

    // API: Stream audio
    expressApp.get('/api/audio', (req, res) => {
        const audioPath = req.query.path;

        if (!audioPath) {
            return res.status(400).send('Audio path required');
        }

        const normalizedPath = path.normalize(audioPath);
        const isAllowed = mediaPaths.some(mp => normalizedPath.startsWith(path.normalize(mp)));

        if (!isAllowed) {
            return res.status(403).send('Access denied');
        }

        try {
            const stat = fs.statSync(normalizedPath);
            const ext = path.extname(normalizedPath).toLowerCase();
            const mimeTypes = {
                '.mp3': 'audio/mpeg',
                '.m4a': 'audio/mp4',
                '.flac': 'audio/flac',
                '.wav': 'audio/wav',
                '.aac': 'audio/aac',
                '.ogg': 'audio/ogg',
                '.wma': 'audio/x-ms-wma',
                '.opus': 'audio/opus'
            };

            res.writeHead(200, {
                'Content-Length': stat.size,
                'Content-Type': mimeTypes[ext] || 'audio/mpeg'
            });
            fs.createReadStream(normalizedPath).pipe(res);
        } catch (error) {
            res.status(500).send('Error streaming audio');
        }
    });

    // API: Get user (mock - no auth in desktop version)
    expressApp.get('/api/user', (req, res) => {
        res.json({
            authenticated: true,
            user: {
                id: 'desktop-user',
                displayName: 'Desktop User',
                provider: 'local',
                isAdmin: true
            }
        });
    });

    // API: Get/save settings
    let userSettings = {
        theme: 'dark',
        streaming: {
            quality: 'auto',
            volume: 80,
            playbackSpeed: 1,
            autoplay: false,
            subtitles: true
        }
    };

    expressApp.get('/api/settings', (req, res) => {
        res.json(userSettings);
    });

    expressApp.post('/api/settings', (req, res) => {
        userSettings = { ...userSettings, ...req.body };
        res.json({ success: true, settings: userSettings });
    });

    // Serve index for SPA
    expressApp.get('*', (req, res) => {
        res.sendFile(path.join(__dirname, 'public', 'index.html'));
    });

    return expressApp;
}

// Create Electron window
function createWindow() {
    mainWindow = new BrowserWindow({
        width: 1400,
        height: 900,
        minWidth: 800,
        minHeight: 600,
        backgroundColor: '#0a0a0f',
        webPreferences: {
            nodeIntegration: false,
            contextIsolation: true,
            preload: path.join(__dirname, 'preload.js')
        },
        frame: true,
        titleBarStyle: 'default',
        icon: path.join(__dirname, 'icon.png')
    });

    // Load the app
    mainWindow.loadURL(`http://localhost:${PORT}`);

    // Open DevTools in development
    if (process.env.NODE_ENV === 'development') {
        mainWindow.webContents.openDevTools();
    }

    // Fullscreen toggle with F11
    mainWindow.on('enter-full-screen', () => {
        mainWindow.webContents.send('fullscreen-change', true);
    });

    mainWindow.on('leave-full-screen', () => {
        mainWindow.webContents.send('fullscreen-change', false);
    });

    mainWindow.on('closed', () => {
        mainWindow = null;
    });
}

// Xbox/Gamepad controller support
function setupGamepadSupport() {
    // Register global shortcuts for media keys
    globalShortcut.register('MediaPlayPause', () => {
        mainWindow?.webContents.send('media-key', 'playpause');
    });

    globalShortcut.register('MediaStop', () => {
        mainWindow?.webContents.send('media-key', 'stop');
    });

    globalShortcut.register('MediaPreviousTrack', () => {
        mainWindow?.webContents.send('media-key', 'previous');
    });

    globalShortcut.register('MediaNextTrack', () => {
        mainWindow?.webContents.send('media-key', 'next');
    });

    // F11 for fullscreen
    globalShortcut.register('F11', () => {
        if (mainWindow) {
            mainWindow.setFullScreen(!mainWindow.isFullScreen());
        }
    });
}

// App ready
app.whenReady().then(() => {
    // Start embedded server
    const expressApp = createServer();
    server = expressApp.listen(PORT, () => {
        console.log(`Nedflix Desktop server running on port ${PORT}`);
    });

    createWindow();
    setupGamepadSupport();

    app.on('activate', () => {
        if (BrowserWindow.getAllWindows().length === 0) {
            createWindow();
        }
    });
});

// Quit when all windows are closed (except on macOS)
app.on('window-all-closed', () => {
    if (process.platform !== 'darwin') {
        globalShortcut.unregisterAll();
        if (server) server.close();
        app.quit();
    }
});

// IPC handlers
ipcMain.handle('get-media-paths', () => {
    return MEDIA_PATHS.split(';').filter(p => p.trim());
});

ipcMain.handle('toggle-fullscreen', () => {
    if (mainWindow) {
        mainWindow.setFullScreen(!mainWindow.isFullScreen());
    }
});
