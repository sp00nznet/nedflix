/**
 * Metadata Service for Nedflix
 * Fetches clean names and thumbnails from OMDb, TVmaze, and Wikidata
 */

const https = require('https');
const http = require('http');
const fs = require('fs');
const path = require('path');

// Rate limiting configuration
const RATE_LIMITS = {
    omdb: { requests: 1000, perMs: 86400000, delay: 250 },    // 1000/day, 250ms between
    tvmaze: { requests: 20, perMs: 10000, delay: 500 },        // 20/10s, 500ms between
    wikidata: { requests: 50, perMs: 60000, delay: 200 }       // 50/min, 200ms between
};

const requestCounts = {
    omdb: { count: 0, resetAt: Date.now() + RATE_LIMITS.omdb.perMs },
    tvmaze: { count: 0, resetAt: Date.now() + RATE_LIMITS.tvmaze.perMs },
    wikidata: { count: 0, resetAt: Date.now() + RATE_LIMITS.wikidata.perMs }
};

let lastRequestTime = {
    omdb: 0,
    tvmaze: 0,
    wikidata: 0
};

// Database setup
let db = null;
const DB_PATH = path.join(__dirname, 'metadata.db');
const THUMBNAIL_DIR = path.join(__dirname, 'public', 'thumbnails');

/**
 * Initialize the metadata service
 */
function init(omdbApiKey) {
    // Create thumbnail directory
    if (!fs.existsSync(THUMBNAIL_DIR)) {
        fs.mkdirSync(THUMBNAIL_DIR, { recursive: true });
    }

    // Initialize database
    try {
        const Database = require('better-sqlite3');
        db = new Database(DB_PATH);

        // Create tables
        db.exec(`
            CREATE TABLE IF NOT EXISTS media_metadata (
                file_path TEXT PRIMARY KEY,
                clean_title TEXT,
                year INTEGER,
                type TEXT,
                poster_path TEXT,
                plot TEXT,
                rating TEXT,
                genre TEXT,
                director TEXT,
                actors TEXT,
                runtime TEXT,
                imdb_id TEXT,
                tvmaze_id INTEGER,
                season INTEGER,
                episode INTEGER,
                episode_title TEXT,
                source TEXT,
                fetched_at INTEGER,
                updated_at INTEGER
            );

            CREATE INDEX IF NOT EXISTS idx_file_path ON media_metadata(file_path);
            CREATE INDEX IF NOT EXISTS idx_imdb_id ON media_metadata(imdb_id);
        `);

        console.log('📚 Metadata database initialized');
        return true;
    } catch (error) {
        console.error('Failed to initialize metadata database:', error.message);
        return false;
    }
}

/**
 * Wait for rate limit
 */
async function waitForRateLimit(api) {
    const limit = RATE_LIMITS[api];
    const state = requestCounts[api];

    // Reset counter if window expired
    if (Date.now() > state.resetAt) {
        state.count = 0;
        state.resetAt = Date.now() + limit.perMs;
    }

    // Check if we've hit the limit
    if (state.count >= limit.requests) {
        const waitTime = state.resetAt - Date.now();
        console.log(`Rate limit reached for ${api}, waiting ${Math.ceil(waitTime/1000)}s`);
        await sleep(waitTime);
        state.count = 0;
        state.resetAt = Date.now() + limit.perMs;
    }

    // Ensure minimum delay between requests
    const timeSinceLastRequest = Date.now() - lastRequestTime[api];
    if (timeSinceLastRequest < limit.delay) {
        await sleep(limit.delay - timeSinceLastRequest);
    }

    state.count++;
    lastRequestTime[api] = Date.now();
}

/**
 * Sleep helper
 */
function sleep(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}

/**
 * Make HTTP/HTTPS request
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
                'Accept': 'application/json',
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
        req.setTimeout(15000, () => {
            req.destroy();
            reject(new Error('Request timeout'));
        });

        req.end();
    });
}

/**
 * Download and save thumbnail
 */
async function downloadThumbnail(url, filename) {
    if (!url || url === 'N/A') return null;

    try {
        const response = await new Promise((resolve, reject) => {
            const urlObj = new URL(url);
            const protocol = urlObj.protocol === 'https:' ? https : http;

            const req = protocol.get(url, {
                headers: { 'User-Agent': 'Nedflix/1.0' }
            }, (res) => {
                if (res.statusCode >= 300 && res.statusCode < 400 && res.headers.location) {
                    downloadThumbnail(res.headers.location, filename)
                        .then(resolve)
                        .catch(reject);
                    return;
                }

                const chunks = [];
                res.on('data', chunk => chunks.push(chunk));
                res.on('end', () => resolve(Buffer.concat(chunks)));
                res.on('error', reject);
            });

            req.on('error', reject);
            req.setTimeout(15000, () => {
                req.destroy();
                reject(new Error('Download timeout'));
            });
        });

        const ext = path.extname(new URL(url).pathname) || '.jpg';
        const safeFilename = filename.replace(/[^a-zA-Z0-9]/g, '_') + ext;
        const filePath = path.join(THUMBNAIL_DIR, safeFilename);

        fs.writeFileSync(filePath, response);
        return `/thumbnails/${safeFilename}`;
    } catch (error) {
        console.error(`Failed to download thumbnail: ${error.message}`);
        return null;
    }
}

