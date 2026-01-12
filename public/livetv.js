// ==================== LIVE TV / IPTV ====================

// Live TV elements
const liveTvView = document.getElementById('livetv-view');
const liveTvCard = document.getElementById('livetv-card');
const liveTvBackBtn = document.getElementById('livetv-back-btn');
const liveTvRefreshBtn = document.getElementById('livetv-refresh-btn');
const liveTvGroups = document.getElementById('livetv-groups');
const liveTvChannels = document.getElementById('livetv-channels');
const liveTvPlayer = document.getElementById('livetv-player');
const liveTvPlaceholder = document.getElementById('livetv-placeholder');
const liveTvNoConfig = document.getElementById('livetv-no-config');
const liveTvOpenSettings = document.getElementById('livetv-open-settings');
const liveTvChannelLogo = document.getElementById('livetv-channel-logo');
const liveTvChannelName = document.getElementById('livetv-channel-name');
const liveTvNowTitle = document.getElementById('livetv-now-title');
const liveTvNowTime = document.getElementById('livetv-now-time');
const liveTvNextTitle = document.getElementById('livetv-next-title');
const liveTvNextTime = document.getElementById('livetv-next-time');

// IPTV settings inputs
const iptvPlaylistUrlInput = document.getElementById('iptv-playlist-url');
const iptvEpgUrlInput = document.getElementById('iptv-epg-url');

// Live TV state
let liveTvChannelsData = [];
let liveTvGroupsData = {};
let liveTvCurrentGroup = 'All';
let liveTvCurrentChannel = null;
let liveTvEpgData = null;
let liveTvEpgInterval = null;

// Initialize Live TV event listeners
function initLiveTv() {
    if (!liveTvCard) return;

    // Live TV card click
    liveTvCard.addEventListener('click', openLiveTv);

    // Back button
    if (liveTvBackBtn) {
        liveTvBackBtn.addEventListener('click', closeLiveTv);
    }

    // Refresh button
    if (liveTvRefreshBtn) {
        liveTvRefreshBtn.addEventListener('click', async () => {
            await fetch('/api/iptv/refresh', { method: 'POST' });
            await loadLiveTvChannels();
        });
    }

    // Open settings button
    if (liveTvOpenSettings) {
        liveTvOpenSettings.addEventListener('click', () => {
            closeLiveTv();
            userPanelOverlay.classList.add('active');
        });
    }
}

// Open Live TV view
async function openLiveTv() {
    // Hide other views
    librarySelector.style.display = 'none';
    mainContent.style.display = 'none';
    liveTvView.style.display = 'flex';

    // Load channels
    await loadLiveTvChannels();
    await loadLiveTvEpg();

    // Start EPG refresh interval
    if (liveTvEpgInterval) clearInterval(liveTvEpgInterval);
    liveTvEpgInterval = setInterval(updateCurrentPrograms, 60000);
}

// Close Live TV view
function closeLiveTv() {
    liveTvView.style.display = 'none';
    librarySelector.style.display = 'flex';

    // Stop player
    if (liveTvPlayer) {
        liveTvPlayer.pause();
        liveTvPlayer.src = '';
    }

    // Clear interval
    if (liveTvEpgInterval) {
        clearInterval(liveTvEpgInterval);
        liveTvEpgInterval = null;
    }
}

// Load Live TV channels
async function loadLiveTvChannels() {
    try {
        const response = await fetch('/api/iptv/channels');
        const data = await response.json();

        if (!data.configured) {
            showLiveTvNoConfig();
            return;
        }

        hideLiveTvNoConfig();
        liveTvChannelsData = data.channels;
        liveTvGroupsData = data.groups;

        renderLiveTvGroups(data.groupNames);
        renderLiveTvChannels(liveTvChannelsData);
    } catch (error) {
        console.error('Failed to load Live TV channels:', error);
        showLiveTvNoConfig();
    }
}

// Load EPG data
async function loadLiveTvEpg() {
    try {
        const response = await fetch('/api/iptv/epg');
        liveTvEpgData = await response.json();
        updateCurrentPrograms();
    } catch (error) {
        console.error('Failed to load EPG:', error);
    }
}

// Show no config message
function showLiveTvNoConfig() {
    if (liveTvNoConfig) liveTvNoConfig.style.display = 'flex';
    const container = document.querySelector('.livetv-container');
    if (container) container.style.display = 'none';
}

// Hide no config message
function hideLiveTvNoConfig() {
    if (liveTvNoConfig) liveTvNoConfig.style.display = 'none';
    const container = document.querySelector('.livetv-container');
    if (container) container.style.display = 'grid';
}

// Render channel groups
function renderLiveTvGroups(groupNames) {
    if (!liveTvGroups) return;

    let html = '<button class="livetv-group-btn active" data-group="All">All</button>';
    for (const name of groupNames) {
        html += '<button class="livetv-group-btn" data-group="' + escapeHtml(name) + '">' + escapeHtml(name) + '</button>';
    }

    liveTvGroups.innerHTML = html;

    // Add click handlers
    liveTvGroups.querySelectorAll('.livetv-group-btn').forEach(btn => {
        btn.addEventListener('click', () => {
            liveTvGroups.querySelectorAll('.livetv-group-btn').forEach(b => b.classList.remove('active'));
            btn.classList.add('active');
            liveTvCurrentGroup = btn.dataset.group;
            filterLiveTvChannels();
        });
    });
}

