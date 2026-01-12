/**
 * Nedflix Desktop - Electron Main Process
 * Standalone version without authentication
 * Supports Windows and Linux
 */

const { app, BrowserWindow, ipcMain, globalShortcut } = require('electron');
const path = require('path');
const express = require('express');
const fs = require('fs');
const http = require('http');
const https = require('https');
const zlib = require('zlib');

// Configuration
const PORT = 3000;
const CONFIG_FILE = path.join(app.getPath('userData'), 'nedflix-config.json');

// Load or initialize media paths and settings
let mediaPaths = [];
let iptvSettings = {
    playlistUrl: '',
    epgUrl: ''
};
let ersatztvSettings = {
    url: ''  // e.g., 'http://192.168.1.100:8409'
};

// IPTV cache
let iptvChannelsCache = null;
let iptvEpgCache = null;
let lastPlaylistUpdate = 0;
let lastEpgUpdate = 0;
const CACHE_TTL = 30 * 60 * 1000; // 30 minutes

function loadConfig() {
    try {
        if (fs.existsSync(CONFIG_FILE)) {
            const config = JSON.parse(fs.readFileSync(CONFIG_FILE, 'utf8'));
            mediaPaths = config.mediaPaths || [];
            iptvSettings = config.iptv || { playlistUrl: '', epgUrl: '' };
            ersatztvSettings = config.ersatztv || { url: '' };
        }
    } catch (error) {
        console.error('Failed to load config:', error);
        mediaPaths = [];
    }

    // Fall back to environment variable if no paths configured
    if (mediaPaths.length === 0 && process.env.NEDFLIX_MEDIA_PATHS) {
        mediaPaths = process.env.NEDFLIX_MEDIA_PATHS.split(';').filter(p => p.trim());
    }

    // IPTV environment variables
    if (!iptvSettings.playlistUrl && process.env.IPTV_PLAYLIST_URL) {
        iptvSettings.playlistUrl = process.env.IPTV_PLAYLIST_URL;
    }
    if (!iptvSettings.epgUrl && process.env.IPTV_EPG_URL) {
        iptvSettings.epgUrl = process.env.IPTV_EPG_URL;
    }

    // ErsatzTV environment variable
    if (!ersatztvSettings.url && process.env.ERSATZTV_URL) {
        ersatztvSettings.url = process.env.ERSATZTV_URL;
    }
}

function saveConfig() {
    try {
        const configDir = path.dirname(CONFIG_FILE);
        if (!fs.existsSync(configDir)) {
            fs.mkdirSync(configDir, { recursive: true });
        }
        fs.writeFileSync(CONFIG_FILE, JSON.stringify({
            mediaPaths,
            iptv: iptvSettings,
            ersatztv: ersatztvSettings
        }, null, 2));
    } catch (error) {
        console.error('Failed to save config:', error);
    }
}

// ==================== IPTV Helper Functions ====================

/**
 * Fetch content from URL
 */
function fetchContent(urlOrPath) {
    return new Promise((resolve, reject) => {
        const client = urlOrPath.startsWith('https') ? https : http;

        client.get(urlOrPath, { timeout: 30000 }, (res) => {
            // Handle redirects
            if (res.statusCode === 301 || res.statusCode === 302) {
                return fetchContent(res.headers.location).then(resolve).catch(reject);
            }

            const chunks = [];
            const isGzipped = res.headers['content-encoding'] === 'gzip';
            const stream = isGzipped ? res.pipe(zlib.createGunzip()) : res;

            stream.on('data', chunk => chunks.push(chunk));
            stream.on('end', () => resolve(Buffer.concat(chunks).toString('utf8')));
            stream.on('error', reject);
        }).on('error', reject);
    });
}

/**
 * Parse M3U playlist
 */
function parseM3U(content) {
    const lines = content.split('\n').map(l => l.trim()).filter(l => l);
    const channels = [];
    let currentChannel = null;

    for (let i = 0; i < lines.length; i++) {
        const line = lines[i];

        if (line.startsWith('#EXTINF:')) {
            // Parse channel info
            const match = line.match(/#EXTINF:(-?\d+)\s*(.*),(.*)$/);
            if (match) {
                currentChannel = {
                    name: match[3].trim(),
                    tvgId: '',
                    tvgName: '',
                    tvgLogo: '',
                    group: 'Uncategorized'
                };

                const attrs = match[2];
                const tvgId = attrs.match(/tvg-id="([^"]*)"/);
                const tvgLogo = attrs.match(/tvg-logo="([^"]*)"/);
                const groupTitle = attrs.match(/group-title="([^"]*)"/);

                if (tvgId) currentChannel.tvgId = tvgId[1];
                if (tvgLogo) currentChannel.tvgLogo = tvgLogo[1];
                if (groupTitle) currentChannel.group = groupTitle[1] || 'Uncategorized';
            }
        } else if (!line.startsWith('#') && currentChannel) {
            currentChannel.url = line;
            currentChannel.id = currentChannel.tvgId || `channel-${channels.length}`;
            channels.push(currentChannel);
            currentChannel = null;
        }
    }

    return channels;
}

