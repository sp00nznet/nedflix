require('dotenv').config();

const express = require('express');
const https = require('https');
const http = require('http');
const fs = require('fs');
const path = require('path');
const session = require('express-session');
const passport = require('passport');
const GoogleStrategy = require('passport-google-oauth20').Strategy;
const GitHubStrategy = require('passport-github2').Strategy;
const { spawn, execSync } = require('child_process');

const app = express();

// Subtitle cache directory
const SUBTITLE_CACHE_DIR = path.join(__dirname, 'subtitle_cache');
if (!fs.existsSync(SUBTITLE_CACHE_DIR)) {
    fs.mkdirSync(SUBTITLE_CACHE_DIR, { recursive: true });
}

// OpenSubtitles API configuration
const OPENSUBTITLES_API_KEY = process.env.OPENSUBTITLES_API_KEY || '';
const OPENSUBTITLES_API_URL = 'https://api.opensubtitles.com/api/v1';

// ============================================
// SUBTITLE UTILITY FUNCTIONS
// ============================================

/**
 * Extract movie/show title and metadata from filename
 */
function extractTitleFromFilename(filename) {
    // Remove file extension
    let name = filename.replace(/\.(mp4|mkv|avi|webm|mov|m4v|ogg)$/i, '');

    // Common patterns to extract
    const patterns = {
        // TV Show: "Show Name S01E02" or "Show Name 1x02"
        tvShow: /^(.+?)[\.\s_-]+[Ss](\d{1,2})[Ee](\d{1,2})/,
        tvShowAlt: /^(.+?)[\.\s_-]+(\d{1,2})x(\d{1,2})/,
        // Movie with year: "Movie Name (2023)" or "Movie Name 2023"
        movieWithYear: /^(.+?)[\.\s_-]*[\(\[]?(\d{4})[\)\]]?/,
        // Clean up common tags
        cleanupTags: /[\.\s_-]*(720p|1080p|2160p|4k|bluray|brrip|webrip|hdtv|dvdrip|x264|x265|hevc|aac|ac3|dts|proper|repack|extended|unrated|directors\.?cut).*$/i
    };

    let title = name;
    let year = null;
    let season = null;
    let episode = null;
    let type = 'movie';

    // Try to match TV show pattern first
    let match = name.match(patterns.tvShow) || name.match(patterns.tvShowAlt);
    if (match) {
        title = match[1];
        season = parseInt(match[2], 10);
        episode = parseInt(match[3], 10);
        type = 'episode';
    }

    // Clean up the title - remove quality/codec tags
    title = title.replace(patterns.cleanupTags, '');

    // Try to extract year for movies
    if (type === 'movie') {
        match = title.match(patterns.movieWithYear);
        if (match) {
            title = match[1];
            year = parseInt(match[2], 10);
            // Validate year is reasonable (1900-2030)
            if (year < 1900 || year > 2030) {
                year = null;
            }
        }
    }

    // Clean up title: replace dots/underscores with spaces, trim
    title = title
        .replace(/[\._]/g, ' ')
        .replace(/\s+/g, ' ')
        .replace(/-+$/, '')
        .trim();

    return {
        title,
        year,
        season,
        episode,
        type,
        originalFilename: filename
    };
}

/**
 * Convert SRT subtitle format to WebVTT
 */
function convertSrtToVtt(srtContent) {
    // Start with WebVTT header
    let vtt = 'WEBVTT\n\n';

    // SRT uses comma for milliseconds, VTT uses period
    // SRT format: 00:00:20,000 --> 00:00:24,400
    // VTT format: 00:00:20.000 --> 00:00:24.400

    const lines = srtContent.split(/\r?\n/);
    let i = 0;

    while (i < lines.length) {
        const line = lines[i].trim();

        // Skip empty lines
        if (!line) {
            i++;
            continue;
        }

        // Skip subtitle number (just a number)
        if (/^\d+$/.test(line)) {
            i++;
            continue;
        }

        // Check for timestamp line
        const timestampMatch = line.match(/^(\d{2}:\d{2}:\d{2}[,\.]\d{3})\s*-->\s*(\d{2}:\d{2}:\d{2}[,\.]\d{3})(.*)$/);
        if (timestampMatch) {
            // Convert timestamps
            const startTime = timestampMatch[1].replace(',', '.');
            const endTime = timestampMatch[2].replace(',', '.');
            const extra = timestampMatch[3] || '';

            vtt += `${startTime} --> ${endTime}${extra}\n`;
            i++;

            // Collect text lines until empty line
            while (i < lines.length && lines[i].trim()) {
                vtt += lines[i] + '\n';
                i++;
            }
            vtt += '\n';
        } else {
            i++;
        }
    }

    return vtt;
}

/**
 * Make HTTP/HTTPS request (Promise-based)
 */
