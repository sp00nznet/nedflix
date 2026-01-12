/**
 * Nedflix Desktop - Channels (ErsatzTV) Module
 */

// Channels elements
const channelsView = document.getElementById('channels-view');
const channelsCard = document.getElementById('channels-card');
const channelsBackBtn = document.getElementById('channels-back-btn');
const channelsRefreshBtn = document.getElementById('channels-refresh-btn');
const channelsList = document.getElementById('channels-list');
const channelsPlayer = document.getElementById('channels-player');
const channelsPlaceholder = document.getElementById('channels-placeholder');
const channelsNoConfig = document.getElementById('channels-no-config');
const channelsOpenSettings = document.getElementById('channels-open-settings');
const channelsStatus = document.getElementById('channels-status');
const channelsChannelNumber = document.getElementById('channels-channel-number');
const channelsChannelName = document.getElementById('channels-channel-name');
const channelsNowTitle = document.getElementById('channels-now-title');

// Channels state
let channelsData = [];
let channelsPlaylistUrl = null;
let channelsEpgUrl = null;
let channelsCurrentChannel = null;
let ersatztvAvailable = false;

// Initialize Channels event listeners
function initChannels() {
    if (!channelsCard) return;

    // Channels card click
    channelsCard.addEventListener('click', openChannels);

    // Back button
    if (channelsBackBtn) {
        channelsBackBtn.addEventListener('click', closeChannels);
    }

    // Refresh button
    if (channelsRefreshBtn) {
        channelsRefreshBtn.addEventListener('click', async () => {
            await loadChannels();
        });
    }

    // Open settings button
    if (channelsOpenSettings) {
        channelsOpenSettings.addEventListener('click', () => {
            closeChannels();
            const userPanelOverlay = document.getElementById('user-panel-overlay');
            if (userPanelOverlay) userPanelOverlay.classList.add('active');
        });
    }
}

// Open Channels view
async function openChannels() {
    // Hide other views
    const librarySelector = document.getElementById('library-selector');
    const mainContent = document.getElementById('main-content');
    const liveTvView = document.getElementById('livetv-view');

    if (librarySelector) librarySelector.style.display = 'none';
    if (mainContent) mainContent.style.display = 'none';
    if (liveTvView) liveTvView.style.display = 'none';
    if (channelsView) channelsView.style.display = 'flex';

    // Load channels
    await loadChannels();
}

// Close Channels view
function closeChannels() {
    const librarySelector = document.getElementById('library-selector');

    if (channelsView) channelsView.style.display = 'none';
    if (librarySelector) librarySelector.style.display = 'flex';

    // Stop player
    if (channelsPlayer) {
        channelsPlayer.pause();
        channelsPlayer.src = '';
    }
}

// Load channels from ErsatzTV
async function loadChannels() {
    try {
        updateChannelsStatus('loading', 'Checking ErsatzTV...');

        const response = await fetch('/api/ersatztv/status');
        const data = await response.json();

        ersatztvAvailable = data.available;

        if (!data.configured) {
            updateChannelsStatus('error', 'Not configured');
            showChannelsNoConfig('ErsatzTV server URL is not configured. Add a server URL in Settings.');
            return;
        }

        if (!data.available) {
            updateChannelsStatus('error', 'Unavailable');
            showChannelsNoConfig('Cannot connect to ErsatzTV server. Please check the server URL and ensure ErsatzTV is running.');
            return;
        }

        channelsPlaylistUrl = data.playlistUrl;
        channelsEpgUrl = data.epgUrl;

        if (!data.channels || data.channels.length === 0) {
            updateChannelsStatus('warning', 'No channels');
            showChannelsNoConfig('No channels have been created in ErsatzTV yet. Create channels in the ErsatzTV web interface.');
            return;
        }

        hideChannelsNoConfig();
        channelsData = data.channels;
        updateChannelsStatus('connected', `${data.channels.length} channel(s)`);
        renderChannelsList(data.channels);

    } catch (error) {
        console.error('Failed to load channels:', error);
        updateChannelsStatus('error', 'Connection failed');
        showChannelsNoConfig('Failed to connect to ErsatzTV: ' + error.message);
    }
}

// Update status indicator
function updateChannelsStatus(status, text) {
    if (!channelsStatus) return;

    const indicator = channelsStatus.querySelector('.status-indicator');
    const textEl = channelsStatus.querySelector('.status-text');

    if (indicator) {
        indicator.className = 'status-indicator status-' + status;
    }
    if (textEl) {
        textEl.textContent = text;
    }
}