/**
 * Parse XMLTV EPG data
 */
function parseEPG(content) {
    const epg = { channels: {}, programs: {} };

    // Parse channels
    const channelRegex = /<channel id="([^"]*)">([\s\S]*?)<\/channel>/g;
    let match;

    while ((match = channelRegex.exec(content)) !== null) {
        const id = match[1];
        const channelContent = match[2];
        const nameMatch = channelContent.match(/<display-name[^>]*>([^<]*)<\/display-name>/);
        epg.channels[id] = {
            id,
            name: nameMatch ? nameMatch[1] : id
        };
    }

    // Parse programs
    const programRegex = /<programme start="([^"]*)" stop="([^"]*)" channel="([^"]*)">([\s\S]*?)<\/programme>/g;

    while ((match = programRegex.exec(content)) !== null) {
        const start = parseXMLTVDate(match[1]);
        const stop = parseXMLTVDate(match[2]);
        const channelId = match[3];
        const programContent = match[4];

        const titleMatch = programContent.match(/<title[^>]*>([^<]*)<\/title>/);

        if (!epg.programs[channelId]) {
            epg.programs[channelId] = [];
        }

        epg.programs[channelId].push({
            start,
            stop,
            title: titleMatch ? titleMatch[1] : 'Unknown Program'
        });
    }

    // Sort programs by start time
    for (const channelId in epg.programs) {
        epg.programs[channelId].sort((a, b) => a.start - b.start);
    }

    return epg;
}

/**
 * Parse XMLTV date format
 */
function parseXMLTVDate(dateStr) {
    const match = dateStr.match(/(\d{4})(\d{2})(\d{2})(\d{2})(\d{2})(\d{2})/);
    if (!match) return Date.now();

    const [, year, month, day, hour, min, sec] = match;
    return new Date(`${year}-${month}-${day}T${hour}:${min}:${sec}`).getTime();
}

/**
 * Load IPTV playlist
 */
async function loadIptvPlaylist() {
    if (!iptvSettings.playlistUrl) return [];

    const now = Date.now();
    if (iptvChannelsCache && (now - lastPlaylistUpdate) < CACHE_TTL) {
        return iptvChannelsCache;
    }

    try {
        const content = await fetchContent(iptvSettings.playlistUrl);
        iptvChannelsCache = parseM3U(content);
        lastPlaylistUpdate = now;
        console.log(`Loaded ${iptvChannelsCache.length} IPTV channels`);
        return iptvChannelsCache;
    } catch (error) {
        console.error('Failed to load IPTV playlist:', error.message);
        return iptvChannelsCache || [];
    }
}

/**
 * Load EPG data
 */
async function loadIptvEpg() {
    if (!iptvSettings.epgUrl) return { channels: {}, programs: {} };

    const now = Date.now();
    if (iptvEpgCache && (now - lastEpgUpdate) < CACHE_TTL) {
        return iptvEpgCache;
    }

    try {
        const content = await fetchContent(iptvSettings.epgUrl);
        iptvEpgCache = parseEPG(content);
        lastEpgUpdate = now;
        console.log(`Loaded EPG with ${Object.keys(iptvEpgCache.channels).length} channels`);
        return iptvEpgCache;
    } catch (error) {
        console.error('Failed to load EPG:', error.message);
        return iptvEpgCache || { channels: {}, programs: {} };
    }
}

/**
 * Get channels grouped by category
 */
function getChannelsByGroup(channels) {
    const groups = {};
    for (const channel of channels) {
        const group = channel.group || 'Uncategorized';
        if (!groups[group]) groups[group] = [];
        groups[group].push(channel);
    }
    return groups;
}

// ==================== ErsatzTV Helper Functions ====================

/**
 * Make request to ErsatzTV API
 */
function ersatztvRequest(endpoint) {
    return new Promise((resolve, reject) => {
        if (!ersatztvSettings.url) {
            reject(new Error('ErsatzTV URL not configured'));
            return;
        }

        const url = new URL(endpoint, ersatztvSettings.url);
        const client = url.protocol === 'https:' ? https : http;

        client.get(url.href, { timeout: 10000 }, (res) => {
            let data = '';
            res.on('data', chunk => data += chunk);
            res.on('end', () => {
                try {
                    resolve(JSON.parse(data));
                } catch {
                    resolve(data);
                }
            });
        }).on('error', reject);
    });
}

let mainWindow;
let server;