function makeRequest(url, options = {}) {
    return new Promise((resolve, reject) => {
        const urlObj = new URL(url);
        const protocol = urlObj.protocol === 'https:' ? https : http;

        const requestOptions = {
            hostname: urlObj.hostname,
            port: urlObj.port || (urlObj.protocol === 'https:' ? 443 : 80),
            path: urlObj.pathname + urlObj.search,
            method: options.method || 'GET',
            headers: {
                'User-Agent': 'Nedflix/1.0',
                ...options.headers
            }
        };

        const req = protocol.request(requestOptions, (res) => {
            let data = '';

            // Handle redirects
            if (res.statusCode >= 300 && res.statusCode < 400 && res.headers.location) {
                makeRequest(res.headers.location, options)
                    .then(resolve)
                    .catch(reject);
                return;
            }

            res.on('data', (chunk) => {
                data += chunk;
            });

            res.on('end', () => {
                resolve({
                    statusCode: res.statusCode,
                    headers: res.headers,
                    body: data
                });
            });
        });

        req.on('error', reject);
        req.setTimeout(10000, () => {
            req.destroy();
            reject(new Error('Request timeout'));
        });

        if (options.body) {
            req.write(options.body);
        }

        req.end();
    });
}

/**
 * Download file from URL
 */
function downloadFile(url) {
    return new Promise((resolve, reject) => {
        const urlObj = new URL(url);
        const protocol = urlObj.protocol === 'https:' ? https : http;

        const requestOptions = {
            hostname: urlObj.hostname,
            port: urlObj.port || (urlObj.protocol === 'https:' ? 443 : 80),
            path: urlObj.pathname + urlObj.search,
            method: 'GET',
            headers: {
                'User-Agent': 'Nedflix/1.0'
            }
        };

        const req = protocol.request(requestOptions, (res) => {
            // Handle redirects
            if (res.statusCode >= 300 && res.statusCode < 400 && res.headers.location) {
                downloadFile(res.headers.location)
                    .then(resolve)
                    .catch(reject);
                return;
            }

            const chunks = [];

            res.on('data', (chunk) => {
                chunks.push(chunk);
            });

            res.on('end', () => {
                resolve(Buffer.concat(chunks));
            });
        });

        req.on('error', reject);
        req.setTimeout(30000, () => {
            req.destroy();
            reject(new Error('Download timeout'));
        });

        req.end();
    });
}

/**
 * Search OpenSubtitles for subtitles
 */
async function searchOpenSubtitles(metadata, language = 'en') {
    if (!OPENSUBTITLES_API_KEY) {
        console.log('OpenSubtitles API key not configured');
        return [];
    }

    try {
        const params = new URLSearchParams();
        params.append('query', metadata.title);
        params.append('languages', language);

        if (metadata.type === 'episode' && metadata.season && metadata.episode) {
            params.append('season_number', metadata.season);
            params.append('episode_number', metadata.episode);
        }

        if (metadata.year) {
            params.append('year', metadata.year);
        }

        const response = await makeRequest(
            `${OPENSUBTITLES_API_URL}/subtitles?${params.toString()}`,
            {
                headers: {
                    'Api-Key': OPENSUBTITLES_API_KEY,
                    'Content-Type': 'application/json'
                }
            }
        );

        if (response.statusCode !== 200) {
            console.log(`OpenSubtitles API error: ${response.statusCode}`);
            return [];
        }

        const data = JSON.parse(response.body);

        if (!data.data || !Array.isArray(data.data)) {
            return [];
        }

        // Map and score results
        const results = data.data.map(item => {
            const attrs = item.attributes;
            return {
                id: item.id,
                fileId: attrs.files?.[0]?.file_id,
                filename: attrs.files?.[0]?.file_name || 'Unknown',
                language: attrs.language,
                downloadCount: attrs.download_count || 0,
                rating: attrs.ratings || 0,
                hearing_impaired: attrs.hearing_impaired || false,
                source: 'opensubtitles'
            };
        }).filter(r => r.fileId);

        return results;
    } catch (error) {
        console.error('OpenSubtitles search error:', error.message);
        return [];
    }
}

/**
 * Download subtitle from OpenSubtitles
 */
