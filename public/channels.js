// ==================== CHANNELS (ErsatzTV Auto-Channels) ====================

// Channels elements
const channelsView = document.getElementById('channels-view');
const channelsCard = document.getElementById('channels-card');
const channelsBackBtn = document.getElementById('channels-back-btn');
const channelsRefreshBtn = document.getElementById('channels-refresh-btn');
const channelsList = document.getElementById('channels-list');
const channelsPlayer = document.getElementById('channels-player');
const channelsPlaceholder = document.getElementById('channels-placeholder');
const channelsNoConfig = document.getElementById('channels-no-config');
const channelsLoading = document.getElementById('channels-loading');
const channelsStatus = document.getElementById('channels-status');
const channelsChannelNumber = document.getElementById('channels-channel-number');
const channelsChannelName = document.getElementById('channels-channel-name');
const channelsNowTitle = document.getElementById('channels-now-title');
const channelsSetupBtn = document.getElementById('channels-setup-btn');
const channelsSetupBtnMain = document.getElementById('channels-setup-btn-main');
const channelsAdminActions = document.getElementById('channels-admin-actions');

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

    // Setup buttons
    if (channelsSetupBtn) {
        channelsSetupBtn.addEventListener('click', setupAutoChannels);
    }
    if (channelsSetupBtnMain) {
        channelsSetupBtnMain.addEventListener('click', setupAutoChannels);
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

    // Show admin actions if user is admin
    updateAdminVisibility();

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

// Update visibility of admin-only elements
function updateAdminVisibility() {
    // Check if user is admin (from global currentUser variable in app.js)
    const isAdmin = typeof currentUser !== 'undefined' && currentUser?.isAdmin;

    if (channelsAdminActions) {
        channelsAdminActions.style.display = isAdmin ? 'block' : 'none';
    }

    // Show/hide other admin elements
    document.querySelectorAll('#channels-view .admin-only').forEach(el => {
        el.style.display = isAdmin ? 'block' : 'none';
    });
}

// Load channels from ErsatzTV
async function loadChannels() {
    try {
        updateChannelsStatus('loading', 'Checking ErsatzTV...');

        const response = await fetch('/api/ersatztv/status');
        const data = await response.json();

        ersatztvAvailable = data.available;

        if (!data.available) {
            updateChannelsStatus('error', 'ErsatzTV unavailable');
            showChannelsNoConfig('ErsatzTV service is not available. Please check your Docker configuration.');
            return;
        }

        channelsPlaylistUrl = data.playlistUrl;
        channelsEpgUrl = data.epgUrl;

        if (!data.channels || data.channels.length === 0) {
            updateChannelsStatus('warning', 'No channels configured');
            showChannelsNoConfig('No channels have been created yet.');
            return;
        }

        hideChannelsNoConfig();
        channelsData = data.channels;
        updateChannelsStatus('connected', `${data.channels.length} channel(s)`);
        renderChannelsList(data.channels);

    } catch (error) {
        console.error('Failed to load channels:', error);
        updateChannelsStatus('error', 'Connection failed');
        showChannelsNoConfig('Failed to connect to ErsatzTV.');
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
        const msgEl = channelsNoConfig.querySelector('p:not(.admin-only)');
        if (msgEl && message) {
            msgEl.textContent = message;
        }
    }
    const container = document.querySelector('.channels-container');
    if (container) container.style.display = 'none';
    if (channelsLoading) channelsLoading.style.display = 'none';
}

// Hide no config message
function hideChannelsNoConfig() {
    if (channelsNoConfig) channelsNoConfig.style.display = 'none';
    const container = document.querySelector('.channels-container');
    if (container) container.style.display = 'grid';
    if (channelsLoading) channelsLoading.style.display = 'none';
}

// Show loading state
function showChannelsLoading() {
    if (channelsLoading) channelsLoading.style.display = 'flex';
    if (channelsNoConfig) channelsNoConfig.style.display = 'none';
    const container = document.querySelector('.channels-container');
    if (container) container.style.display = 'none';
}

// Render channels list
function renderChannelsList(channels) {
    if (!channelsList) return;

    let html = '';
    channels.forEach(channel => {
        const isActive = channelsCurrentChannel && channelsCurrentChannel.id === channel.id;
        html += `
            <div class="channels-channel-item ${isActive ? 'active' : ''}"
                 data-id="${escapeHtml(String(channel.id))}"
                 data-number="${escapeHtml(String(channel.number))}"
                 data-name="${escapeHtml(channel.name)}">
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

    // Hide placeholder, show player
    if (channelsPlaceholder) channelsPlaceholder.classList.add('hidden');

    // Get stream URL from ErsatzTV
    // ErsatzTV uses /iptv/channel/<number>.ts for HLS streams
    const streamUrl = `${channelsPlaylistUrl.replace('/channels.m3u', '')}/channel/${channelNumber}.ts`;

    channelsPlayer.src = streamUrl;
    channelsPlayer.play().catch(e => {
        console.error('Playback failed:', e);
        // Try alternative URL format
        const altUrl = `${channelsPlaylistUrl.replace('/channels.m3u', '')}/iptv/channel/${channelNumber}.ts`;
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

// Setup auto-channels from NFS media
async function setupAutoChannels() {
    try {
        showChannelsLoading();
        updateChannelsStatus('loading', 'Setting up channels...');

        const response = await fetch('/api/ersatztv/setup', {
            method: 'POST'
        });

        const result = await response.json();

        if (result.success) {
            // Show success message briefly
            updateChannelsStatus('connected', 'Setup complete!');

            // Reload channels
            await loadChannels();
        } else {
            updateChannelsStatus('error', 'Setup failed');
            showChannelsNoConfig(result.error || 'Failed to setup auto-channels.');
        }

    } catch (error) {
        console.error('Auto-channel setup failed:', error);
        updateChannelsStatus('error', 'Setup failed');
        showChannelsNoConfig('Failed to setup auto-channels: ' + error.message);
    }
}

// Helper function to escape HTML (if not already defined)
if (typeof escapeHtml === 'undefined') {
    function escapeHtml(text) {
        const div = document.createElement('div');
        div.textContent = text;
        return div.innerHTML;
    }
}

// Initialize on page load
document.addEventListener('DOMContentLoaded', () => {
    initChannels();
});
