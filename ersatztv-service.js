/**
 * ErsatzTV Service - Manages integration with ErsatzTV for auto-generated channels
 *
 * This service handles:
 * - Connecting to ErsatzTV API
 * - Creating local media libraries from NFS mount
 * - Auto-generating channels from media collections
 * - Providing M3U playlist and EPG URLs
 */

const http = require('http');
const https = require('https');

// ErsatzTV API configuration
const ERSATZTV_URL = process.env.ERSATZTV_URL || 'http://localhost:8409';
const NFS_MOUNT_PATH = process.env.NFS_MOUNT_PATH || '/mnt/nfs';

// Channel configuration presets
const CHANNEL_PRESETS = {
    movies: {
        name: 'Movies 24/7',
        number: 1,
        ffmpegProfile: 'default',
        playoutMode: 'shuffle',
        icon: 'movie'
    },
    tv: {
        name: 'TV Shows Marathon',
        number: 2,
        ffmpegProfile: 'default',
        playoutMode: 'shuffle',
        icon: 'tv'
    },
    music: {
        name: 'Music Videos',
        number: 3,
        ffmpegProfile: 'default',
        playoutMode: 'shuffle',
        icon: 'music'
    },
    mixed: {
        name: 'Mixed Entertainment',
        number: 4,
        ffmpegProfile: 'default',
        playoutMode: 'shuffle',
        icon: 'mixed'
    }
};

/**
 * Make HTTP request to ErsatzTV API
 */