async function downloadFromOpenSubtitles(fileId) {
    if (!OPENSUBTITLES_API_KEY) {
        throw new Error('OpenSubtitles API key not configured');
    }

    try {
        // First, get the download link
        const response = await makeRequest(
            `${OPENSUBTITLES_API_URL}/download`,
            {
                method: 'POST',
                headers: {
                    'Api-Key': OPENSUBTITLES_API_KEY,
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({ file_id: fileId })
            }
        );

        if (response.statusCode !== 200) {
            throw new Error(`Failed to get download link: ${response.statusCode}`);
        }

        const data = JSON.parse(response.body);

        if (!data.link) {
            throw new Error('No download link received');
        }

        // Download the subtitle file
        const subtitleBuffer = await downloadFile(data.link);
        return subtitleBuffer.toString('utf-8');
    } catch (error) {
        console.error('OpenSubtitles download error:', error.message);
        throw error;
    }
}

/**
 * Select the best subtitle from a list of results
 */
function selectBestSubtitle(subtitles, metadata) {
    if (!subtitles || subtitles.length === 0) {
        return null;
    }

    // Score each subtitle
    const scored = subtitles.map(sub => {
        let score = 0;

        // Prefer non-hearing impaired versions
        if (!sub.hearing_impaired) {
            score += 10;
        }

        // Prefer higher download counts (popularity)
        score += Math.min(sub.downloadCount / 1000, 20);

        // Prefer higher ratings
        score += (sub.rating || 0) * 2;

        // Check if filename contains the original filename parts
        const originalParts = metadata.title.toLowerCase().split(/\s+/);
        const subFilename = (sub.filename || '').toLowerCase();
        const matchingParts = originalParts.filter(part =>
            part.length > 2 && subFilename.includes(part)
        );
        score += matchingParts.length * 5;

        return { ...sub, score };
    });

    // Sort by score descending
    scored.sort((a, b) => b.score - a.score);

    return scored[0];
}

/**
 * Get cache file path for a video
 */
function getSubtitleCachePath(videoPath, language = 'en') {
    const hash = Buffer.from(videoPath).toString('base64').replace(/[/+=]/g, '_');
    return path.join(SUBTITLE_CACHE_DIR, `${hash}_${language}.vtt`);
}

// ============================================
// AUDIO TRACK UTILITY FUNCTIONS
// ============================================

// Check if FFprobe is available
let ffprobeAvailable = false;
let ffmpegAvailable = false;

try {
    execSync('ffprobe -version', { stdio: 'ignore' });
    ffprobeAvailable = true;
} catch {
    console.log('⚠️  FFprobe not found. Audio track detection will be disabled.');
}

try {
    execSync('ffmpeg -version', { stdio: 'ignore' });
    ffmpegAvailable = true;
} catch {
    console.log('⚠️  FFmpeg not found. Audio track switching will be disabled.');
}

// Language code to name mapping
const languageNames = {
    'eng': 'English',
    'en': 'English',
    'fra': 'French',
    'fre': 'French',
    'fr': 'French',
    'deu': 'German',
    'ger': 'German',
    'de': 'German',
    'spa': 'Spanish',
    'es': 'Spanish',
    'ita': 'Italian',
    'it': 'Italian',
    'por': 'Portuguese',
    'pt': 'Portuguese',
    'rus': 'Russian',
    'ru': 'Russian',
    'jpn': 'Japanese',
    'ja': 'Japanese',
    'kor': 'Korean',
    'ko': 'Korean',
    'chi': 'Chinese',
    'zho': 'Chinese',
    'zh': 'Chinese',
    'ara': 'Arabic',
    'ar': 'Arabic',
    'hin': 'Hindi',
    'hi': 'Hindi',
    'pol': 'Polish',
    'pl': 'Polish',
    'nld': 'Dutch',
    'dut': 'Dutch',
    'nl': 'Dutch',
    'swe': 'Swedish',
    'sv': 'Swedish',
    'nor': 'Norwegian',
    'no': 'Norwegian',
    'dan': 'Danish',
    'da': 'Danish',
    'fin': 'Finnish',
    'fi': 'Finnish',
    'tur': 'Turkish',
    'tr': 'Turkish',
    'tha': 'Thai',
    'th': 'Thai',
    'vie': 'Vietnamese',
    'vi': 'Vietnamese',
    'und': 'Unknown',
    '': 'Unknown'
};

/**
 * Get human-readable language name from code
 */
function getLanguageName(code) {
    if (!code) return 'Unknown';
    const lowerCode = code.toLowerCase();
    return languageNames[lowerCode] || code;
}

/**
 * Get audio tracks from a video file using FFprobe
 */
function getAudioTracks(videoPath) {
    return new Promise((resolve, reject) => {
        if (!ffprobeAvailable) {
            return resolve([]);
        }

        const args = [
            '-v', 'quiet',
            '-print_format', 'json',
            '-show_streams',
            '-select_streams', 'a',
            videoPath
        ];

        const ffprobe = spawn('ffprobe', args);
        let stdout = '';
        let stderr = '';

        ffprobe.stdout.on('data', (data) => {
            stdout += data.toString();
        });

        ffprobe.stderr.on('data', (data) => {
            stderr += data.toString();
        });

        ffprobe.on('close', (code) => {
            if (code !== 0) {
                console.error('FFprobe error:', stderr);
                return resolve([]);
            }

            try {
                const data = JSON.parse(stdout);
                const streams = data.streams || [];

                const audioTracks = streams.map((stream, index) => {
                    const tags = stream.tags || {};
                    const language = tags.language || '';
                    const title = tags.title || '';

                    // Build a descriptive label
                    let label = getLanguageName(language);
                    if (title && title !== label) {
                        label += ` (${title})`;
                    }

                    // Add codec info
                    const codec = stream.codec_name || '';
                    const channels = stream.channels || 0;
                    let channelDesc = '';
                    if (channels === 1) channelDesc = 'Mono';
                    else if (channels === 2) channelDesc = 'Stereo';
                    else if (channels === 6) channelDesc = '5.1';
                    else if (channels === 8) channelDesc = '7.1';
                    else if (channels > 0) channelDesc = `${channels}ch`;

                    if (channelDesc) {
                        label += ` - ${channelDesc}`;
                    }

                    return {
                        index: stream.index,
                        streamIndex: index,
                        codec: codec,
                        language: language,
                        languageName: getLanguageName(language),
                        title: title,
                        channels: channels,
                        channelLayout: stream.channel_layout || '',
                        sampleRate: stream.sample_rate || '',
                        bitRate: stream.bit_rate || '',
                        label: label,
                        default: stream.disposition?.default === 1
                    };
                });

                resolve(audioTracks);
            } catch (error) {
                console.error('FFprobe parse error:', error);
                resolve([]);
            }
        });

        ffprobe.on('error', (error) => {
            console.error('FFprobe spawn error:', error);
            resolve([]);
        });
    });
}

/**
 * Get video metadata including duration
 */
function getVideoMetadata(videoPath) {
    return new Promise((resolve, reject) => {
        if (!ffprobeAvailable) {
            return resolve(null);
        }

        const args = [
            '-v', 'quiet',
            '-print_format', 'json',
            '-show_format',
            '-show_streams',
            '-select_streams', 'v:0',
            videoPath
        ];

        const ffprobe = spawn('ffprobe', args);
        let stdout = '';

        ffprobe.stdout.on('data', (data) => {
            stdout += data.toString();
        });

        ffprobe.on('close', (code) => {
            if (code !== 0) {
                return resolve(null);
            }

            try {
                const data = JSON.parse(stdout);
                const format = data.format || {};
                const videoStream = (data.streams || [])[0] || {};

                resolve({
                    duration: parseFloat(format.duration) || 0,
                    bitRate: parseInt(format.bit_rate) || 0,
                    width: videoStream.width || 0,
                    height: videoStream.height || 0,
                    codec: videoStream.codec_name || ''
                });
            } catch (error) {
                resolve(null);
            }
        });

        ffprobe.on('error', () => {
            resolve(null);
        });
    });
}

// Cache for audio track info (to avoid repeated ffprobe calls)
const audioTrackCache = new Map();
const AUDIO_CACHE_TTL = 3600000; // 1 hour

/**
 * Get audio tracks with caching
 */
async function getAudioTracksWithCache(videoPath) {
    const cached = audioTrackCache.get(videoPath);
    if (cached && Date.now() - cached.timestamp < AUDIO_CACHE_TTL) {
        return cached.tracks;
    }

    const tracks = await getAudioTracks(videoPath);
    audioTrackCache.set(videoPath, {
        tracks,
        timestamp: Date.now()
    });

    return tracks;
}

// Configuration
const PORT = process.env.PORT || 3000;
const HTTPS_PORT = process.env.HTTPS_PORT || 3443;
const NFS_MOUNT_PATH = process.env.NFS_MOUNT_PATH || '/mnt/nfs';

// Local admin credentials
const ADMIN_USERNAME = process.env.ADMIN_USERNAME || '';
const ADMIN_PASSWORD = process.env.ADMIN_PASSWORD || '';

// Default profile pictures
const DEFAULT_AVATARS = ['cat', 'dog', 'cow', 'fox', 'owl', 'bear', 'rabbit', 'penguin'];

// In-memory user store (use a database in production)
const users = new Map();
const userSettings = new Map();

// Middleware
app.use(express.json());
app.use(express.static('public'));

// Session configuration
app.use(session({
    secret: process.env.SESSION_SECRET || 'change-this-secret-in-production',
    resave: false,
    saveUninitialized: false,
    cookie: {
        secure: true,
        httpOnly: true,
        maxAge: 24 * 60 * 60 * 1000 // 24 hours
    }
}));

// Initialize Passport
app.use(passport.initialize());
app.use(passport.session());

// Passport serialization
passport.serializeUser((user, done) => {
    done(null, user.id);
});

passport.deserializeUser((id, done) => {
    const user = users.get(id);
    done(null, user || null);
});

// Google OAuth Strategy
if (process.env.GOOGLE_CLIENT_ID && process.env.GOOGLE_CLIENT_SECRET) {
    passport.use(new GoogleStrategy({
        clientID: process.env.GOOGLE_CLIENT_ID,
        clientSecret: process.env.GOOGLE_CLIENT_SECRET,
        callbackURL: `${process.env.CALLBACK_BASE_URL || 'https://localhost:' + HTTPS_PORT}/auth/google/callback`
    }, (accessToken, refreshToken, profile, done) => {
        let user = users.get(profile.id);
        if (!user) {
            user = {
                id: profile.id,
                provider: 'google',
                displayName: profile.displayName,
                email: profile.emails?.[0]?.value,
                avatar: profile.photos?.[0]?.value || 'cat'
            };
            users.set(profile.id, user);
            userSettings.set(profile.id, getDefaultSettings());
        }
        return done(null, user);
    }));
}

// GitHub OAuth Strategy
if (process.env.GITHUB_CLIENT_ID && process.env.GITHUB_CLIENT_SECRET) {
    passport.use(new GitHubStrategy({
        clientID: process.env.GITHUB_CLIENT_ID,
        clientSecret: process.env.GITHUB_CLIENT_SECRET,
        callbackURL: `${process.env.CALLBACK_BASE_URL || 'https://localhost:' + HTTPS_PORT}/auth/github/callback`
    }, (accessToken, refreshToken, profile, done) => {
        let user = users.get(profile.id);
        if (!user) {
            user = {
                id: profile.id,
                provider: 'github',
                displayName: profile.displayName || profile.username,
                email: profile.emails?.[0]?.value,
                avatar: profile.photos?.[0]?.value || 'dog'
            };
            users.set(profile.id, user);
            userSettings.set(profile.id, getDefaultSettings());
        }
        return done(null, user);
    }));
}

// Default user settings
function getDefaultSettings() {
    return {
        streaming: {
            quality: 'auto',
            autoplay: false,
            volume: 80,
            playbackSpeed: 1.0,
            subtitles: false
        },
        profilePicture: 'cat'
    };
}

// Authentication middleware
function ensureAuthenticated(req, res, next) {
    if (req.isAuthenticated()) {
        return next();
    }
    res.status(401).json({ error: 'Not authenticated' });
}

// Auth routes
app.get('/auth/google', passport.authenticate('google', {
    scope: ['profile', 'email']
}));

app.get('/auth/google/callback',
    passport.authenticate('google', { failureRedirect: '/login.html' }),
    (req, res) => {
        res.redirect('/');
    }
);

app.get('/auth/github', passport.authenticate('github', {
    scope: ['user:email']
}));

app.get('/auth/github/callback',
    passport.authenticate('github', { failureRedirect: '/login.html' }),
    (req, res) => {
        res.redirect('/');
    }
);

app.get('/auth/logout', (req, res) => {
    req.logout((err) => {
        if (err) {
            return res.status(500).json({ error: 'Logout failed' });
        }
        res.redirect('/login.html');
    });
});

// Local admin authentication
app.post('/auth/local', express.urlencoded({ extended: true }), (req, res) => {
    const { username, password } = req.body;

    // Check if local admin is configured
    if (!ADMIN_USERNAME || !ADMIN_PASSWORD) {
        return res.status(401).json({ error: 'Local admin not configured' });
    }

    // Validate credentials
    if (username === ADMIN_USERNAME && password === ADMIN_PASSWORD) {
        // Create admin user if not exists
        const adminId = 'local-admin';
        let adminUser = users.get(adminId);

        if (!adminUser) {
            adminUser = {
                id: adminId,
                provider: 'local',
                displayName: 'Admin',
                email: null,
                avatar: 'bear',
                isAdmin: true
            };
            users.set(adminId, adminUser);
            userSettings.set(adminId, getDefaultSettings());
        }

        // Log in the user
        req.login(adminUser, (err) => {
            if (err) {
                return res.status(500).json({ error: 'Login failed' });
            }
            res.redirect('/');
        });
    } else {
        res.redirect('/login.html?error=invalid');
    }
});

// API: Get current user
app.get('/api/user', (req, res) => {
    if (req.isAuthenticated()) {
        const settings = userSettings.get(req.user.id) || getDefaultSettings();
        res.json({
            authenticated: true,
            user: {
                ...req.user,
                profilePicture: settings.profilePicture
            },
            settings: settings
        });
    } else {
        res.json({ authenticated: false });
    }
});

// API: Get available avatars
app.get('/api/avatars', (req, res) => {
    res.json(DEFAULT_AVATARS);
});

// API: Update user settings
app.post('/api/settings', ensureAuthenticated, (req, res) => {
    const { streaming, profilePicture } = req.body;
    let settings = userSettings.get(req.user.id) || getDefaultSettings();
    
    if (streaming) {
        settings.streaming = { ...settings.streaming, ...streaming };
    }
    
    if (profilePicture && DEFAULT_AVATARS.includes(profilePicture)) {
        settings.profilePicture = profilePicture;
    }
    
    userSettings.set(req.user.id, settings);
    res.json({ success: true, settings });
});

// API: Browse directories (protected)
app.get('/api/browse', ensureAuthenticated, (req, res) => {
    const requestedPath = req.query.path || NFS_MOUNT_PATH;
    
    // Security: Ensure path is within NFS mount
    const normalizedPath = path.normalize(requestedPath);
    if (!normalizedPath.startsWith(NFS_MOUNT_PATH)) {
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
            
            return {
                name: item.name,
                path: fullPath,
                isDirectory: item.isDirectory(),
                isVideo: !item.isDirectory() && /\.(mp4|webm|ogg|avi|mkv|mov|m4v)$/i.test(item.name),
                size: stats.size
            };
        }).filter(Boolean);
        
        fileList.sort((a, b) => {
            if (a.isDirectory && !b.isDirectory) return -1;
            if (!a.isDirectory && b.isDirectory) return 1;
            return a.name.localeCompare(b.name);
        });
        
        // Calculate parent path (but don't go above mount point)
        let parentPath = path.dirname(normalizedPath);
        if (!parentPath.startsWith(NFS_MOUNT_PATH)) {
            parentPath = NFS_MOUNT_PATH;
        }
        
        res.json({
            currentPath: normalizedPath,
            parentPath: parentPath,
            canGoUp: normalizedPath !== NFS_MOUNT_PATH,
            items: fileList
        });
    } catch (error) {
        res.status(500).json({ error: error.message });
    }
});