// Create Express server (embedded)
function createServer() {
    const expressApp = express();

    // Static files
    expressApp.use(express.static(path.join(__dirname, 'public')));
    expressApp.use(express.json());

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

    // ==================== IPTV API Endpoints ====================

    // API: Get IPTV channels
    expressApp.get('/api/iptv/channels', async (req, res) => {
        try {
            if (!iptvSettings.playlistUrl) {
                return res.json({ configured: false, channels: [], groups: {}, groupNames: [] });
            }

            const channels = await loadIptvPlaylist();
            const groups = getChannelsByGroup(channels);
            const groupNames = Object.keys(groups).sort();

            res.json({
                configured: true,
                channels,
                groups,
                groupNames
            });
        } catch (error) {
            res.status(500).json({ error: error.message });
        }
    });

    // API: Get EPG data
    expressApp.get('/api/iptv/epg', async (req, res) => {
        try {
            const epg = await loadIptvEpg();
            res.json(epg);
        } catch (error) {
            res.status(500).json({ error: error.message });
        }
    });

    // API: Refresh IPTV cache
    expressApp.post('/api/iptv/refresh', (req, res) => {
        iptvChannelsCache = null;
        iptvEpgCache = null;
        lastPlaylistUpdate = 0;
        lastEpgUpdate = 0;
        res.json({ success: true });
    });

    // API: Stream proxy for IPTV
    expressApp.get('/api/iptv/stream', (req, res) => {
        const streamUrl = req.query.url;
        if (!streamUrl) {
            return res.status(400).send('Stream URL required');
        }

        const client = streamUrl.startsWith('https') ? https : http;

        const proxyReq = client.get(streamUrl, {
            headers: {
                'User-Agent': 'Nedflix/1.0'
            }
        }, (proxyRes) => {
            // Copy headers
            const contentType = proxyRes.headers['content-type'];
            if (contentType) res.setHeader('Content-Type', contentType);

            // Pipe the response
            proxyRes.pipe(res);
        });

        proxyReq.on('error', (err) => {
            console.error('Stream proxy error:', err.message);
            res.status(500).send('Failed to proxy stream');
        });

        req.on('close', () => {
            proxyReq.destroy();
        });
    });

    // API: Get/save IPTV settings
    expressApp.get('/api/iptv/settings', (req, res) => {
        res.json(iptvSettings);
    });

    expressApp.post('/api/iptv/settings', (req, res) => {
        iptvSettings = { ...iptvSettings, ...req.body };
        // Clear cache when settings change
        iptvChannelsCache = null;
        iptvEpgCache = null;
        lastPlaylistUpdate = 0;
        lastEpgUpdate = 0;
        saveConfig();
        res.json({ success: true, settings: iptvSettings });
    });

    // ==================== ErsatzTV/Channels API Endpoints ====================

    // API: Get ErsatzTV status and channels
    expressApp.get('/api/ersatztv/status', async (req, res) => {
        try {
            if (!ersatztvSettings.url) {
                return res.json({
                    available: false,
                    configured: false,
                    error: 'ErsatzTV URL not configured'
                });
            }

            // Check health
            await ersatztvRequest('/api/health');

            // Get channels
            const channels = await ersatztvRequest('/api/channels');

            res.json({
                available: true,
                configured: true,
                url: ersatztvSettings.url,
                channels: channels || [],
                playlistUrl: `${ersatztvSettings.url}/iptv/channels.m3u`,
                epgUrl: `${ersatztvSettings.url}/iptv/xmltv.xml`
            });
        } catch (error) {
            res.json({
                available: false,
                configured: !!ersatztvSettings.url,
                error: error.message
            });
        }
    });

    // API: Get ErsatzTV channels
    expressApp.get('/api/ersatztv/channels', async (req, res) => {
        try {
            const channels = await ersatztvRequest('/api/channels');
            res.json(channels || []);
        } catch (error) {
            res.status(500).json({ error: error.message });
        }
    });

    // API: Get channel guide
    expressApp.get('/api/ersatztv/channels/:id/guide', async (req, res) => {
        try {
            const guide = await ersatztvRequest(`/api/channels/${req.params.id}/guide`);
            res.json({ guide: guide || [] });
        } catch (error) {
            res.status(500).json({ error: error.message });
        }
    });

    // API: Get/save ErsatzTV settings
    expressApp.get('/api/ersatztv/settings', (req, res) => {
        res.json(ersatztvSettings);
    });

    expressApp.post('/api/ersatztv/settings', (req, res) => {
        ersatztvSettings = { ...ersatztvSettings, ...req.body };
        saveConfig();
        res.json({ success: true, settings: ersatztvSettings });
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
    // Load configuration
    loadConfig();

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
    return mediaPaths;
});

ipcMain.handle('set-media-paths', (event, paths) => {
    mediaPaths = paths || [];
    saveConfig();
    return { success: true, paths: mediaPaths };
});

ipcMain.handle('toggle-fullscreen', () => {
    if (mainWindow) {
        mainWindow.setFullScreen(!mainWindow.isFullScreen());
    }
});

ipcMain.handle('get-iptv-settings', () => {
    return iptvSettings;
});

ipcMain.handle('set-iptv-settings', (event, settings) => {
    iptvSettings = { ...iptvSettings, ...settings };
    // Clear cache when settings change
    iptvChannelsCache = null;
    iptvEpgCache = null;
    lastPlaylistUpdate = 0;
    lastEpgUpdate = 0;
    saveConfig();
    return { success: true, settings: iptvSettings };
});

ipcMain.handle('get-ersatztv-settings', () => {
    return ersatztvSettings;
});

ipcMain.handle('set-ersatztv-settings', (event, settings) => {
    ersatztvSettings = { ...ersatztvSettings, ...settings };
    saveConfig();
    return { success: true, settings: ersatztvSettings };
});
