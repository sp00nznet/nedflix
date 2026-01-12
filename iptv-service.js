/**
 * IPTV Service - Handles M3U playlists and EPG data
 */

const fs = require('fs');
const path = require('path');
const https = require('https');
const http = require('http');
const zlib = require('zlib');

// Cache for parsed data
let channelsCache = null;
let epgCache = null;
let lastPlaylistUpdate = 0;
let lastEpgUpdate = 0;

const CACHE_TTL = 30 * 60 * 1000; // 30 minutes

/**
 * Fetch content from URL or file path
 */
async function fetchContent(urlOrPath) {
    // Check if it's a local file
    if (urlOrPath.startsWith('/') || urlOrPath.startsWith('./')) {
        return fs.readFileSync(urlOrPath, 'utf8');
    }

    // Fetch from URL
    return new Promise((resolve, reject) => {
        const client = urlOrPath.startsWith('https') ? https : http;

        client.get(urlOrPath, (res) => {
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
 * Parse M3U/M3U8 playlist
 */
function parseM3U(content) {
    const lines = content.split('\n').map(l => l.trim()).filter(l => l);
    const channels = [];

    let currentChannel = null;

    for (let i = 0; i < lines.length; i++) {
        const line = lines[i];

        if (line.startsWith('#EXTINF:')) {
            // Parse channel info
            currentChannel = parseExtInf(line);
        } else if (line.startsWith('#EXTGRP:')) {
            // Group tag
            if (currentChannel) {
                currentChannel.group = line.substring(8).trim();
            }
        } else if (!line.startsWith('#') && currentChannel) {
            // Stream URL
            currentChannel.url = line;
            currentChannel.id = currentChannel.tvgId || `channel-${channels.length}`;
            channels.push(currentChannel);
            currentChannel = null;
        }
    }

    return channels;
}

/**
 * Parse #EXTINF line
 */
function parseExtInf(line) {
    const channel = {
        name: '',
        tvgId: '',
        tvgName: '',
        tvgLogo: '',
        group: 'Uncategorized',
        language: ''
    };

    // Extract duration and attributes
    const match = line.match(/#EXTINF:(-?\d+)\s*(.*),(.*)$/);
    if (match) {
        const attributes = match[2];
        channel.name = match[3].trim();

        // Parse attributes
        const tvgId = attributes.match(/tvg-id="([^"]*)"/);
        const tvgName = attributes.match(/tvg-name="([^"]*)"/);
        const tvgLogo = attributes.match(/tvg-logo="([^"]*)"/);
        const groupTitle = attributes.match(/group-title="([^"]*)"/);
        const tvgLanguage = attributes.match(/tvg-language="([^"]*)"/);

        if (tvgId) channel.tvgId = tvgId[1];
        if (tvgName) channel.tvgName = tvgName[1];
        if (tvgLogo) channel.tvgLogo = tvgLogo[1];
        if (groupTitle) channel.group = groupTitle[1] || 'Uncategorized';
        if (tvgLanguage) channel.language = tvgLanguage[1];
    }

    return channel;
}

/**
 * Parse XMLTV EPG data
 */
function parseEPG(content) {
    const epg = {
        channels: {},
        programs: {}
    };

    // Simple XML parsing for XMLTV format
    // Parse channels
    const channelRegex = /<channel id="([^"]*)">([\s\S]*?)<\/channel>/g;
    let match;

    while ((match = channelRegex.exec(content)) !== null) {
        const id = match[1];
        const channelContent = match[2];

        const nameMatch = channelContent.match(/<display-name[^>]*>([^<]*)<\/display-name>/);
        const iconMatch = channelContent.match(/<icon src="([^"]*)"/);

        epg.channels[id] = {
            id,
            name: nameMatch ? nameMatch[1] : id,
            icon: iconMatch ? iconMatch[1] : null
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
        const descMatch = programContent.match(/<desc[^>]*>([^<]*)<\/desc>/);
        const categoryMatch = programContent.match(/<category[^>]*>([^<]*)<\/category>/);
        const iconMatch = programContent.match(/<icon src="([^"]*)"/);

        if (!epg.programs[channelId]) {
            epg.programs[channelId] = [];
        }

        epg.programs[channelId].push({
            start,
            stop,
            title: titleMatch ? titleMatch[1] : 'Unknown Program',
            description: descMatch ? descMatch[1] : '',
            category: categoryMatch ? categoryMatch[1] : '',
            icon: iconMatch ? iconMatch[1] : null
        });
    }

    // Sort programs by start time
    for (const channelId in epg.programs) {
        epg.programs[channelId].sort((a, b) => a.start - b.start);
    }

    return epg;
}