// API: Stream video (protected)
app.get('/api/video', ensureAuthenticated, (req, res) => {
    const videoPath = req.query.path;
    
    if (!videoPath) {
        return res.status(400).send('Video path is required');
    }
    
    // Security: Ensure path is within NFS mount
    const normalizedPath = path.normalize(videoPath);
    if (!normalizedPath.startsWith(NFS_MOUNT_PATH)) {
        return res.status(403).json({ error: 'Access denied' });
    }
    
    try {
        const stat = fs.statSync(normalizedPath);
        const fileSize = stat.size;
        const range = req.headers.range;
        
        // Get user's quality settings
        const settings = userSettings.get(req.user.id) || getDefaultSettings();
        
        // Determine content type
        const ext = path.extname(normalizedPath).toLowerCase();
        const contentTypes = {
            '.mp4': 'video/mp4',
            '.webm': 'video/webm',
            '.ogg': 'video/ogg',
            '.m4v': 'video/mp4',
            '.mkv': 'video/x-matroska',
            '.avi': 'video/x-msvideo',
            '.mov': 'video/quicktime'
        };
        const contentType = contentTypes[ext] || 'video/mp4';
        
        if (range) {
            const parts = range.replace(/bytes=/, "").split("-");
            const start = parseInt(parts[0], 10);
            const end = parts[1] ? parseInt(parts[1], 10) : fileSize - 1;
            const chunksize = (end - start) + 1;
            const file = fs.createReadStream(normalizedPath, { start, end });
            
            const head = {
                'Content-Range': `bytes ${start}-${end}/${fileSize}`,
                'Accept-Ranges': 'bytes',
                'Content-Length': chunksize,
                'Content-Type': contentType,
            };
            
            res.writeHead(206, head);
            file.pipe(res);
        } else {
            const head = {
                'Content-Length': fileSize,
                'Content-Type': contentType,
            };
            
            res.writeHead(200, head);
            fs.createReadStream(normalizedPath).pipe(res);
        }
    } catch (error) {
        res.status(500).json({ error: error.message });
    }
});