// Show no config / setup needed message
function showChannelsNoConfig(message) {
    if (channelsNoConfig) {
        channelsNoConfig.style.display = 'flex';
        const msgEl = channelsNoConfig.querySelector('p');
        if (msgEl && message) {
            msgEl.textContent = message;
        }
    }
    const container = document.querySelector('.channels-container');
    if (container) container.style.display = 'none';
}

// Hide no config message
function hideChannelsNoConfig() {
    if (channelsNoConfig) channelsNoConfig.style.display = 'none';
    const container = document.querySelector('.channels-container');
    if (container) container.style.display = 'grid';
}

// Render channels list
function renderChannelsList(channels) {
    if (!channelsList) return;

    let html = '';
    channels.forEach(channel => {
        const isActive = channelsCurrentChannel && channelsCurrentChannel.id === channel.id;
        html += `
            <div class="channels-channel-item ${isActive ? 'active' : ''}"
                 data-id="${escapeHtmlAttr(String(channel.id))}"
                 data-number="${escapeHtmlAttr(String(channel.number))}"
                 data-name="${escapeHtmlAttr(channel.name)}">
                <span class="channel-number">${escapeHtml(String(channel.number))}</span>
                <span class="channel-name">${escapeHtml(channel.name)}</span>
            </div>
        `;
    });

    channelsList.innerHTML = html;

    // Add click handlers
    channelsList.querySelectorAll('.channels-channel-item').forEach(item => {
        item.addEventListener('click', () => playChannel(item));
    });
}

// Play a channel
function playChannel(element) {
    const channelId = element.dataset.id;
    const channelNumber = element.dataset.number;
    const channelName = element.dataset.name;

    // Update active state
    channelsList.querySelectorAll('.channels-channel-item').forEach(el => el.classList.remove('active'));
    element.classList.add('active');

    // Update channel info
    channelsCurrentChannel = { id: channelId, number: channelNumber, name: channelName };
    if (channelsChannelNumber) channelsChannelNumber.textContent = channelNumber;
    if (channelsChannelName) channelsChannelName.textContent = channelName;

    // Hide placeholder
    if (channelsPlaceholder) channelsPlaceholder.classList.add('hidden');

    // Get stream URL from ErsatzTV
    // ErsatzTV uses /iptv/channel/<number>.ts for streams
    const baseUrl = channelsPlaylistUrl.replace('/iptv/channels.m3u', '');
    const streamUrl = `${baseUrl}/iptv/channel/${channelNumber}.ts`;

    channelsPlayer.src = streamUrl;
    channelsPlayer.play().catch(e => {
        console.error('Playback failed:', e);
        // Try alternative URL format
        const altUrl = `${baseUrl}/iptv/channel/${channelNumber}.m3u8`;
        channelsPlayer.src = altUrl;
        channelsPlayer.play().catch(e2 => {
            console.error('Alt playback also failed:', e2);
        });
    });

    // Update now playing info
    updateNowPlaying(channelId);
}

// Update now playing info
async function updateNowPlaying(channelId) {
    try {
        const response = await fetch(`/api/ersatztv/channels/${channelId}/guide`);
        const data = await response.json();

        if (data.guide && data.guide.length > 0) {
            // Find current program
            const now = Date.now();
            const current = data.guide.find(prog => {
                const start = new Date(prog.start).getTime();
                const stop = new Date(prog.stop).getTime();
                return start <= now && stop > now;
            });

            if (channelsNowTitle) {
                channelsNowTitle.textContent = current?.title || 'Unknown';
            }
        } else {
            if (channelsNowTitle) channelsNowTitle.textContent = '-';
        }
    } catch (error) {
        console.error('Failed to get channel guide:', error);
        if (channelsNowTitle) channelsNowTitle.textContent = '-';
    }
}

// Helper function to escape HTML
function escapeHtml(text) {
    if (!text) return '';
    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
}

// Helper function to escape HTML attributes
function escapeHtmlAttr(text) {
    if (!text) return '';
    return text.replace(/"/g, '&quot;').replace(/'/g, '&#39;');
}

// Initialize on page load
document.addEventListener('DOMContentLoaded', () => {
    initChannels();
});
