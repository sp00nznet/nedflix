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
const db = require('./db');
const metadataService = require('./metadata-service');
const userService = require('./user-service');
const mediaService = require('./media-service');

const app = express();

// Subtitle cache directory
const SUBTITLE_CACHE_DIR = path.join(__dirname, 'subtitle_cache');
if (!fs.existsSync(SUBTITLE_CACHE_DIR)) {
    fs.mkdirSync(SUBTITLE_CACHE_DIR, { recursive: true });
}

// OpenSubtitles API configuration
const OPENSUBTITLES_API_KEY = process.env.OPENSUBTITLES_API_KEY || '';
const OPENSUBTITLES_API_URL = 'https://api.opensubtitles.com/api/v1';

// OMDb API configuration (for movie/show metadata)
const OMDB_API_KEY = process.env.OMDB_API_KEY || '';

// Initialize metadata service
metadataService.init(OMDB_API_KEY);

// Services will be initialized after database connection
// See startServer() function

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

// Cache for subtitle track info
const subtitleTrackCache = new Map();
const SUBTITLE_CACHE_TTL = 3600000; // 1 hour

/**
 * Get embedded subtitle tracks from a video file
 */
function getSubtitleTracks(videoPath) {
    return new Promise((resolve, reject) => {
        if (!ffprobeAvailable) {
            return resolve([]);
        }

        const args = [
            '-v', 'quiet',
            '-print_format', 'json',
            '-show_streams',
            '-select_streams', 's',
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
                console.error('FFprobe subtitle error:', stderr);
                return resolve([]);
            }

            try {
                const data = JSON.parse(stdout);
                const streams = data.streams || [];

                const subtitleTracks = streams.map((stream, index) => {
                    const tags = stream.tags || {};
                    const language = tags.language || '';
                    const title = tags.title || '';

                    // Build a descriptive label
                    let label = getLanguageName(language) || 'Unknown';
                    if (title && title !== label) {
                        label += ` (${title})`;
                    }

                    // Add codec info for context
                    const codec = stream.codec_name || '';
                    if (codec === 'hdmv_pgs_subtitle' || codec === 'pgssub') {
                        label += ' [PGS]';
                    } else if (codec === 'dvd_subtitle' || codec === 'dvdsub') {
                        label += ' [DVD]';
                    } else if (codec === 'ass' || codec === 'ssa') {
                        label += ' [ASS]';
                    }

                    // Check if this is a text-based subtitle (extractable)
                    const textBasedCodecs = ['subrip', 'srt', 'ass', 'ssa', 'webvtt', 'mov_text'];
                    const isTextBased = textBasedCodecs.includes(codec);

                    return {
                        index: stream.index,
                        streamIndex: index,
                        codec: codec,
                        language: language,
                        languageName: getLanguageName(language),
                        title: title,
                        label: label,
                        default: stream.disposition?.default === 1,
                        forced: stream.disposition?.forced === 1,
                        isTextBased: isTextBased
                    };
                });

                resolve(subtitleTracks);
            } catch (error) {
                console.error('FFprobe subtitle parse error:', error);
                resolve([]);
            }
        });

        ffprobe.on('error', (error) => {
            console.error('FFprobe subtitle spawn error:', error);
            resolve([]);
        });
    });
}

/**
 * Get subtitle tracks with caching
 */
async function getSubtitleTracksWithCache(videoPath) {
    const cached = subtitleTrackCache.get(videoPath);
    if (cached && Date.now() - cached.timestamp < SUBTITLE_CACHE_TTL) {
        return cached.tracks;
    }

    const tracks = await getSubtitleTracks(videoPath);
    subtitleTrackCache.set(videoPath, {
        tracks,
        timestamp: Date.now()
    });

    return tracks;
}

/**
 * Extract subtitle track from video and convert to WebVTT
 */