// ============================================
// SUBTITLE API ENDPOINTS
// ============================================

// API: Search and fetch subtitles for a video (protected)
app.get('/api/subtitles/search', ensureAuthenticated, async (req, res) => {
    const videoPath = req.query.path;
    const language = req.query.language || 'en';

    if (!videoPath) {
        return res.status(400).json({ error: 'Video path is required' });
    }

    // Security: Ensure path is within NFS mount
    const normalizedPath = path.normalize(videoPath);
    if (!normalizedPath.startsWith(NFS_MOUNT_PATH)) {
        return res.status(403).json({ error: 'Access denied' });
    }

    // Check for cached subtitle first
    const cachePath = getSubtitleCachePath(videoPath, language);
    if (fs.existsSync(cachePath)) {
        return res.json({
            found: true,
            cached: true,
            subtitleUrl: `/api/subtitles/serve?path=${encodeURIComponent(videoPath)}&language=${language}`
        });
    }

    // Extract metadata from filename
    const filename = path.basename(videoPath);
    const metadata = extractTitleFromFilename(filename);

    console.log(`Searching subtitles for: "${metadata.title}" (${metadata.type})`);

    try {
        // Search OpenSubtitles
        const results = await searchOpenSubtitles(metadata, language);

        if (results.length === 0) {
            return res.json({
                found: false,
                message: 'No subtitles found',
                searchedTitle: metadata.title
            });
        }

        // Select best subtitle
        const best = selectBestSubtitle(results, metadata);

        if (!best) {
            return res.json({
                found: false,
                message: 'Could not select a suitable subtitle',
                searchedTitle: metadata.title
            });
        }

        console.log(`Selected subtitle: ${best.filename} (score: ${best.score})`);

        // Download and cache the subtitle
        let srtContent;
        try {
            srtContent = await downloadFromOpenSubtitles(best.fileId);
        } catch (downloadError) {
            return res.json({
                found: false,
                message: 'Failed to download subtitle',
                error: downloadError.message
            });
        }

        // Convert to VTT format
        const vttContent = convertSrtToVtt(srtContent);

        // Cache the VTT file
        fs.writeFileSync(cachePath, vttContent, 'utf-8');

        res.json({
            found: true,
            cached: false,
            subtitleUrl: `/api/subtitles/serve?path=${encodeURIComponent(videoPath)}&language=${language}`,
            metadata: {
                filename: best.filename,
                language: best.language,
                source: best.source
            }
        });
    } catch (error) {
        console.error('Subtitle search error:', error);
        res.status(500).json({
            found: false,
            error: error.message
        });
    }
});

