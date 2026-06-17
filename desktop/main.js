/**
 * Nedflix Desktop - Electron Main Process
 * Standalone version without authentication
 * Supports Windows and Linux
 */

const { app, BrowserWindow, ipcMain, globalShortcut, dialog } = require('electron');
const path = require('path');
const express = require('express');
const fs = require('fs');
const http = require('http');
const https = require('https');
const zlib = require('zlib');
const registerMarqueeDesktop = require('./marquee-desktop');

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

// Remote Nedflix server settings
let remoteServers = [];  // Array of {url, name, enabled}

// User playback settings (persisted to config). streaming holds defaults applied to
// every player, including audioLanguage / subtitleLanguage.
let userSettings = {
    theme: 'dark',
    streaming: {
        quality: 'auto',
        volume: 80,
        playbackSpeed: 1,
        autoplay: false,
        subtitles: true,
        audioLanguage: 'eng',
        subtitleLanguage: 'off'
    }
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
            remoteServers = config.remoteServers || [];
            if (config.userSettings) {
                userSettings = { ...userSettings, ...config.userSettings, streaming: { ...userSettings.streaming, ...(config.userSettings.streaming || {}) } };
            }
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
            ersatztv: ersatztvSettings,
            remoteServers,
            userSettings
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
        // Local file (or file:// URL) — read from disk instead of over HTTP.
        if (!/^https?:\/\//i.test(urlOrPath)) {
            try {
                let filePath = urlOrPath.replace(/^file:\/\//i, '');
                if (/^\/[A-Za-z]:/.test(filePath)) filePath = filePath.slice(1); // /C:/... -> C:/...
                const buf = fs.readFileSync(filePath);
                const data = /\.gz$/i.test(filePath) ? zlib.gunzipSync(buf) : buf;
                return resolve(data.toString('utf8'));
            } catch (e) { return reject(e); }
        }

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

// ==================== Remote Server Helper Functions ====================

/**
 * Make request to a remote Nedflix server
 */
function remoteServerRequest(serverUrl, endpoint, options = {}) {
    return new Promise((resolve, reject) => {
        const url = new URL(endpoint, serverUrl);
        const client = url.protocol === 'https:' ? https : http;

        const reqOptions = {
            hostname: url.hostname,
            port: url.port,
            path: url.pathname + url.search,
            method: options.method || 'GET',
            timeout: options.timeout || 10000,
            headers: {
                'User-Agent': 'Nedflix-Desktop/1.0',
                ...options.headers
            }
        };

        const req = client.request(reqOptions, (res) => {
            let data = '';
            res.on('data', chunk => data += chunk);
            res.on('end', () => {
                try {
                    resolve({
                        status: res.statusCode,
                        data: JSON.parse(data),
                        headers: res.headers
                    });
                } catch {
                    resolve({
                        status: res.statusCode,
                        data: data,
                        headers: res.headers
                    });
                }
            });
        });

        req.on('error', reject);
        req.on('timeout', () => {
            req.destroy();
            reject(new Error('Request timeout'));
        });

        if (options.body) {
            req.write(JSON.stringify(options.body));
        }
        req.end();
    });
}

/**
 * Check if a remote server is reachable
 */
async function checkRemoteServer(serverUrl) {
    try {
        const response = await remoteServerRequest(serverUrl, '/api/user', { timeout: 5000 });
        return {
            reachable: response.status === 200,
            authenticated: response.data?.authenticated || false,
            user: response.data?.user || null
        };
    } catch (error) {
        return {
            reachable: false,
            error: error.message
        };
    }
}

/**
 * Fetch libraries from a remote server
 */
async function fetchRemoteLibraries(serverUrl) {
    try {
        const response = await remoteServerRequest(serverUrl, '/api/libraries');
        if (response.status === 200 && Array.isArray(response.data)) {
            return response.data.map(lib => ({
                ...lib,
                source: 'remote',
                serverUrl: serverUrl
            }));
        }
        return [];
    } catch (error) {
        console.error(`Failed to fetch libraries from ${serverUrl}:`, error.message);
        return [];
    }
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
// UI directory: prefer the bundled Marquee SPA build (web-dist, copied from web/dist at
// build time); fall back to the legacy desktop public/ app when it isn't present.
const UI_DIR = fs.existsSync(path.join(__dirname, 'web-dist', 'index.html'))
    ? path.join(__dirname, 'web-dist')
    : path.join(__dirname, 'public');

function createServer() {
    const expressApp = express();

    // Static files (Marquee SPA when available)
    expressApp.use(express.static(UI_DIR));
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

    const librariesList = () => mediaPaths.map(p => ({ path: p, name: path.basename(p) }));

    // API: Add a media library path (validates it's an existing directory)
    expressApp.post('/api/media-paths', (req, res) => {
        const p = req.body && req.body.path && path.normalize(req.body.path);
        if (!p) return res.status(400).json({ error: 'path required' });
        try {
            if (!fs.existsSync(p) || !fs.statSync(p).isDirectory()) return res.status(400).json({ error: 'Not an existing folder' });
        } catch { return res.status(400).json({ error: 'Invalid path' }); }
        if (!mediaPaths.includes(p)) { mediaPaths.push(p); saveConfig(); }
        res.json(librariesList());
    });

    // API: Remove a media library path
    expressApp.delete('/api/media-paths', (req, res) => {
        const p = req.query.path && path.normalize(req.query.path);
        mediaPaths = mediaPaths.filter(mp => mp !== p);
        saveConfig();
        res.json(librariesList());
    });

    // API: Open a native folder picker and add the chosen folder(s)
    expressApp.post('/api/media-paths/pick', async (req, res) => {
        try {
            const result = await dialog.showOpenDialog(mainWindow, { title: 'Add media library', properties: ['openDirectory', 'multiSelections'] });
            if (!result.canceled) {
                for (const p of result.filePaths) if (!mediaPaths.includes(p)) mediaPaths.push(p);
                saveConfig();
            }
            res.json(librariesList());
        } catch (e) { res.status(500).json({ error: e.message }); }
    });

    // ---- Window controls (frameless window) ----
    expressApp.post('/api/app/minimize', (req, res) => { if (mainWindow) mainWindow.minimize(); res.json({ ok: true }); });
    expressApp.post('/api/app/toggle-maximize', (req, res) => {
        if (mainWindow) { mainWindow.isMaximized() ? mainWindow.unmaximize() : mainWindow.maximize(); }
        res.json({ ok: true, maximized: mainWindow ? mainWindow.isMaximized() : false });
    });
    expressApp.post('/api/app/quit', (req, res) => { res.json({ ok: true }); app.quit(); });

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
    expressApp.get('/api/settings', (req, res) => {
        res.json(userSettings);
    });

    expressApp.post('/api/settings', (req, res) => {
        userSettings = { ...userSettings, ...req.body };
        saveConfig();
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

    // API: Pick a local M3U playlist file via native dialog
    expressApp.post('/api/iptv/pick-playlist', async (req, res) => {
        try {
            const r = await dialog.showOpenDialog(mainWindow, {
                title: 'Select M3U playlist',
                properties: ['openFile'],
                filters: [{ name: 'Playlists', extensions: ['m3u', 'm3u8'] }, { name: 'All files', extensions: ['*'] }],
            });
            if (!r.canceled && r.filePaths[0]) {
                iptvSettings.playlistUrl = r.filePaths[0];
                iptvChannelsCache = null; lastPlaylistUpdate = 0;
                saveConfig();
            }
            res.json(iptvSettings);
        } catch (e) { res.status(500).json({ error: e.message }); }
    });

    // API: Pick a local XMLTV EPG file via native dialog
    expressApp.post('/api/iptv/pick-epg', async (req, res) => {
        try {
            const r = await dialog.showOpenDialog(mainWindow, {
                title: 'Select XMLTV EPG',
                properties: ['openFile'],
                filters: [{ name: 'XMLTV', extensions: ['xml', 'xmltv', 'gz'] }, { name: 'All files', extensions: ['*'] }],
            });
            if (!r.canceled && r.filePaths[0]) {
                iptvSettings.epgUrl = r.filePaths[0];
                iptvEpgCache = null; lastEpgUpdate = 0;
                saveConfig();
            }
            res.json(iptvSettings);
        } catch (e) { res.status(500).json({ error: e.message }); }
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

    // ==================== Remote Server API Endpoints ====================

    // API: Get all remote servers
    expressApp.get('/api/remote-servers', (req, res) => {
        res.json(remoteServers);
    });

    // API: Add/update remote server
    expressApp.post('/api/remote-servers', async (req, res) => {
        const { url, name, enabled = true } = req.body;

        if (!url) {
            return res.status(400).json({ error: 'Server URL is required' });
        }

        // Normalize URL (remove trailing slash)
        const normalizedUrl = url.replace(/\/+$/, '');

        // Check if server already exists
        const existingIndex = remoteServers.findIndex(s => s.url === normalizedUrl);

        const serverConfig = {
            url: normalizedUrl,
            name: name || new URL(normalizedUrl).hostname,
            enabled
        };

        // Verify server is reachable
        const status = await checkRemoteServer(normalizedUrl);
        serverConfig.status = status;

        if (existingIndex >= 0) {
            remoteServers[existingIndex] = serverConfig;
        } else {
            remoteServers.push(serverConfig);
        }

        saveConfig();
        res.json({ success: true, server: serverConfig, servers: remoteServers });
    });

    // API: Remove remote server
    expressApp.delete('/api/remote-servers', (req, res) => {
        const { url } = req.body;
        const normalizedUrl = url.replace(/\/+$/, '');
        remoteServers = remoteServers.filter(s => s.url !== normalizedUrl);
        saveConfig();
        res.json({ success: true, servers: remoteServers });
    });

    // API: Check remote server status
    expressApp.get('/api/remote-servers/status', async (req, res) => {
        const results = await Promise.all(
            remoteServers.map(async (server) => {
                const status = await checkRemoteServer(server.url);
                return { ...server, status };
            })
        );
        res.json(results);
    });

    // API: Get combined libraries (local + all enabled remote servers)
    expressApp.get('/api/all-libraries', async (req, res) => {
        try {
            // Local libraries
            const localLibraries = mediaPaths.map(p => ({
                path: p,
                name: path.basename(p),
                source: 'local'
            })).filter(lib => {
                try {
                    return fs.existsSync(lib.path) && fs.statSync(lib.path).isDirectory();
                } catch {
                    return false;
                }
            });

            // Remote libraries from all enabled servers
            const enabledServers = remoteServers.filter(s => s.enabled);
            const remoteLibrariesArrays = await Promise.all(
                enabledServers.map(server => fetchRemoteLibraries(server.url))
            );
            const remoteLibraries = remoteLibrariesArrays.flat();

            res.json({
                local: localLibraries,
                remote: remoteLibraries,
                combined: [...localLibraries, ...remoteLibraries]
            });
        } catch (error) {
            res.status(500).json({ error: error.message });
        }
    });

    // API: Proxy browse request to remote server
    expressApp.get('/api/remote/browse', async (req, res) => {
        const { serverUrl, path: remotePath } = req.query;

        if (!serverUrl || !remotePath) {
            return res.status(400).json({ error: 'serverUrl and path are required' });
        }

        try {
            const response = await remoteServerRequest(
                serverUrl,
                `/api/browse?path=${encodeURIComponent(remotePath)}`
            );

            if (response.status === 200) {
                // Tag items with source info
                const data = response.data;
                if (data.items) {
                    data.items = data.items.map(item => ({
                        ...item,
                        source: 'remote',
                        serverUrl
                    }));
                }
                data.source = 'remote';
                data.serverUrl = serverUrl;
                res.json(data);
            } else {
                res.status(response.status).json(response.data);
            }
        } catch (error) {
            res.status(500).json({ error: error.message });
        }
    });

    // API: Proxy video stream from remote server
    expressApp.get('/api/remote/video', (req, res) => {
        const { serverUrl, path: remotePath } = req.query;

        if (!serverUrl || !remotePath) {
            return res.status(400).send('serverUrl and path are required');
        }

        const url = new URL(`/api/video?path=${encodeURIComponent(remotePath)}`, serverUrl);
        const client = url.protocol === 'https:' ? https : http;

        const proxyReq = client.get(url.href, {
            headers: {
                'User-Agent': 'Nedflix-Desktop/1.0',
                ...(req.headers.range ? { Range: req.headers.range } : {})
            }
        }, (proxyRes) => {
            // Copy response headers
            res.writeHead(proxyRes.statusCode, proxyRes.headers);
            proxyRes.pipe(res);
        });

        proxyReq.on('error', (err) => {
            console.error('Remote video proxy error:', err.message);
            res.status(500).send('Failed to proxy video stream');
        });

        req.on('close', () => {
            proxyReq.destroy();
        });
    });

    // API: Proxy audio stream from remote server
    expressApp.get('/api/remote/audio', (req, res) => {
        const { serverUrl, path: remotePath } = req.query;

        if (!serverUrl || !remotePath) {
            return res.status(400).send('serverUrl and path are required');
        }

        const url = new URL(`/api/audio?path=${encodeURIComponent(remotePath)}`, serverUrl);
        const client = url.protocol === 'https:' ? https : http;

        const proxyReq = client.get(url.href, {
            headers: {
                'User-Agent': 'Nedflix-Desktop/1.0'
            }
        }, (proxyRes) => {
            res.writeHead(proxyRes.statusCode, proxyRes.headers);
            proxyRes.pipe(res);
        });

        proxyReq.on('error', (err) => {
            console.error('Remote audio proxy error:', err.message);
            res.status(500).send('Failed to proxy audio stream');
        });

        req.on('close', () => {
            proxyReq.destroy();
        });
    });

    // Marquee SPA endpoints (library titles, music, audiobooks, profiles, resume, favorites)
    // backed by the real local filesystem under mediaPaths. Must precede the SPA catch-all.
    registerMarqueeDesktop(expressApp, {
        getMediaPaths: () => mediaPaths,
        dataDir: app.getPath('userData'),
    });

    // Serve index for SPA
    expressApp.get('*', (req, res) => {
        res.sendFile(path.join(UI_DIR, 'index.html'));
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
        frame: false,
        backgroundColor: '#0b0c11',
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

ipcMain.handle('get-remote-servers', () => {
    return remoteServers;
});

ipcMain.handle('set-remote-servers', (event, servers) => {
    remoteServers = servers || [];
    saveConfig();
    return { success: true, servers: remoteServers };
});