function extractSubtitleTrack(videoPath, streamIndex) {
    return new Promise((resolve, reject) => {
        if (!ffmpegAvailable) {
            return reject(new Error('FFmpeg not available'));
        }

        const args = [
            '-i', videoPath,
            '-map', `0:s:${streamIndex}`,
            '-f', 'webvtt',
            '-'
        ];

        const ffmpeg = spawn('ffmpeg', args);
        let stdout = '';
        let stderr = '';

        ffmpeg.stdout.on('data', (data) => {
            stdout += data.toString();
        });

        ffmpeg.stderr.on('data', (data) => {
            stderr += data.toString();
        });

        ffmpeg.on('close', (code) => {
            if (code !== 0) {
                console.error('FFmpeg subtitle extraction error:', stderr);
                return reject(new Error('Failed to extract subtitle'));
            }
            resolve(stdout);
        });

        ffmpeg.on('error', (error) => {
            console.error('FFmpeg subtitle spawn error:', error);
            reject(error);
        });
    });
}

// Configuration
const PORT = process.env.PORT || 3000;
const HTTPS_PORT = process.env.HTTPS_PORT || 3443;
const NFS_MOUNT_PATH = process.env.NFS_MOUNT_PATH || '/mnt/nfs';

// Library paths for media indexing
const libraries = [
    { name: 'Movies', path: `${NFS_MOUNT_PATH}/Movies` },
    { name: 'TV Shows', path: `${NFS_MOUNT_PATH}/TV Shows` },
    { name: 'Music', path: `${NFS_MOUNT_PATH}/Music` },
    { name: 'Audiobooks', path: `${NFS_MOUNT_PATH}/Audiobooks` }
];

// Local admin credentials
const ADMIN_USERNAME = process.env.ADMIN_USERNAME || '';
const ADMIN_PASSWORD = process.env.ADMIN_PASSWORD || '';

// Default profile pictures
const DEFAULT_AVATARS = ['cat', 'dog', 'cow', 'fox', 'owl', 'bear', 'rabbit', 'penguin'];

// In-memory session cache (user data is now stored in database)
const sessionUsers = new Map();

// Middleware
app.use(express.json());
app.use(express.static('public'));

// Security headers
app.use((req, res, next) => {
    // HSTS - tell browsers to always use HTTPS
    res.setHeader('Strict-Transport-Security', 'max-age=31536000; includeSubDomains');
    // Prevent MIME type sniffing
    res.setHeader('X-Content-Type-Options', 'nosniff');
    // Clickjacking protection
    res.setHeader('X-Frame-Options', 'SAMEORIGIN');
    // XSS protection
    res.setHeader('X-XSS-Protection', '1; mode=block');
    // Referrer policy
    res.setHeader('Referrer-Policy', 'strict-origin-when-cross-origin');
    next();
});

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

passport.deserializeUser(async (id, done) => {
    // Try session cache first, then database
    let user = sessionUsers.get(id);
    if (!user) {
        try {
            const userData = await userService.getUser(id);
            if (userData) {
                user = userData.user;
                sessionUsers.set(id, user);
            }
        } catch (err) {
            return done(err, null);
        }
    }
    done(null, user || null);
});

// Google OAuth Strategy
if (process.env.GOOGLE_CLIENT_ID && process.env.GOOGLE_CLIENT_SECRET) {
    passport.use(new GoogleStrategy({
        clientID: process.env.GOOGLE_CLIENT_ID,
        clientSecret: process.env.GOOGLE_CLIENT_SECRET,
        callbackURL: `${process.env.CALLBACK_BASE_URL || 'https://localhost:' + HTTPS_PORT}/auth/google/callback`
    }, async (accessToken, refreshToken, profile, done) => {
        try {
            const user = await userService.upsertOAuthUser(profile, 'google');
            if (!user.isAllowed) {
                return done(null, false, { message: 'User not authorized' });
            }
            sessionUsers.set(user.id, user);
            return done(null, user);
        } catch (err) {
            return done(err, null);
        }
    }));
}