// API: Serve cached subtitle file (protected)
app.get('/api/subtitles/serve', ensureAuthenticated, (req, res) => {
    const videoPath = req.query.path;
    const language = req.query.language || 'en';

    if (!videoPath) {
        return res.status(400).send('Video path is required');
    }

    // Security: Ensure path is within NFS mount
    const normalizedPath = path.normalize(videoPath);
    if (!normalizedPath.startsWith(NFS_MOUNT_PATH)) {
        return res.status(403).json({ error: 'Access denied' });
    }

    const cachePath = getSubtitleCachePath(videoPath, language);

    if (!fs.existsSync(cachePath)) {
        return res.status(404).json({ error: 'Subtitle not found' });
    }

    res.setHeader('Content-Type', 'text/vtt; charset=utf-8');
    res.setHeader('Cache-Control', 'public, max-age=86400');
    fs.createReadStream(cachePath).pipe(res);
});

// API: Check subtitle configuration status
app.get('/api/subtitles/status', ensureAuthenticated, (req, res) => {
    res.json({
        configured: !!OPENSUBTITLES_API_KEY,
        provider: 'OpenSubtitles',
        message: OPENSUBTITLES_API_KEY
            ? 'Subtitle search is enabled'
            : 'Set OPENSUBTITLES_API_KEY environment variable to enable automatic subtitle search'
    });
});