/**
 * Extract metadata from filename
 */
function extractFromFilename(filename) {
    // Remove extension
    let name = filename.replace(/\.(mp4|mkv|avi|webm|mov|m4v|ogg|mp3|m4a|flac|wav|aac)$/i, '');

    const patterns = {
        // TV Show: "Show Name S01E02" or "Show Name 1x02"
        tvShow: /^(.+?)[\.\s_-]+[Ss](\d{1,2})[Ee](\d{1,2})/,
        tvShowAlt: /^(.+?)[\.\s_-]+(\d{1,2})x(\d{1,2})/,
        // Movie with year: "Movie Name (2023)" or "Movie Name 2023"
        movieWithYear: /^(.+?)[\.\s_-]*[\(\[]?(\d{4})[\)\]]?/,
        // Common release tags to remove
        cleanupTags: /[\.\s_-]*(720p|1080p|2160p|4k|uhd|bluray|brrip|webrip|web-dl|hdtv|dvdrip|x264|x265|hevc|h\.?264|h\.?265|aac|ac3|dts|atmos|proper|repack|extended|unrated|directors\.?cut|remastered|remux|hdr|10bit).*$/i
    };

    let title = name;
    let year = null;
    let season = null;
    let episode = null;
    let type = 'movie';

    // Try TV show pattern first
    let match = name.match(patterns.tvShow) || name.match(patterns.tvShowAlt);
    if (match) {
        title = match[1];
        season = parseInt(match[2], 10);
        episode = parseInt(match[3], 10);
        type = 'series';
    }

    // Remove release tags
    title = title.replace(patterns.cleanupTags, '');

    // Extract year for movies
    if (type === 'movie') {
        match = title.match(patterns.movieWithYear);
        if (match) {
            title = match[1];
            year = parseInt(match[2], 10);
            if (year < 1888 || year > new Date().getFullYear() + 2) {
                year = null;
            }
        }
    }

    // Clean title
    title = title
        .replace(/[\._]/g, ' ')
        .replace(/\s+/g, ' ')
        .replace(/-+$/, '')
        .trim();

    return { title, year, season, episode, type, originalFilename: filename };
}

/**
 * Search OMDb for movie/series info
 */
async function searchOMDb(title, year, type, apiKey) {
    if (!apiKey) return null;

    await waitForRateLimit('omdb');

    try {
        const params = new URLSearchParams({
            apikey: apiKey,
            t: title,
            type: type === 'series' ? 'series' : 'movie'
        });

        if (year) params.append('y', year);

        const response = await makeRequest(`https://www.omdbapi.com/?${params.toString()}`);

        if (response.statusCode !== 200) return null;

        const data = JSON.parse(response.body);

        if (data.Response === 'False') {
            // Try without year
            if (year) {
                params.delete('y');
                const retryResponse = await makeRequest(`https://www.omdbapi.com/?${params.toString()}`);
                if (retryResponse.statusCode === 200) {
                    const retryData = JSON.parse(retryResponse.body);
                    if (retryData.Response === 'True') {
                        return retryData;
                    }
                }
            }
            return null;
        }

        return data;
    } catch (error) {
        console.error(`OMDb error: ${error.message}`);
        return null;
    }
}

/**
 * Search TVmaze for TV show info
 */
async function searchTVmaze(title, season, episode) {
    await waitForRateLimit('tvmaze');

    try {
        // Search for show
        const searchResponse = await makeRequest(
            `https://api.tvmaze.com/singlesearch/shows?q=${encodeURIComponent(title)}`
        );

        if (searchResponse.statusCode !== 200) return null;

        const show = JSON.parse(searchResponse.body);

        // Get episode info if season/episode provided
        let episodeData = null;
        if (season && episode) {
            await waitForRateLimit('tvmaze');
            const epResponse = await makeRequest(
                `https://api.tvmaze.com/shows/${show.id}/episodebynumber?season=${season}&number=${episode}`
            );

            if (epResponse.statusCode === 200) {
                episodeData = JSON.parse(epResponse.body);
            }
        }

        return { show, episode: episodeData };
    } catch (error) {
        console.error(`TVmaze error: ${error.message}`);
        return null;
    }
}