// GitHub OAuth Strategy
if (process.env.GITHUB_CLIENT_ID && process.env.GITHUB_CLIENT_SECRET) {
    passport.use(new GitHubStrategy({
        clientID: process.env.GITHUB_CLIENT_ID,
        clientSecret: process.env.GITHUB_CLIENT_SECRET,
        callbackURL: `${process.env.CALLBACK_BASE_URL || 'https://localhost:' + HTTPS_PORT}/auth/github/callback`
    }, async (accessToken, refreshToken, profile, done) => {
        try {
            const user = await userService.upsertOAuthUser(profile, 'github');
            if (!user.isAllowed) {
                return done(null, false, { message: 'User not authorized' });
            }
            sessionUsers.set(user.id, user);
            return done(null, user);
        } catch (err) {
            return done(err, null);
        }
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
            subtitles: false,
            audioLanguage: 'eng',
            subtitleLanguage: 'en'
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

// Admin middleware
function ensureAdmin(req, res, next) {
    if (req.isAuthenticated() && req.user?.isAdmin) {
        return next();
    }
    res.status(403).json({ error: 'Admin access required' });
}

// Auth routes
app.get('/auth/google', passport.authenticate('google', {
    scope: ['profile', 'email']
}));

app.get('/auth/google/callback',
    passport.authenticate('google', { failureRedirect: '/login.html?error=unauthorized' }),
    (req, res) => {
        res.redirect('/');
    }
);

app.get('/auth/github', passport.authenticate('github', {
    scope: ['user:email']
}));

app.get('/auth/github/callback',
    passport.authenticate('github', { failureRedirect: '/login.html?error=unauthorized' }),
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

// Local user authentication (admin and regular users)
app.post('/auth/local', express.urlencoded({ extended: true }), async (req, res) => {
    const { username, password } = req.body;

    if (!username || !password) {
        return res.redirect('/login.html?error=invalid');
    }

    // First check if this is the env-configured admin
    if (ADMIN_USERNAME && ADMIN_PASSWORD && username === ADMIN_USERNAME && password === ADMIN_PASSWORD) {
        const userData = await userService.getUser('local-admin');
        const adminUser = userData?.user || {
            id: 'local-admin',
            provider: 'local',
            displayName: 'Admin',
            avatar: 'bear',
            isAdmin: true,
            isAllowed: true
        };

        sessionUsers.set(adminUser.id, adminUser);

        return req.login(adminUser, (err) => {
            if (err) {
                return res.status(500).json({ error: 'Login failed' });
            }
            res.redirect('/');
        });
    }

    // Try to authenticate from database
    const user = await userService.authenticateUser(username, password);

    if (!user) {
        return res.redirect('/login.html?error=invalid');
    }

    if (!user.isAllowed) {
        return res.redirect('/login.html?error=disabled');
    }

    sessionUsers.set(user.id, user);

    req.login(user, (err) => {
        if (err) {
            return res.status(500).json({ error: 'Login failed' });
        }
        res.redirect('/');
    });
});

// API: Get current user
app.get('/api/user', async (req, res) => {
    if (req.isAuthenticated()) {
        const userData = await userService.getUser(req.user.id);
        const settings = userData?.settings || getDefaultSettings();
        const libraryAccess = userData?.user?.libraryAccess || ['movies', 'tv', 'music', 'audiobooks'];
        res.json({
            authenticated: true,
            user: {
                ...req.user,
                profilePicture: settings.profilePicture,
                libraryAccess: libraryAccess
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
app.post('/api/settings', ensureAuthenticated, async (req, res) => {
    const { streaming, profilePicture } = req.body;

    try {
        const updates = {};
        if (streaming) {
            updates.streaming = streaming;
        }
        if (profilePicture && DEFAULT_AVATARS.includes(profilePicture)) {
            updates.profilePicture = profilePicture;
        }

        const settings = await userService.updateSettings(req.user.id, updates);
        res.json({ success: true, settings });
    } catch (error) {
        res.status(500).json({ error: error.message });
    }
});

// API: Change own password (any authenticated user)
app.post('/api/user/change-password', ensureAuthenticated, async (req, res) => {
    const { currentPassword, newPassword } = req.body;

    if (!currentPassword || !newPassword) {
        return res.status(400).json({ error: 'Current and new passwords are required' });
    }

    if (newPassword.length < 4) {
        return res.status(400).json({ error: 'New password must be at least 4 characters' });
    }

    try {
        // Check if this is the env-configured admin
        if (req.user.id === 'local-admin') {
            // For env admin, verify current password matches env config
            if (currentPassword !== ADMIN_PASSWORD) {
                return res.status(401).json({ error: 'Current password is incorrect' });
            }
            // Cannot change env admin password through the UI
            return res.status(400).json({ error: 'Admin password must be changed in environment configuration' });
        }

        // For database users, verify and update password
        const result = await userService.changePassword(req.user.id, currentPassword, newPassword);

        if (!result.success) {
            return res.status(401).json({ error: result.error || 'Failed to change password' });
        }

        res.json({ success: true, message: 'Password changed successfully' });
    } catch (error) {
        res.status(500).json({ error: error.message });
    }
});

// API: Browse directories (protected)
app.get('/api/browse', ensureAuthenticated, async (req, res) => {
    const requestedPath = req.query.path || NFS_MOUNT_PATH;

    // Security: Ensure path is within NFS mount
    const normalizedPath = path.normalize(requestedPath);
    if (!normalizedPath.startsWith(NFS_MOUNT_PATH)) {
        return res.status(403).json({ error: 'Access denied' });
    }

    // Check library access permissions
    const userData = await userService.getUser(req.user.id);
    const libraryAccess = userData?.user?.libraryAccess || ['movies', 'tv', 'music', 'audiobooks'];

    // Map library paths to access keys
    const libraryPaths = {
        'Movies': 'movies',
        'TV Shows': 'tv',
        'Music': 'music',
        'Audiobooks': 'audiobooks'
    };

    // Check if accessing a restricted library
    const relativePath = normalizedPath.replace(NFS_MOUNT_PATH, '').replace(/^\//, '');
    const topLevelDir = relativePath.split('/')[0];

    if (topLevelDir && libraryPaths[topLevelDir]) {
        const requiredAccess = libraryPaths[topLevelDir];
        if (!libraryAccess.includes(requiredAccess)) {
            return res.status(403).json({ error: 'Access to this library is restricted' });
        }
    }

    try {
        // Try to use database index first for better performance
        let fileList = [];
        const hasIndexedData = await mediaService.hasIndexedChildren(normalizedPath);

        if (hasIndexedData) {
            // Use database index (fast, no disk access)
            fileList = await mediaService.browse(normalizedPath);
        } else {
            // Fall back to filesystem scan if path not indexed
            const items = fs.readdirSync(normalizedPath, { withFileTypes: true });

            fileList = items.map(item => {
                const fullPath = path.join(normalizedPath, item.name);
                let stats;
                try {
                    stats = fs.statSync(fullPath);
                } catch {
                    return null;
                }

                const isDir = item.isDirectory();
                const isVideo = !isDir && /\.(mp4|webm|ogg|avi|mkv|mov|m4v)$/i.test(item.name);
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

            // Sort filesystem results
            fileList.sort((a, b) => {
                if (a.isDirectory && !b.isDirectory) return -1;
                if (!a.isDirectory && b.isDirectory) return 1;
                return a.name.localeCompare(b.name);
            });
        }

        // Get cached metadata for all media files
        const mediaPaths = fileList.filter(f => f.isVideo || f.isAudio).map(f => f.path);
        const metadataMap = await metadataService.getCachedMetadataBulkAsync(mediaPaths);

        // Enrich items with metadata
        for (const item of fileList) {
            const meta = metadataMap[item.path];
            if (meta) {
                item.cleanTitle = meta.clean_title;
                item.year = meta.year;
                item.poster = meta.poster_path;
                item.rating = meta.rating;
                item.genre = meta.genre;
                item.plot = meta.plot;
                item.episodeTitle = meta.episode_title;
                item.season = meta.season;
                item.episode = meta.episode;
                item.type = meta.type;
                item.hasMetadata = true;
            } else {
                // Extract basic info from filename
                const extracted = metadataService.extractFromFilename(item.name);
                item.cleanTitle = extracted.title;
                item.year = extracted.year;
                item.season = extracted.season;
                item.episode = extracted.episode;
                item.type = extracted.type;
                item.hasMetadata = false;
            }
        }

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
app.get('/api/video', ensureAuthenticated, async (req, res) => {
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
        const userData = await userService.getUser(req.user.id);
        const settings = userData?.settings || getDefaultSettings();
        
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

// API: Stream video with transcoding fallback (for incompatible formats)
app.get('/api/video-transcode', ensureAuthenticated, async (req, res) => {
    const videoPath = req.query.path;
    const startTime = req.query.start || 0;

    if (!videoPath) {
        return res.status(400).send('Video path is required');
    }

    // Security: Ensure path is within NFS mount
    const normalizedPath = path.normalize(videoPath);
    if (!normalizedPath.startsWith(NFS_MOUNT_PATH)) {
        return res.status(403).json({ error: 'Access denied' });
    }

    // Check file exists
    if (!fs.existsSync(normalizedPath)) {
        return res.status(404).json({ error: 'File not found' });
    }

    // First, probe the file to determine the best transcoding strategy
    let videoCodec = null;
    let audioCodec = null;
    try {
        const probeResult = await new Promise((resolve, reject) => {
            const probe = spawn('ffprobe', [
                '-v', 'quiet',
                '-print_format', 'json',
                '-show_streams',
                normalizedPath
            ]);
            let output = '';
            probe.stdout.on('data', data => output += data);
            probe.on('close', code => {
                if (code === 0) {
                    try {
                        resolve(JSON.parse(output));
                    } catch (e) {
                        reject(e);
                    }
                } else {
                    reject(new Error('ffprobe failed'));
                }
            });
        });

        const videoStream = probeResult.streams?.find(s => s.codec_type === 'video');
        const audioStream = probeResult.streams?.find(s => s.codec_type === 'audio');
        videoCodec = videoStream?.codec_name;
        audioCodec = audioStream?.codec_name;
        console.log(`Transcoding: video=${videoCodec}, audio=${audioCodec}`);
    } catch (e) {
        console.log('Could not probe file, will attempt full transcode');
    }

    // Set headers for streaming
    res.setHeader('Content-Type', 'video/mp4');
    res.setHeader('Transfer-Encoding', 'chunked');

    // Build FFmpeg arguments based on what needs transcoding
    // If video is already H.264, just copy it (much faster)
    const videoArgs = (videoCodec === 'h264')
        ? ['-c:v', 'copy']
        : ['-c:v', 'libx264', '-preset', 'veryfast', '-crf', '23'];

    // If audio is already AAC, just copy it
    const audioArgs = (audioCodec === 'aac')
        ? ['-c:a', 'copy']
        : ['-c:a', 'aac', '-b:a', '192k', '-ac', '2'];

    const ffmpegArgs = [
        '-hide_banner',
        '-loglevel', 'error',
        '-ss', String(startTime),      // Input seeking (faster)
        '-i', normalizedPath,
        ...videoArgs,
        ...audioArgs,
        '-f', 'mp4',                   // MP4 container
        '-movflags', 'frag_keyframe+empty_moov+faststart',  // Enable streaming
        'pipe:1'                       // Output to stdout
    ];

    console.log('FFmpeg transcode args:', ffmpegArgs.join(' '));

    const ffmpeg = spawn('ffmpeg', ffmpegArgs);
    let hasData = false;

    // Pipe FFmpeg output to response
    ffmpeg.stdout.on('data', (chunk) => {
        hasData = true;
        res.write(chunk);
    });

    // Log errors
    ffmpeg.stderr.on('data', (data) => {
        console.error('FFmpeg stderr:', data.toString());
    });

    ffmpeg.on('error', (err) => {
        console.error('FFmpeg spawn error:', err);
        if (!res.headersSent) {
            res.status(500).json({ error: 'Transcoding failed' });
        }
    });

    ffmpeg.on('close', (code) => {
        if (code !== 0) {
            console.error(`FFmpeg exited with code ${code}`);
            if (!hasData && !res.headersSent) {
                res.status(500).json({ error: 'Transcoding failed' });
            }
        }
        res.end();
    });

    // Clean up on client disconnect
    req.on('close', () => {
        ffmpeg.kill('SIGTERM');
    });
});

// API: Stream audio (protected)
app.get('/api/audio', ensureAuthenticated, (req, res) => {
    const audioPath = req.query.path;

    if (!audioPath) {
        return res.status(400).send('Audio path is required');
    }

    // Security: Ensure path is within NFS mount
    const normalizedPath = path.normalize(audioPath);
    if (!normalizedPath.startsWith(NFS_MOUNT_PATH)) {
        return res.status(403).json({ error: 'Access denied' });
    }

    try {
        const stat = fs.statSync(normalizedPath);
        const fileSize = stat.size;
        const range = req.headers.range;

        // Determine content type
        const ext = path.extname(normalizedPath).toLowerCase();
        const contentTypes = {
            '.mp3': 'audio/mpeg',
            '.m4a': 'audio/mp4',
            '.flac': 'audio/flac',
            '.wav': 'audio/wav',
            '.aac': 'audio/aac',
            '.ogg': 'audio/ogg',
            '.wma': 'audio/x-ms-wma',
            '.opus': 'audio/opus',
            '.aiff': 'audio/aiff'
        };
        const contentType = contentTypes[ext] || 'audio/mpeg';

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
        const [tracks, metadata] = await Promise.all([
            getAudioTracksWithCache(normalizedPath),
            getVideoMetadata(normalizedPath)
        ]);

        res.json({
            available: ffprobeAvailable,
            switchable: ffmpegAvailable,
            tracks: tracks,
            duration: metadata?.duration || null,
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

// API: Get embedded subtitle tracks from a video (protected)
app.get('/api/embedded-subtitles', ensureAuthenticated, async (req, res) => {
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
        const tracks = await getSubtitleTracksWithCache(normalizedPath);

        res.json({
            available: ffprobeAvailable,
            extractable: ffmpegAvailable,
            tracks: tracks,
            message: !ffprobeAvailable
                ? 'FFprobe not installed - subtitle detection unavailable'
                : tracks.length > 0
                    ? `${tracks.length} subtitle track(s) found`
                    : 'No embedded subtitles found'
        });
    } catch (error) {
        console.error('Subtitle track detection error:', error);
        res.status(500).json({ error: error.message });
    }
});

// API: Extract and serve embedded subtitle track as WebVTT (protected)
app.get('/api/embedded-subtitles/extract', ensureAuthenticated, async (req, res) => {
    const videoPath = req.query.path;
    const trackIndex = parseInt(req.query.track, 10);

    if (!videoPath) {
        return res.status(400).json({ error: 'Video path is required' });
    }

    if (isNaN(trackIndex)) {
        return res.status(400).json({ error: 'Track index is required' });
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
        const vttContent = await extractSubtitleTrack(normalizedPath, trackIndex);
        res.set('Content-Type', 'text/vtt');
        res.send(vttContent);
    } catch (error) {
        console.error('Subtitle extraction error:', error);
        res.status(500).json({ error: 'Failed to extract subtitle' });
    }
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

// ============================================
// USER MANAGEMENT API ENDPOINTS (Admin only)
// ============================================

// API: Get all users (admin only)
app.get('/api/admin/users', ensureAdmin, async (req, res) => {
    try {
        const users = await userService.getAllUsers();
        res.json({ users });
    } catch (error) {
        res.status(500).json({ error: error.message });
    }
});

// API: Add a new user (admin only)
app.post('/api/admin/users', ensureAdmin, async (req, res) => {
    const { username, password, displayName, isAdmin } = req.body;

    if (!username) {
        return res.status(400).json({ error: 'Username is required' });
    }
    if (!password) {
        return res.status(400).json({ error: 'Password is required' });
    }

    try {
        const user = await userService.addUser(username, password, displayName, isAdmin);
        res.json({ success: true, user });
    } catch (error) {
        res.status(400).json({ error: error.message });
    }
});

// API: Update a user (admin only)
app.put('/api/admin/users/:id', ensureAdmin, async (req, res) => {
    const { id } = req.params;
    const { displayName, password, isAdmin, isAllowed, libraryAccess } = req.body;

    try {
        const result = await userService.updateUser(id, { displayName, password, isAdmin, isAllowed, libraryAccess });
        // Update session cache if user is currently logged in
        if (result?.user) {
            sessionUsers.set(id, result.user);
        }
        res.json({ success: true, user: result?.user });
    } catch (error) {
        res.status(400).json({ error: error.message });
    }
});

// API: Delete a user (admin only)
app.delete('/api/admin/users/:id', ensureAdmin, async (req, res) => {
    const { id } = req.params;

    try {
        await userService.deleteUser(id);
        sessionUsers.delete(id);
        res.json({ success: true });
    } catch (error) {
        res.status(400).json({ error: error.message });
    }
});

// ============================================
// MEDIA INDEX & SEARCH API ENDPOINTS
// ============================================

// API: Search files (authenticated users)
app.get('/api/search', ensureAuthenticated, async (req, res) => {
    const { q, type, library, limit } = req.query;

    if (!q || q.length < 2) {
        return res.status(400).json({ error: 'Search query must be at least 2 characters' });
    }

    try {
        // Get user's library access
        const userData = await userService.getUser(req.user.id);
        const userLibraryAccess = userData?.user?.libraryAccess || ['movies', 'tv', 'music', 'audiobooks'];

        // Search in index
        let results = await mediaService.search(q, {
            fileType: type,
            library: library,
            limit: parseInt(limit) || 100
        });

        // Filter results by user's library access
        results = results.filter(file => {
            const fileLibrary = file.library?.toLowerCase();
            return userLibraryAccess.some(lib =>
                fileLibrary?.includes(lib.toLowerCase())
            );
        });

        res.json({
            query: q,
            count: results.length,
            results: results
        });
    } catch (error) {
        res.status(500).json({ error: error.message });
    }
});

// API: Get media index stats
app.get('/api/search/stats', ensureAuthenticated, async (req, res) => {
    try {
        const stats = await mediaService.getIndexStats();
        res.json(stats);
    } catch (error) {
        res.status(500).json({ error: error.message });
    }
});

// API: Start media scan (admin only)
app.post('/api/admin/scan', ensureAdmin, async (req, res) => {
    try {
        const result = await mediaService.startScan(
            NFS_MOUNT_PATH,
            libraries,
            req.user.displayName || req.user.id
        );
        res.json(result);
    } catch (error) {
        res.status(500).json({ error: error.message });
    }
});

// API: Get scan status
app.get('/api/admin/scan/status', ensureAdmin, async (req, res) => {
    try {
        const status = await mediaService.getScanStatus();
        res.json(status || { status: 'no_scans' });
    } catch (error) {
        res.status(500).json({ error: error.message });
    }
});

// API: Get scan logs (admin only)
app.get('/api/admin/scan/logs', ensureAdmin, async (req, res) => {
    try {
        const limit = parseInt(req.query.limit) || 50;
        const logs = await mediaService.getScanLogs(limit);
        res.json(logs);
    } catch (error) {
        res.status(500).json({ error: error.message });
    }
});

// API: Download scan logs as CSV (admin only)
app.get('/api/admin/scan/logs/download', ensureAdmin, async (req, res) => {
    try {
        const logs = await mediaService.getScanLogs(1000);

        // Build CSV
        const headers = ['ID', 'Started At', 'Completed At', 'Status', 'Files Found', 'Files Indexed', 'Errors', 'Scan Path', 'Triggered By'];
        const rows = logs.map(log => [
            log.id,
            new Date(log.started_at * 1000).toISOString(),
            log.completed_at ? new Date(log.completed_at * 1000).toISOString() : '',
            log.status,
            log.files_found,
            log.files_indexed,
            log.errors,
            log.scan_path,
            log.triggered_by
        ]);

        const csv = [
            headers.join(','),
            ...rows.map(row => row.map(cell => `"${String(cell).replace(/"/g, '""')}"`).join(','))
        ].join('\n');

        res.setHeader('Content-Type', 'text/csv');
        res.setHeader('Content-Disposition', `attachment; filename="scan-logs-${Date.now()}.csv"`);
        res.send(csv);
    } catch (error) {
        res.status(500).json({ error: error.message });
    }
});

// ============================================
// METADATA API ENDPOINTS
// ============================================

// API: Get metadata status
app.get('/api/metadata/status', ensureAuthenticated, (req, res) => {
    res.json({
        configured: !!OMDB_API_KEY,
        provider: 'OMDb + TVmaze + Wikidata',
        message: OMDB_API_KEY
            ? 'Metadata fetching is enabled'
            : 'Set OMDB_API_KEY environment variable to enable automatic metadata fetching'
    });
});

// API: Start metadata scan for a directory (admin only)
app.post('/api/metadata/scan', ensureAdmin, async (req, res) => {
    const { path: dirPath } = req.body;

    if (!dirPath) {
        return res.status(400).json({ error: 'Directory path is required' });
    }

    // Security: Ensure path is within NFS mount
    const normalizedPath = path.normalize(dirPath);
    if (!normalizedPath.startsWith(NFS_MOUNT_PATH)) {
        return res.status(403).json({ error: 'Access denied' });
    }

    if (!OMDB_API_KEY) {
        return res.status(400).json({
            error: 'OMDb API key not configured',
            message: 'Set OMDB_API_KEY in your .env file'
        });
    }

    const result = await metadataService.startBackgroundScan(normalizedPath, OMDB_API_KEY);
    res.json(result);
});

// API: Get scan progress
app.get('/api/metadata/scan/progress', ensureAuthenticated, (req, res) => {
    res.json(metadataService.getScanProgress());
});

// API: Get metadata for a specific file
app.get('/api/metadata', ensureAuthenticated, async (req, res) => {
    const filePath = req.query.path;

    if (!filePath) {
        return res.status(400).json({ error: 'File path is required' });
    }

    // Security: Ensure path is within NFS mount
    const normalizedPath = path.normalize(filePath);
    if (!normalizedPath.startsWith(NFS_MOUNT_PATH)) {
        return res.status(403).json({ error: 'Access denied' });
    }

    // Check for cached metadata first
    let metadata = metadataService.getCachedMetadata(normalizedPath);

    // If not cached and API key available, fetch it
    if (!metadata && OMDB_API_KEY) {
        metadata = await metadataService.getMetadata(normalizedPath, OMDB_API_KEY);
    }

    if (metadata) {
        res.json({
            found: true,
            metadata: {
                cleanTitle: metadata.clean_title,
                year: metadata.year,
                type: metadata.type,
                poster: metadata.poster_path,
                plot: metadata.plot,
                rating: metadata.rating,
                genre: metadata.genre,
                director: metadata.director,
                actors: metadata.actors,
                runtime: metadata.runtime,
                imdbId: metadata.imdb_id,
                season: metadata.season,
                episode: metadata.episode,
                episodeTitle: metadata.episode_title,
                source: metadata.source
            }
        });
    } else {
        // Return extracted info from filename
        const extracted = metadataService.extractFromFilename(path.basename(normalizedPath));
        res.json({
            found: false,
            metadata: {
                cleanTitle: extracted.title,
                year: extracted.year,
                type: extracted.type,
                season: extracted.season,
                episode: extracted.episode,
                source: 'filename'
            }
        });
    }
});

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
async function startServer() {
    // Initialize database first
    try {
        await db.init();
        console.log('✅ Database initialized');
    } catch (error) {
        console.error('❌ Database initialization failed:', error.message);
        process.exit(1);
    }

    // Initialize services after database is ready
    await userService.init();
    await mediaService.init();

    const certsPath = path.join(__dirname, 'certs');
    const keyPath = path.join(certsPath, 'server.key');
    const certPath = path.join(certsPath, 'server.cert');

    // Also check for Let's Encrypt style filenames
    const leKeyPath = path.join(certsPath, 'privkey.pem');
    const leCertPath = path.join(certsPath, 'fullchain.pem');

    // Determine which certificate files to use
    let actualKeyPath = null;
    let actualCertPath = null;

    if (fs.existsSync(keyPath) && fs.existsSync(certPath)) {
        actualKeyPath = keyPath;
        actualCertPath = certPath;
    } else if (fs.existsSync(leKeyPath) && fs.existsSync(leCertPath)) {
        actualKeyPath = leKeyPath;
        actualCertPath = leCertPath;
        console.log('📜 Using Let\'s Encrypt certificate format');
    }

    // Check for SSL certificates
    if (actualKeyPath && actualCertPath) {
        const httpsOptions = {
            key: fs.readFileSync(actualKeyPath),
            cert: fs.readFileSync(actualCertPath)
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

    // Show database mode
    if (db.isUsingPostgres()) {
        console.log('🐘 Database: PostgreSQL (Docker)');
    } else {
        console.log('📦 Database: SQLite (local)');
    }

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