// ============================================
// AUDIO TRACK API ENDPOINTS
// ============================================

// API: Get audio tracks for a video (protected)
app.get('/api/audio-tracks', ensureAuthenticated, async (req, res) => {
    const videoPath = req.query.path;

    if (!videoPath) {
        return res.status(400).json({ error: 'Video path is required' });
    }

    // Security: Ensure path is within NFS mount
    const normalizedPath = path.normalize(videoPath);
    if (!normalizedPath.startsWith(NFS_MOUNT_PATH)) {
        return res.status(403).json({ error: 'Access denied' });
    }

    // Check if file exists
    if (!fs.existsSync(normalizedPath)) {
        return res.status(404).json({ error: 'Video file not found' });
    }

    try {
        const tracks = await getAudioTracksWithCache(normalizedPath);

        res.json({
            available: ffprobeAvailable,
            switchable: ffmpegAvailable,
            tracks: tracks,
            message: !ffprobeAvailable
                ? 'FFprobe not installed - audio track detection unavailable'
                : !ffmpegAvailable
                    ? 'FFmpeg not installed - audio track switching unavailable'
                    : tracks.length > 1
                        ? `${tracks.length} audio tracks available`
                        : tracks.length === 1
                            ? '1 audio track available'
                            : 'No audio tracks detected'
        });
    } catch (error) {
        console.error('Audio track detection error:', error);
        res.status(500).json({ error: error.message });
    }
});

// API: Check audio feature status
app.get('/api/audio-tracks/status', ensureAuthenticated, (req, res) => {
    res.json({
        ffprobeAvailable,
        ffmpegAvailable,
        canDetect: ffprobeAvailable,
        canSwitch: ffmpegAvailable,
        message: ffprobeAvailable && ffmpegAvailable
            ? 'Audio track switching is fully available'
            : !ffprobeAvailable
                ? 'Install FFprobe to enable audio track detection'
                : 'Install FFmpeg to enable audio track switching'
    });
});

// API: Stream video with specific audio track (protected)
app.get('/api/video-stream', ensureAuthenticated, (req, res) => {
    const videoPath = req.query.path;
    const audioTrack = parseInt(req.query.audio, 10);
    const startTime = parseFloat(req.query.start) || 0;

    if (!videoPath) {
        return res.status(400).send('Video path is required');
    }

    // Security: Ensure path is within NFS mount
    const normalizedPath = path.normalize(videoPath);
    if (!normalizedPath.startsWith(NFS_MOUNT_PATH)) {
        return res.status(403).json({ error: 'Access denied' });
    }

    // Check if file exists
    if (!fs.existsSync(normalizedPath)) {
        return res.status(404).json({ error: 'Video file not found' });
    }

    // If no audio track specified or FFmpeg not available, use regular streaming
    if (isNaN(audioTrack) || !ffmpegAvailable) {
        return streamVideoFile(req, res, normalizedPath);
    }

    // Use FFmpeg to transcode with specific audio track
    try {
        const args = [
            '-hide_banner',
            '-loglevel', 'error'
        ];

        // Add start time if seeking
        if (startTime > 0) {
            args.push('-ss', startTime.toString());
        }

        args.push(
            '-i', normalizedPath,
            '-map', '0:v:0',                    // First video stream
            '-map', `0:a:${audioTrack}`,        // Selected audio stream
            '-c:v', 'copy',                     // Copy video (no re-encode)
            '-c:a', 'aac',                      // Transcode audio to AAC for compatibility
            '-b:a', '192k',                     // Audio bitrate
            '-movflags', 'frag_keyframe+empty_moov+faststart',
            '-f', 'mp4',                        // Output format
            'pipe:1'                            // Output to stdout
        );

        const ffmpeg = spawn('ffmpeg', args);

        res.setHeader('Content-Type', 'video/mp4');
        res.setHeader('Accept-Ranges', 'none'); // Disable range requests for transcoded streams
        res.setHeader('Cache-Control', 'no-cache');

        ffmpeg.stdout.pipe(res);

        ffmpeg.stderr.on('data', (data) => {
            console.error('FFmpeg stderr:', data.toString());
        });

        ffmpeg.on('error', (error) => {
            console.error('FFmpeg error:', error);
            if (!res.headersSent) {
                res.status(500).json({ error: 'Transcoding failed' });
            }
        });

        ffmpeg.on('close', (code) => {
            if (code !== 0 && code !== 255) {
                console.error(`FFmpeg exited with code ${code}`);
            }
        });

        // Handle client disconnect
        req.on('close', () => {
            ffmpeg.kill('SIGTERM');
        });

    } catch (error) {
        console.error('Video stream error:', error);
        res.status(500).json({ error: error.message });
    }
});