/**
 * Parse XMLTV date format (YYYYMMDDHHmmss +/-HHMM)
 */
function parseXMLTVDate(dateStr) {
    const match = dateStr.match(/(\d{4})(\d{2})(\d{2})(\d{2})(\d{2})(\d{2})\s*([+-]\d{4})?/);
    if (!match) return Date.now();

    const [, year, month, day, hour, min, sec, tz] = match;
    let date = new Date(`${year}-${month}-${day}T${hour}:${min}:${sec}`);

    if (tz) {
        const tzHours = parseInt(tz.substring(0, 3));
        const tzMins = parseInt(tz.substring(3));
        date = new Date(date.getTime() - (tzHours * 60 + tzMins) * 60 * 1000);
    }

    return date.getTime();
}

/**
 * Load and parse playlist
 */
async function loadPlaylist(playlistUrl) {
    if (!playlistUrl) return [];

    const now = Date.now();
    if (channelsCache && (now - lastPlaylistUpdate) < CACHE_TTL) {
        return channelsCache;
    }

    try {
        const content = await fetchContent(playlistUrl);
        channelsCache = parseM3U(content);
        lastPlaylistUpdate = now;
        console.log(`Loaded ${channelsCache.length} channels from playlist`);
        return channelsCache;
    } catch (error) {
        console.error('Failed to load playlist:', error.message);
        return channelsCache || [];
    }
}

/**
 * Load and parse EPG
 */
async function loadEPG(epgUrl) {
    if (!epgUrl) return { channels: {}, programs: {} };

    const now = Date.now();
    if (epgCache && (now - lastEpgUpdate) < CACHE_TTL) {
        return epgCache;
    }

    try {
        const content = await fetchContent(epgUrl);
        epgCache = parseEPG(content);
        lastEpgUpdate = now;
        console.log(`Loaded EPG with ${Object.keys(epgCache.channels).length} channels`);
        return epgCache;
    } catch (error) {
        console.error('Failed to load EPG:', error.message);
        return epgCache || { channels: {}, programs: {} };
    }
}

/**
 * Get current and next program for a channel
 */
function getCurrentProgram(channelId, epg) {
    const programs = epg.programs[channelId];
    if (!programs || programs.length === 0) return { current: null, next: null };

    const now = Date.now();
    let current = null;
    let next = null;

    for (let i = 0; i < programs.length; i++) {
        const program = programs[i];
        if (program.start <= now && program.stop > now) {
            current = program;
            next = programs[i + 1] || null;
            break;
        } else if (program.start > now && !next) {
            next = program;
        }
    }

    return { current, next };
}

/**
 * Get programs for a time range
 */
function getProgramsInRange(channelId, epg, startTime, endTime) {
    const programs = epg.programs[channelId];
    if (!programs) return [];

    return programs.filter(p =>
        (p.start >= startTime && p.start < endTime) ||
        (p.stop > startTime && p.stop <= endTime) ||
        (p.start <= startTime && p.stop >= endTime)
    );
}

/**
 * Get channels grouped by category
 */
function getChannelsByGroup(channels) {
    const groups = {};

    for (const channel of channels) {
        const group = channel.group || 'Uncategorized';
        if (!groups[group]) {
            groups[group] = [];
        }
        groups[group].push(channel);
    }

    return groups;
}

/**
 * Clear cache (force refresh)
 */
function clearCache() {
    channelsCache = null;
    epgCache = null;
    lastPlaylistUpdate = 0;
    lastEpgUpdate = 0;
}

module.exports = {
    loadPlaylist,
    loadEPG,
    getCurrentProgram,
    getProgramsInRange,
    getChannelsByGroup,
    clearCache
};