/**
 * Search Wikidata for additional info (fallback)
 */
async function searchWikidata(title, year) {
    await waitForRateLimit('wikidata');

    try {
        const query = `
            SELECT ?item ?itemLabel ?image ?imdbId WHERE {
                ?item wdt:P31/wdt:P279* wd:Q11424.
                ?item rdfs:label "${title}"@en.
                OPTIONAL { ?item wdt:P18 ?image. }
                OPTIONAL { ?item wdt:P345 ?imdbId. }
                SERVICE wikibase:label { bd:serviceParam wikibase:language "en". }
            }
            LIMIT 1
        `;

        const response = await makeRequest(
            `https://query.wikidata.org/sparql?query=${encodeURIComponent(query)}&format=json`
        );

        if (response.statusCode !== 200) return null;

        const data = JSON.parse(response.body);
        const results = data.results?.bindings;

        if (!results || results.length === 0) return null;

        return {
            title: results[0].itemLabel?.value,
            image: results[0].image?.value,
            imdbId: results[0].imdbId?.value
        };
    } catch (error) {
        console.error(`Wikidata error: ${error.message}`);
        return null;
    }
}

/**
 * Get metadata for a file
 */
async function getMetadata(filePath, omdbApiKey) {
    if (!db) return null;

    // Check cache first
    const cached = db.prepare('SELECT * FROM media_metadata WHERE file_path = ?').get(filePath);

    // Return cached if less than 7 days old
    if (cached && (Date.now() - cached.fetched_at) < 7 * 24 * 60 * 60 * 1000) {
        return cached;
    }

    const filename = path.basename(filePath);
    const extracted = extractFromFilename(filename);

    let metadata = {
        file_path: filePath,
        clean_title: extracted.title,
        year: extracted.year,
        type: extracted.type,
        season: extracted.season,
        episode: extracted.episode,
        source: 'filename',
        fetched_at: Date.now(),
        updated_at: Date.now()
    };

    // Try OMDb first (works for both movies and TV series)
    const omdbData = await searchOMDb(extracted.title, extracted.year, extracted.type, omdbApiKey);

    if (omdbData) {
        metadata.clean_title = omdbData.Title;
        metadata.year = parseInt(omdbData.Year) || extracted.year;
        metadata.plot = omdbData.Plot !== 'N/A' ? omdbData.Plot : null;
        metadata.rating = omdbData.imdbRating !== 'N/A' ? omdbData.imdbRating : null;
        metadata.genre = omdbData.Genre !== 'N/A' ? omdbData.Genre : null;
        metadata.director = omdbData.Director !== 'N/A' ? omdbData.Director : null;
        metadata.actors = omdbData.Actors !== 'N/A' ? omdbData.Actors : null;
        metadata.runtime = omdbData.Runtime !== 'N/A' ? omdbData.Runtime : null;
        metadata.imdb_id = omdbData.imdbID;
        metadata.source = 'omdb';

        // Download poster
        if (omdbData.Poster && omdbData.Poster !== 'N/A') {
            const posterPath = await downloadThumbnail(omdbData.Poster, omdbData.imdbID || extracted.title);
            metadata.poster_path = posterPath;
        }
    }

    // For TV episodes, get more details from TVmaze
    if (extracted.type === 'series' && extracted.season && extracted.episode) {
        const tvmazeData = await searchTVmaze(extracted.title, extracted.season, extracted.episode);

        if (tvmazeData) {
            if (tvmazeData.show) {
                metadata.tvmaze_id = tvmazeData.show.id;
                if (!metadata.clean_title || metadata.source === 'filename') {
                    metadata.clean_title = tvmazeData.show.name;
                }
                // Use TVmaze poster if we don't have one
                if (!metadata.poster_path && tvmazeData.show.image?.medium) {
                    const posterPath = await downloadThumbnail(
                        tvmazeData.show.image.medium,
                        `tvmaze_${tvmazeData.show.id}`
                    );
                    metadata.poster_path = posterPath;
                }
            }
            if (tvmazeData.episode) {
                metadata.episode_title = tvmazeData.episode.name;
                if (!metadata.plot) {
                    metadata.plot = tvmazeData.episode.summary?.replace(/<[^>]*>/g, '');
                }
                metadata.source = metadata.source === 'omdb' ? 'omdb+tvmaze' : 'tvmaze';
            }
        }
    }

    // Fallback to Wikidata if nothing found
    if (metadata.source === 'filename') {
        const wikidataResult = await searchWikidata(extracted.title, extracted.year);
        if (wikidataResult) {
            if (wikidataResult.title) metadata.clean_title = wikidataResult.title;
            if (wikidataResult.imdbId) metadata.imdb_id = wikidataResult.imdbId;
            if (wikidataResult.image && !metadata.poster_path) {
                const posterPath = await downloadThumbnail(wikidataResult.image, extracted.title);
                metadata.poster_path = posterPath;
            }
            metadata.source = 'wikidata';
        }
    }

    // Save to database
    saveMetadata(metadata);

    return metadata;
}