function makeRequest(endpoint, options = {}) {
    return new Promise((resolve, reject) => {
        const url = new URL(endpoint, ERSATZTV_URL);
        const isHttps = url.protocol === 'https:';
        const client = isHttps ? https : http;

        const requestOptions = {
            hostname: url.hostname,
            port: url.port || (isHttps ? 443 : 80),
            path: url.pathname + url.search,
            method: options.method || 'GET',
            headers: {
                'Content-Type': 'application/json',
                'Accept': 'application/json',
                ...options.headers
            }
        };

        const req = client.request(requestOptions, (res) => {
            let data = '';

            res.on('data', (chunk) => {
                data += chunk;
            });

            res.on('end', () => {
                try {
                    const parsed = data ? JSON.parse(data) : null;
                    if (res.statusCode >= 200 && res.statusCode < 300) {
                        resolve(parsed);
                    } else {
                        reject(new Error(`HTTP ${res.statusCode}: ${parsed?.error || data}`));
                    }
                } catch (e) {
                    if (res.statusCode >= 200 && res.statusCode < 300) {
                        resolve(data);
                    } else {
                        reject(new Error(`HTTP ${res.statusCode}: ${data}`));
                    }
                }
            });
        });

        req.on('error', reject);
        req.setTimeout(30000, () => {
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
 * Check if ErsatzTV is available
 */
async function checkHealth() {
    try {
        await makeRequest('/api/health');
        return { available: true, url: ERSATZTV_URL };
    } catch (error) {
        return { available: false, url: ERSATZTV_URL, error: error.message };
    }
}

/**
 * Get all local libraries from ErsatzTV
 */
async function getLocalLibraries() {
    try {
        const libraries = await makeRequest('/api/libraries/local');
        return libraries || [];
    } catch (error) {
        console.error('Failed to get local libraries:', error.message);
        return [];
    }
}

/**
 * Create a local library in ErsatzTV
 */
async function createLocalLibrary(name, paths, mediaKind = 'Movies') {
    try {
        const result = await makeRequest('/api/libraries/local', {
            method: 'POST',
            body: {
                name: name,
                paths: paths,
                mediaKind: mediaKind  // Movies, Shows, MusicVideos, OtherVideos
            }
        });
        return result;
    } catch (error) {
        console.error(`Failed to create library ${name}:`, error.message);
        throw error;
    }
}

/**
 * Scan a local library
 */
async function scanLibrary(libraryId) {
    try {
        await makeRequest(`/api/libraries/local/${libraryId}/scan`, {
            method: 'POST'
        });
        return { success: true };
    } catch (error) {
        console.error(`Failed to scan library ${libraryId}:`, error.message);
        throw error;
    }
}

/**
 * Get all channels from ErsatzTV
 */
async function getChannels() {
    try {
        const channels = await makeRequest('/api/channels');
        return channels || [];
    } catch (error) {
        console.error('Failed to get channels:', error.message);
        return [];
    }
}

/**
 * Create a new channel in ErsatzTV
 */
async function createChannel(name, number, ffmpegProfileId = null) {
    try {
        const body = {
            name: name,
            number: String(number),
            ffmpegProfileId: ffmpegProfileId,
            preferredAudioLanguageCode: 'eng',
            preferredAudioTitle: '',
            preferredSubtitleLanguageCode: '',
            subtitleMode: 'None',
            musicVideoCreditsMode: 'None',
            streamingMode: 'TransportStream'
        };

        const result = await makeRequest('/api/channels', {
            method: 'POST',
            body: body
        });
        return result;
    } catch (error) {
        console.error(`Failed to create channel ${name}:`, error.message);
        throw error;
    }
}

/**
 * Get FFmpeg profiles
 */
async function getFFmpegProfiles() {
    try {
        const profiles = await makeRequest('/api/ffmpeg/profiles');
        return profiles || [];
    } catch (error) {
        console.error('Failed to get FFmpeg profiles:', error.message);
        return [];
    }
}

/**
 * Get all collections (smart collections, etc.)
 */
async function getCollections() {
    try {
        const collections = await makeRequest('/api/collections');
        return collections || [];
    } catch (error) {
        console.error('Failed to get collections:', error.message);
        return [];
    }
}

/**
 * Create a smart collection
 */
async function createSmartCollection(name, query) {
    try {
        const result = await makeRequest('/api/collections/smart', {
            method: 'POST',
            body: {
                name: name,
                query: query
            }
        });
        return result;
    } catch (error) {
        console.error(`Failed to create smart collection ${name}:`, error.message);
        throw error;
    }
}

/**
 * Add items to a channel's schedule (using a collection)
 */
async function addCollectionToSchedule(channelId, collectionId, playoutMode = 'Shuffle') {
    try {
        const result = await makeRequest(`/api/channels/${channelId}/schedule/items`, {
            method: 'POST',
            body: {
                collectionId: collectionId,
                collectionType: 'Collection',
                playbackOrder: playoutMode === 'Shuffle' ? 'Shuffle' : 'Chronological'
            }
        });
        return result;
    } catch (error) {
        console.error(`Failed to add collection to channel schedule:`, error.message);
        throw error;
    }
}

/**
 * Get media items from a library
 */
async function getLibraryMedia(libraryId, mediaKind = 'Movies') {
    try {
        const endpoint = mediaKind === 'Movies'
            ? `/api/media/movies?libraryId=${libraryId}`
            : `/api/media/shows?libraryId=${libraryId}`;
        const media = await makeRequest(endpoint);
        return media || [];
    } catch (error) {
        console.error(`Failed to get library media:`, error.message);
        return [];
    }
}

/**
 * Setup initial ErsatzTV configuration with NFS media
 * Creates libraries pointing to NFS mount and generates channels
 */
async function setupAutoChannels() {
    const results = {
        libraries: [],
        channels: [],
        errors: []
    };

    try {
        // Check if ErsatzTV is available
        const health = await checkHealth();
        if (!health.available) {
            throw new Error('ErsatzTV is not available');
        }

        // Get existing libraries to avoid duplicates
        const existingLibraries = await getLocalLibraries();
        const existingLibraryPaths = new Set(
            existingLibraries.flatMap(lib => lib.paths || [])
        );

        // Get existing channels
        const existingChannels = await getChannels();
        const existingChannelNumbers = new Set(
            existingChannels.map(ch => String(ch.number))
        );

        // Get FFmpeg profiles
        const ffmpegProfiles = await getFFmpegProfiles();
        const defaultProfile = ffmpegProfiles.find(p => p.name === 'Default') || ffmpegProfiles[0];

        // Define libraries to create
        const librariesToCreate = [
            { name: 'NFS Movies', path: `${NFS_MOUNT_PATH}/Movies`, mediaKind: 'Movies' },
            { name: 'NFS TV Shows', path: `${NFS_MOUNT_PATH}/TV Shows`, mediaKind: 'Shows' },
            { name: 'NFS Music', path: `${NFS_MOUNT_PATH}/Music`, mediaKind: 'MusicVideos' }
        ];

        // Create libraries that don't exist
        for (const lib of librariesToCreate) {
            if (!existingLibraryPaths.has(lib.path)) {
                try {
                    const created = await createLocalLibrary(lib.name, [lib.path], lib.mediaKind);
                    results.libraries.push({ name: lib.name, id: created?.id, status: 'created' });

                    // Scan the newly created library
                    if (created?.id) {
                        await scanLibrary(created.id);
                    }
                } catch (error) {
                    results.errors.push({ type: 'library', name: lib.name, error: error.message });
                }
            } else {
                results.libraries.push({ name: lib.name, status: 'exists' });
            }
        }

        // Wait for scans to complete (simplified - in production you'd poll status)
        await new Promise(resolve => setTimeout(resolve, 5000));

        // Refresh libraries list after creation
        const allLibraries = await getLocalLibraries();

        // Create channels for each library type
        const channelConfigs = [
            {
                preset: CHANNEL_PRESETS.movies,
                libraryName: 'NFS Movies',
                mediaKind: 'Movies'
            },
            {
                preset: CHANNEL_PRESETS.tv,
                libraryName: 'NFS TV Shows',
                mediaKind: 'Shows'
            },
            {
                preset: CHANNEL_PRESETS.music,
                libraryName: 'NFS Music',
                mediaKind: 'MusicVideos'
            }
        ];

        for (const config of channelConfigs) {
            const channelNumber = String(config.preset.number);

            if (!existingChannelNumbers.has(channelNumber)) {
                try {
                    // Find the library for this channel
                    const library = allLibraries.find(lib => lib.name === config.libraryName);

                    if (library) {
                        // Create the channel
                        const channel = await createChannel(
                            config.preset.name,
                            config.preset.number,
                            defaultProfile?.id
                        );

                        results.channels.push({
                            name: config.preset.name,
                            number: channelNumber,
                            id: channel?.id,
                            status: 'created'
                        });
                    }
                } catch (error) {
                    results.errors.push({
                        type: 'channel',
                        name: config.preset.name,
                        error: error.message
                    });
                }
            } else {
                results.channels.push({
                    name: config.preset.name,
                    number: channelNumber,
                    status: 'exists'
                });
            }
        }

        return {
            success: true,
            ...results
        };

    } catch (error) {
        return {
            success: false,
            error: error.message,
            ...results
        };
    }
}

/**
 * Get the M3U playlist URL for all channels
 */
function getPlaylistUrl() {
    return `${ERSATZTV_URL}/iptv/channels.m3u`;
}

/**
 * Get the XMLTV EPG URL
 */
function getEpgUrl() {
    return `${ERSATZTV_URL}/iptv/xmltv.xml`;
}

/**
 * Get channel guide/schedule for a specific channel
 */
async function getChannelGuide(channelId) {
    try {
        const guide = await makeRequest(`/api/channels/${channelId}/guide`);
        return guide || [];
    } catch (error) {
        console.error(`Failed to get channel guide:`, error.message);
        return [];
    }
}

/**
 * Delete a channel
 */
async function deleteChannel(channelId) {
    try {
        await makeRequest(`/api/channels/${channelId}`, {
            method: 'DELETE'
        });
        return { success: true };
    } catch (error) {
        console.error(`Failed to delete channel ${channelId}:`, error.message);
        throw error;
    }
}

/**
 * Update a channel
 */
async function updateChannel(channelId, updates) {
    try {
        const result = await makeRequest(`/api/channels/${channelId}`, {
            method: 'PUT',
            body: updates
        });
        return result;
    } catch (error) {
        console.error(`Failed to update channel ${channelId}:`, error.message);
        throw error;
    }
}

/**
 * Get status of all services
 */
async function getStatus() {
    const health = await checkHealth();

    if (!health.available) {
        return {
            available: false,
            url: ERSATZTV_URL,
            error: health.error,
            libraries: [],
            channels: [],
            playlistUrl: null,
            epgUrl: null
        };
    }

    const [libraries, channels] = await Promise.all([
        getLocalLibraries(),
        getChannels()
    ]);

    return {
        available: true,
        url: ERSATZTV_URL,
        libraries: libraries,
        channels: channels,
        playlistUrl: getPlaylistUrl(),
        epgUrl: getEpgUrl()
    };
}

/**
 * Force rebuild playout for a channel
 */
async function rebuildChannelPlayout(channelId) {
    try {
        await makeRequest(`/api/channels/${channelId}/playout/build`, {
            method: 'POST'
        });
        return { success: true };
    } catch (error) {
        console.error(`Failed to rebuild channel playout:`, error.message);
        throw error;
    }
}

// ==========================================
// THEMED CHANNELS INTEGRATION
// ==========================================

const themedChannels = require('./themed-channels');

/**
 * Get all available themed channel presets
 */
function getThemedChannelPresets() {
    return themedChannels.getAllChannels().map(ch => ({
        key: Object.keys(themedChannels.THEMED_CHANNELS).find(
            k => themedChannels.THEMED_CHANNELS[k] === ch
        ),
        name: ch.name,
        number: ch.number,
        description: ch.description,
        icon: ch.icon,
        hasSchedule: ch.schedule.length > 0,
        collectionCount: Object.keys(ch.collections).length
    }));
}

/**
 * Create a schedule block in ErsatzTV
 */
async function createScheduleBlock(channelId, block) {
    try {
        const result = await makeRequest(`/api/channels/${channelId}/schedule`, {
            method: 'POST',
            body: {
                index: block.index || 0,
                startTime: block.startTime,
                playoutMode: block.playoutMode || 'Shuffle',
                collectionType: 'SmartCollection',
                collectionId: block.collectionId,
                multipleCount: 1,
                durationSeconds: (block.durationMinutes || 60) * 60,
                offlineTail: false,
                customTitle: block.title || null
            }
        });
        return result;
    } catch (error) {
        console.error(`Failed to create schedule block:`, error.message);
        throw error;
    }
}

/**
 * Create a themed channel with all its collections and schedules
 */
async function createThemedChannel(channelKey) {
    const channelConfig = themedChannels.getChannel(channelKey);
    if (!channelConfig) {
        throw new Error(`Unknown themed channel: ${channelKey}`);
    }

    const results = {
        channel: null,
        collections: [],
        scheduleBlocks: [],
        errors: []
    };

    try {
        // Check if channel number is already in use
        const existingChannels = await getChannels();
        const numberInUse = existingChannels.find(
            ch => String(ch.number) === String(channelConfig.number)
        );
        if (numberInUse) {
            throw new Error(`Channel number ${channelConfig.number} already in use by "${numberInUse.name}"`);
        }

        // Get FFmpeg profiles
        const profiles = await getFFmpegProfiles();
        const defaultProfile = profiles.find(p => p.name === 'Default') || profiles[0];

        // Create the channel
        const channel = await createChannel(
            channelConfig.name,
            channelConfig.number,
            defaultProfile?.id
        );
        results.channel = {
            id: channel?.id,
            name: channelConfig.name,
            number: channelConfig.number
        };

        // Create smart collections for this channel
        for (const [collName, collConfig] of Object.entries(channelConfig.collections)) {
            try {
                const collection = await createSmartCollection(
                    `${channelConfig.name} - ${collName}`,
                    collConfig.query
                );
                results.collections.push({
                    name: collName,
                    id: collection?.id,
                    query: collConfig.query,
                    status: 'created'
                });
            } catch (error) {
                results.errors.push({
                    type: 'collection',
                    name: collName,
                    error: error.message
                });
            }
        }

        // Note: Schedule blocks require additional ErsatzTV API setup
        // which varies by ErsatzTV version. This provides the framework.
        results.scheduleBlocks = themedChannels.toErsatzTVSchedule(channelConfig);

        return {
            success: true,
            ...results
        };

    } catch (error) {
        return {
            success: false,
            error: error.message,
            ...results
        };
    }
}

/**
 * Setup all themed channels at once
 */
async function setupThemedChannels(channelKeys = null) {
    const results = {
        success: true,
        channels: [],
        errors: []
    };

    // If no keys specified, setup all themed channels
    const keysToSetup = channelKeys || Object.keys(themedChannels.THEMED_CHANNELS);

    for (const key of keysToSetup) {
        try {
            const result = await createThemedChannel(key);
            results.channels.push({
                key,
                ...result
            });
            if (!result.success) {
                results.errors.push({
                    channel: key,
                    error: result.error
                });
            }
        } catch (error) {
            results.errors.push({
                channel: key,
                error: error.message
            });
        }
    }

    results.success = results.errors.length === 0;
    return results;
}

/**
 * Get themed channels that would work with current library content
 */
async function getRecommendedThemedChannels() {
    const recommendations = [];

    try {
        const libraries = await getLocalLibraries();
        const hasMovies = libraries.some(lib => lib.mediaKind === 'Movies');
        const hasShows = libraries.some(lib => lib.mediaKind === 'Shows');
        const hasMusic = libraries.some(lib => lib.mediaKind === 'MusicVideos');

        const allChannels = themedChannels.getAllChannels();

        for (const channel of allChannels) {
            const queries = Object.values(channel.collections).map(c => c.query);
            const needsMovies = queries.some(q => q.includes('type:movie'));
            const needsShows = queries.some(q => q.includes('type:show'));
            const needsMusic = queries.some(q => q.includes('type:musicvideo'));

            let compatible = true;
            let reason = [];

            if (needsMovies && !hasMovies) {
                compatible = false;
                reason.push('Requires movie library');
            }
            if (needsShows && !hasShows) {
                compatible = false;
                reason.push('Requires TV show library');
            }
            if (needsMusic && !hasMusic) {
                compatible = false;
                reason.push('Requires music video library');
            }

            recommendations.push({
                key: Object.keys(themedChannels.THEMED_CHANNELS).find(
                    k => themedChannels.THEMED_CHANNELS[k] === channel
                ),
                name: channel.name,
                number: channel.number,
                description: channel.description,
                compatible,
                reason: reason.length > 0 ? reason.join(', ') : 'Ready to use'
            });
        }

    } catch (error) {
        console.error('Failed to get channel recommendations:', error.message);
    }

    return recommendations;
}

module.exports = {
    // Core ErsatzTV functions
    checkHealth,
    getLocalLibraries,
    createLocalLibrary,
    scanLibrary,
    getChannels,
    createChannel,
    deleteChannel,
    updateChannel,
    getFFmpegProfiles,
    getCollections,
    createSmartCollection,
    addCollectionToSchedule,
    getLibraryMedia,
    setupAutoChannels,
    getPlaylistUrl,
    getEpgUrl,
    getChannelGuide,
    getStatus,
    rebuildChannelPlayout,
    CHANNEL_PRESETS,

    // Themed channels
    getThemedChannelPresets,
    createThemedChannel,
    setupThemedChannels,
    getRecommendedThemedChannels,
    createScheduleBlock,
    themedChannels
};