// Filter channels by group
function filterLiveTvChannels() {
    if (liveTvCurrentGroup === 'All') {
        renderLiveTvChannels(liveTvChannelsData);
    } else {
        const filtered = liveTvGroupsData[liveTvCurrentGroup] || [];
        renderLiveTvChannels(filtered);
    }
}

// Render channels list
function renderLiveTvChannels(channels) {
    if (!liveTvChannels) return;

    let html = '';
    channels.forEach((channel, index) => {
        const logoHtml = channel.tvgLogo
            ? '<img src="' + escapeHtml(channel.tvgLogo) + '" alt="" onerror="this.style.display=\'none\'">'
            : '<div style="width:40px;height:40px;background:var(--color-bg-card);border-radius:var(--radius-sm)"></div>';

        html += '<div class="livetv-channel-item" data-url="' + escapeHtml(channel.url) + '" data-id="' + escapeHtml(channel.tvgId || channel.id) + '" data-name="' + escapeHtml(channel.name) + '" data-logo="' + escapeHtml(channel.tvgLogo || '') + '">' +
            '<span class="channel-number">' + (index + 1) + '</span>' +
            logoHtml +
            '<span class="channel-name">' + escapeHtml(channel.name) + '</span>' +
            '</div>';
    });

    liveTvChannels.innerHTML = html;

    // Add click handlers
    liveTvChannels.querySelectorAll('.livetv-channel-item').forEach(item => {
        item.addEventListener('click', () => playLiveTvChannel(item));
    });
}

// Play a channel
function playLiveTvChannel(element) {
    const url = element.dataset.url;
    const channelId = element.dataset.id;
    const name = element.dataset.name;
    const logo = element.dataset.logo;

    // Update active state
    liveTvChannels.querySelectorAll('.livetv-channel-item').forEach(el => el.classList.remove('active'));
    element.classList.add('active');

    // Update channel info
    liveTvCurrentChannel = { id: channelId, name, logo, url };
    if (liveTvChannelLogo) {
        liveTvChannelLogo.src = logo || '';
        liveTvChannelLogo.style.display = logo ? 'block' : 'none';
    }
    if (liveTvChannelName) liveTvChannelName.textContent = name;

    // Hide placeholder, show player
    if (liveTvPlaceholder) liveTvPlaceholder.classList.add('hidden');

    // Play stream (use proxy for CORS)
    const streamUrl = '/api/iptv/stream?url=' + encodeURIComponent(url);
    liveTvPlayer.src = streamUrl;
    liveTvPlayer.play().catch(e => {
        // Try direct URL if proxy fails
        console.log('Proxy failed, trying direct:', e);
        liveTvPlayer.src = url;
        liveTvPlayer.play().catch(e2 => console.error('Playback failed:', e2));
    });

    // Update EPG info
    updateChannelEpg(channelId);
}

// Update EPG for current channel
function updateChannelEpg(channelId) {
    if (!liveTvEpgData || !channelId) {
        if (liveTvNowTitle) liveTvNowTitle.textContent = '-';
        if (liveTvNowTime) liveTvNowTime.textContent = '-';
        if (liveTvNextTitle) liveTvNextTitle.textContent = '-';
        if (liveTvNextTime) liveTvNextTime.textContent = '-';
        return;
    }

    const programs = liveTvEpgData.programs[channelId] || [];
    const now = Date.now();

    let current = null;
    let next = null;

    for (let i = 0; i < programs.length; i++) {
        const prog = programs[i];
        if (prog.start <= now && prog.stop > now) {
            current = prog;
            next = programs[i + 1] || null;
            break;
        } else if (prog.start > now && !next) {
            next = prog;
        }
    }

    if (liveTvNowTitle) liveTvNowTitle.textContent = current?.title || '-';
    if (liveTvNowTime) liveTvNowTime.textContent = current ? formatEpgTime(current.start, current.stop) : '-';
    if (liveTvNextTitle) liveTvNextTitle.textContent = next?.title || '-';
    if (liveTvNextTime) liveTvNextTime.textContent = next ? formatEpgTime(next.start, next.stop) : '-';
}

// Format EPG time range
function formatEpgTime(start, stop) {
    const startDate = new Date(start);
    const stopDate = new Date(stop);
    const fmt = (d) => d.toLocaleTimeString('en-US', { hour: 'numeric', minute: '2-digit' });
    return fmt(startDate) + ' - ' + fmt(stopDate);
}

// Update current programs for all channels
function updateCurrentPrograms() {
    if (liveTvCurrentChannel) {
        updateChannelEpg(liveTvCurrentChannel.id);
    }
}

// Apply IPTV settings when loading user settings
function applyIptvSettings() {
    if (userSettings?.iptv) {
        if (iptvPlaylistUrlInput) iptvPlaylistUrlInput.value = userSettings.iptv.playlistUrl || '';
        if (iptvEpgUrlInput) iptvEpgUrlInput.value = userSettings.iptv.epgUrl || '';
    }
}

// Initialize on page load
document.addEventListener('DOMContentLoaded', () => {
    initLiveTv();
});