/**
 * Save metadata to database
 */
function saveMetadata(metadata) {
    if (!db) return;

    const stmt = db.prepare(`
        INSERT OR REPLACE INTO media_metadata
        (file_path, clean_title, year, type, poster_path, plot, rating, genre,
         director, actors, runtime, imdb_id, tvmaze_id, season, episode,
         episode_title, source, fetched_at, updated_at)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    `);

    stmt.run(
        metadata.file_path,
        metadata.clean_title,
        metadata.year,
        metadata.type,
        metadata.poster_path,
        metadata.plot,
        metadata.rating,
        metadata.genre,
        metadata.director,
        metadata.actors,
        metadata.runtime,
        metadata.imdb_id,
        metadata.tvmaze_id,
        metadata.season,
        metadata.episode,
        metadata.episode_title,
        metadata.source,
        metadata.fetched_at,
        metadata.updated_at || Date.now()
    );
}

/**
 * Get cached metadata for a file (no API calls)
 */
function getCachedMetadata(filePath) {
    if (!db) return null;
    return db.prepare('SELECT * FROM media_metadata WHERE file_path = ?').get(filePath);
}

/**
 * Get cached metadata for multiple files
 */
function getCachedMetadataBulk(filePaths) {
    if (!db || !filePaths.length) return {};

    const placeholders = filePaths.map(() => '?').join(',');
    const rows = db.prepare(`SELECT * FROM media_metadata WHERE file_path IN (${placeholders})`).all(...filePaths);

    const result = {};
    for (const row of rows) {
        result[row.file_path] = row;
    }
    return result;
}

/**
 * Scan a directory and fetch metadata for all media files
 */
async function scanDirectory(dirPath, omdbApiKey, progressCallback) {
    const mediaExtensions = /\.(mp4|mkv|avi|webm|mov|m4v|ogg)$/i;
    let scanned = 0;
    let total = 0;

    // Count total files first
    function countFiles(dir) {
        try {
            const items = fs.readdirSync(dir, { withFileTypes: true });
            for (const item of items) {
                if (item.isDirectory()) {
                    countFiles(path.join(dir, item.name));
                } else if (mediaExtensions.test(item.name)) {
                    total++;
                }
            }
        } catch (e) {
            // Skip inaccessible directories
        }
    }

    countFiles(dirPath);

    // Scan and fetch metadata
    async function processDirectory(dir) {
        try {
            const items = fs.readdirSync(dir, { withFileTypes: true });

            for (const item of items) {
                const fullPath = path.join(dir, item.name);

                if (item.isDirectory()) {
                    await processDirectory(fullPath);
                } else if (mediaExtensions.test(item.name)) {
                    // Check if already cached
                    const cached = getCachedMetadata(fullPath);
                    if (!cached || (Date.now() - cached.fetched_at) > 7 * 24 * 60 * 60 * 1000) {
                        await getMetadata(fullPath, omdbApiKey);
                    }
                    scanned++;

                    if (progressCallback) {
                        progressCallback({ scanned, total, current: item.name });
                    }
                }
            }
        } catch (e) {
            console.error(`Error scanning ${dir}: ${e.message}`);
        }
    }

    await processDirectory(dirPath);

    return { scanned, total };
}

/**
 * Get scan progress (for background scans)
 */
let currentScan = null;

async function startBackgroundScan(dirPath, omdbApiKey) {
    if (currentScan && currentScan.status === 'running') {
        return { error: 'Scan already in progress', ...currentScan };
    }

    currentScan = {
        status: 'running',
        scanned: 0,
        total: 0,
        current: '',
        startedAt: Date.now()
    };

    // Run scan in background
    scanDirectory(dirPath, omdbApiKey, (progress) => {
        currentScan.scanned = progress.scanned;
        currentScan.total = progress.total;
        currentScan.current = progress.current;
    }).then((result) => {
        currentScan.status = 'completed';
        currentScan.completedAt = Date.now();
    }).catch((error) => {
        currentScan.status = 'error';
        currentScan.error = error.message;
    });

    return currentScan;
}

function getScanProgress() {
    return currentScan || { status: 'idle' };
}

module.exports = {
    init,
    getMetadata,
    getCachedMetadata,
    getCachedMetadataBulk,
    scanDirectory,
    startBackgroundScan,
    getScanProgress,
    extractFromFilename
};