/**
 * Stream video file directly (without transcoding)
 */
function streamVideoFile(req, res, filePath) {
    try {
        const stat = fs.statSync(filePath);
        const fileSize = stat.size;
        const range = req.headers.range;

        // Determine content type
        const ext = path.extname(filePath).toLowerCase();
        const contentTypes = {
            '.mp4': 'video/mp4',
            '.webm': 'video/webm',
            '.ogg': 'video/ogg',
            '.m4v': 'video/mp4',
            '.mkv': 'video/x-matroska',
            '.avi': 'video/x-msvideo',
            '.mov': 'video/quicktime'
        };
        const contentType = contentTypes[ext] || 'video/mp4';

        if (range) {
            const parts = range.replace(/bytes=/, "").split("-");
            const start = parseInt(parts[0], 10);
            const end = parts[1] ? parseInt(parts[1], 10) : fileSize - 1;
            const chunksize = (end - start) + 1;
            const file = fs.createReadStream(filePath, { start, end });

            const head = {
                'Content-Range': `bytes ${start}-${end}/${fileSize}`,
                'Accept-Ranges': 'bytes',
                'Content-Length': chunksize,
                'Content-Type': contentType,
            };

            res.writeHead(206, head);
            file.pipe(res);
        } else {
            const head = {
                'Content-Length': fileSize,
                'Content-Type': contentType,
            };

            res.writeHead(200, head);
            fs.createReadStream(filePath).pipe(res);
        }
    } catch (error) {
        res.status(500).json({ error: error.message });
    }
}

// Check for available auth providers
app.get('/api/auth-providers', (req, res) => {
    const providers = [];
    if (process.env.GOOGLE_CLIENT_ID) providers.push('google');
    if (process.env.GITHUB_CLIENT_ID) providers.push('github');
    if (ADMIN_USERNAME && ADMIN_PASSWORD) providers.push('local');
    res.json(providers);
});

// Redirect root to login if not authenticated
app.get('/', (req, res, next) => {
    if (!req.isAuthenticated()) {
        return res.redirect('/login.html');
    }
    next();
});

// Start servers
function startServer() {
    const certsPath = path.join(__dirname, 'certs');
    const keyPath = path.join(certsPath, 'server.key');
    const certPath = path.join(certsPath, 'server.cert');
    
    // Check for SSL certificates
    if (fs.existsSync(keyPath) && fs.existsSync(certPath)) {
        const httpsOptions = {
            key: fs.readFileSync(keyPath),
            cert: fs.readFileSync(certPath)
        };
        
        // HTTPS server
        https.createServer(httpsOptions, app).listen(HTTPS_PORT, () => {
            console.log(`🔒 HTTPS server running at https://localhost:${HTTPS_PORT}`);
        });
        
        // HTTP redirect server
        const redirectApp = express();
        redirectApp.all('*', (req, res) => {
            res.redirect(`https://${req.hostname}:${HTTPS_PORT}${req.url}`);
        });
        http.createServer(redirectApp).listen(PORT, () => {
            console.log(`🔀 HTTP redirect server running at http://localhost:${PORT}`);
        });
    } else {
        console.log('⚠️  SSL certificates not found. Running HTTP only (not recommended).');
        console.log('   Run "npm run generate-certs" to create self-signed certificates.');
        
        // HTTP only (not secure)
        app.listen(PORT, () => {
            console.log(`⚠️  HTTP server running at http://localhost:${PORT}`);
        });
    }
    
    console.log(`\n📁 NFS mount path: ${NFS_MOUNT_PATH}`);
    
    // Check OAuth configuration
    const providers = [];
    if (process.env.GOOGLE_CLIENT_ID) providers.push('Google');
    if (process.env.GITHUB_CLIENT_ID) providers.push('GitHub');
    
    if (providers.length > 0) {
        console.log(`🔐 OAuth providers enabled: ${providers.join(', ')}`);
    } else {
        console.log('⚠️  No OAuth providers configured. Copy .env.example to .env and configure.');
    }
}

startServer();
