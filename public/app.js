// Application State
let currentPath = '/mnt/nfs';
let parentPath = '';
let canGoUp = false;
let currentUser = null;
let userSettings = null;
let availableAvatars = [];
let selectedAvatar = 'cat';
let currentVideoPath = null;
let currentVideoName = null;
let subtitleStatus = { configured: false };
let audioTrackStatus = { canDetect: false, canSwitch: false };
let currentAudioTracks = [];
let selectedAudioTrack = 0;
let videoDuration = null;

// DOM Elements
const fileList = document.getElementById('file-list');
const currentPathDisplay = document.getElementById('current-path');
const parentBtn = document.getElementById('parent-btn');
const refreshBtn = document.getElementById('refresh-btn');
const videoPlayer = document.getElementById('video-player');
const videoName = document.getElementById('video-name');
const videoPlaceholder = document.getElementById('video-placeholder');

const userMenuToggle = document.getElementById('user-menu-toggle');
const userPanelOverlay = document.getElementById('user-panel-overlay');
const closePanel = document.getElementById('close-panel');
const headerAvatar = document.getElementById('header-avatar');
const headerUsername = document.getElementById('header-username');
const panelAvatar = document.getElementById('panel-avatar');
const panelUsername = document.getElementById('panel-username');
const panelEmail = document.getElementById('panel-email');
const panelProvider = document.getElementById('panel-provider');
const avatarGrid = document.getElementById('avatar-grid');
const saveSettingsBtn = document.getElementById('save-settings');

// Settings elements
const qualitySelect = document.getElementById('quality-select');
const volumeSlider = document.getElementById('volume-slider');
const volumeValue = document.getElementById('volume-value');
const speedSelect = document.getElementById('speed-select');
const autoplayToggle = document.getElementById('autoplay-toggle');
const subtitlesToggle = document.getElementById('subtitles-toggle');

// Initialize
document.addEventListener('DOMContentLoaded', async () => {
    await checkAuth();
    await loadAvatars();
    setupEventListeners();
});

// Check authentication
async function checkAuth() {
    try {
        const response = await fetch('/api/user');
        const data = await response.json();

        if (!data.authenticated) {
            window.location.href = '/login.html';
            return;
        }

        currentUser = data.user;
        userSettings = data.settings;

        updateUserDisplay();
        applySettings();
        loadDirectory(currentPath);

        // Check subtitle configuration
        checkSubtitleStatus();

        // Check audio track configuration
        checkAudioTrackStatus();
    } catch (error) {
        console.error('Auth check failed:', error);
        window.location.href = '/login.html';
    }
}

// Check if subtitle search is configured
async function checkSubtitleStatus() {
    try {
        const response = await fetch('/api/subtitles/status');
        subtitleStatus = await response.json();
        console.log('Subtitle status:', subtitleStatus.message);
    } catch (error) {
        console.error('Failed to check subtitle status:', error);
        subtitleStatus = { configured: false };
    }
}

// Check if audio track switching is available
async function checkAudioTrackStatus() {
    try {
        const response = await fetch('/api/audio-tracks/status');
        audioTrackStatus = await response.json();
        console.log('Audio track status:', audioTrackStatus.message);
    } catch (error) {
        console.error('Failed to check audio track status:', error);
        audioTrackStatus = { canDetect: false, canSwitch: false };
    }
}

// Load available avatars
async function loadAvatars() {
    try {
        const response = await fetch('/api/avatars');
        availableAvatars = await response.json();
        renderAvatarGrid();
    } catch (error) {
        console.error('Failed to load avatars:', error);
    }
}

// Update user display in header and panel
function updateUserDisplay() {
    if (!currentUser) return;
    
    const avatarUrl = getAvatarUrl(userSettings?.profilePicture || 'cat');
    
    headerAvatar.src = avatarUrl;
    headerUsername.textContent = currentUser.displayName || 'User';
    
    panelAvatar.src = avatarUrl;
    panelUsername.textContent = currentUser.displayName || 'User';
    panelEmail.textContent = currentUser.email || '';
    panelProvider.textContent = `Signed in with ${capitalize(currentUser.provider)}`;
    
    selectedAvatar = userSettings?.profilePicture || 'cat';
}

// Get avatar URL
function getAvatarUrl(avatarName) {
    // Check if it's a URL (from OAuth provider)
    if (avatarName && avatarName.startsWith('http')) {
        return avatarName;
    }
    return `/images/${avatarName}.svg`;
}

// Render avatar grid
function renderAvatarGrid() {
    avatarGrid.innerHTML = '';
    
    availableAvatars.forEach(avatar => {
        const div = document.createElement('div');
        div.className = 'avatar-option';
        if (avatar === selectedAvatar) {
            div.classList.add('selected');
        }
        
        const img = document.createElement('img');
        img.src = `/images/${avatar}.svg`;
        img.alt = capitalize(avatar);
        
        div.appendChild(img);
        div.addEventListener('click', () => selectAvatar(avatar));
        avatarGrid.appendChild(div);
    });
}

// Select avatar
function selectAvatar(avatar) {
    selectedAvatar = avatar;
    document.querySelectorAll('.avatar-option').forEach(el => {
        el.classList.remove('selected');
    });
    document.querySelector(`.avatar-option img[alt="${capitalize(avatar)}"]`)
        ?.parentElement?.classList.add('selected');
}

// Apply settings to UI
function applySettings() {
    if (!userSettings?.streaming) return;
    
    const s = userSettings.streaming;
    
    qualitySelect.value = s.quality || 'auto';
    volumeSlider.value = s.volume || 80;
    volumeValue.textContent = `${s.volume || 80}%`;
    speedSelect.value = s.playbackSpeed || 1;
    autoplayToggle.checked = s.autoplay || false;
    subtitlesToggle.checked = s.subtitles || false;
    
    // Apply to video player
    videoPlayer.volume = (s.volume || 80) / 100;
    videoPlayer.playbackRate = s.playbackSpeed || 1;
}

// Setup event listeners
function setupEventListeners() {
    // Navigation
    parentBtn.addEventListener('click', () => {
        if (canGoUp && parentPath) {
            loadDirectory(parentPath);
        }
    });
    
    refreshBtn.addEventListener('click', () => {
        loadDirectory(currentPath);
    });
    
    // User panel
    userMenuToggle.addEventListener('click', () => {
        userPanelOverlay.classList.add('active');
        userMenuToggle.classList.add('active');
    });
    
    closePanel.addEventListener('click', closeUserPanel);
    
    userPanelOverlay.addEventListener('click', (e) => {
        if (e.target === userPanelOverlay) {
            closeUserPanel();
        }
    });
    
    // Settings
    volumeSlider.addEventListener('input', () => {
        volumeValue.textContent = `${volumeSlider.value}%`;
    });
    
    saveSettingsBtn.addEventListener('click', saveSettings);
    
    // Keyboard
    document.addEventListener('keydown', (e) => {
        if (e.key === 'Escape') {
            closeUserPanel();
        }
    });
}

// Close user panel
function closeUserPanel() {
    userPanelOverlay.classList.remove('active');
    userMenuToggle.classList.remove('active');
}

// Save settings
async function saveSettings() {
    const settings = {
        streaming: {
            quality: qualitySelect.value,
            volume: parseInt(volumeSlider.value),
            playbackSpeed: parseFloat(speedSelect.value),
            autoplay: autoplayToggle.checked,
            subtitles: subtitlesToggle.checked
        },
        profilePicture: selectedAvatar
    };
    
    try {
        const response = await fetch('/api/settings', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(settings)
        });
        
        const data = await response.json();
        
        if (data.success) {
            userSettings = data.settings;
            applySettings();
            updateUserDisplay();
            
            // Visual feedback
            saveSettingsBtn.textContent = 'Saved!';
            setTimeout(() => {
                saveSettingsBtn.textContent = 'Save Settings';
            }, 2000);
        }
    } catch (error) {
        console.error('Failed to save settings:', error);
        alert('Failed to save settings');
    }
}

// Load directory
async function loadDirectory(path) {
    try {
        fileList.innerHTML = `
            <div class="loading-state">
                <div class="spinner"></div>
                <p>Loading files...</p>
            </div>
        `;
        
        const response = await fetch(`/api/browse?path=${encodeURIComponent(path)}`);
        const data = await response.json();
        
        if (response.ok) {
            currentPath = data.currentPath;
            parentPath = data.parentPath;
            canGoUp = data.canGoUp;
            currentPathDisplay.textContent = currentPath;
            
            parentBtn.disabled = !canGoUp;
            renderFileList(data.items);
        } else {
            throw new Error(data.error || 'Failed to load directory');
        }
    } catch (error) {
        fileList.innerHTML = `
            <div class="error-state">
                <p>Error: ${error.message}</p>
            </div>
        `;
    }
}

// Render file list
function renderFileList(items) {
    fileList.innerHTML = '';
    
    if (items.length === 0) {
        fileList.innerHTML = `
            <div class="empty-state">
                <p>This directory is empty</p>
            </div>
        `;
        return;
    }
    
    items.forEach(item => {
        const div = document.createElement('div');
        div.className = 'file-item';
        
        if (item.isDirectory) {
            div.classList.add('directory');
            div.innerHTML = `
                <div class="file-icon">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                        <path d="M22 19a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5l2 3h9a2 2 0 0 1 2 2z"></path>
                    </svg>
                </div>
                <div class="file-info">
                    <div class="file-name">${escapeHtml(item.name)}</div>
                    <div class="file-meta">Folder</div>
                </div>
            `;
            div.addEventListener('click', () => loadDirectory(item.path));
        } else if (item.isVideo) {
            div.classList.add('video');
            div.innerHTML = `
                <div class="file-icon">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                        <polygon points="5 3 19 12 5 21 5 3"></polygon>
                    </svg>
                </div>
                <div class="file-info">
                    <div class="file-name">${escapeHtml(item.name)}</div>
                    <div class="file-meta">${formatFileSize(item.size)}</div>
                </div>
            `;
            div.addEventListener('click', () => playVideo(item.path, item.name));
        } else {
            div.classList.add('other');
            div.innerHTML = `
                <div class="file-icon">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                        <path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"></path>
                        <polyline points="14 2 14 8 20 8"></polyline>
                    </svg>
                </div>
                <div class="file-info">
                    <div class="file-name">${escapeHtml(item.name)}</div>
                    <div class="file-meta">${formatFileSize(item.size)}</div>
                </div>
            `;
        }
        
        fileList.appendChild(div);
    });
}

// Play video
async function playVideo(path, name) {
    const videoUrl = `/api/video?path=${encodeURIComponent(path)}`;

    // Store current video path for subtitle and audio operations
    currentVideoPath = path;
    currentVideoName = name;
    selectedAudioTrack = 0;
    currentAudioTracks = [];
    videoDuration = null;

    // Remove any existing subtitle tracks
    removeSubtitleTracks();

    // Hide custom time display until duration is loaded
    hideCustomTimeDisplay();

    // Hide audio selector while loading
    hideAudioTrackSelector();

    videoPlayer.src = videoUrl;
    videoName.textContent = name;

    videoPlaceholder.classList.add('hidden');
    videoPlayer.classList.add('active');

    // Apply user settings
    if (userSettings?.streaming) {
        videoPlayer.volume = userSettings.streaming.volume / 100;
        videoPlayer.playbackRate = userSettings.streaming.playbackSpeed;
    }

    videoPlayer.play().catch(e => console.log('Autoplay prevented:', e));

    // Mark active item
    document.querySelectorAll('.file-item').forEach(el => el.classList.remove('active'));
    event.currentTarget.classList.add('active');

    // Scroll to player on mobile
    if (window.innerWidth <= 1200) {
        document.getElementById('player-section').scrollIntoView({ behavior: 'smooth' });
    }

    // Search and load subtitles if enabled
    if (userSettings?.streaming?.subtitles && subtitleStatus.configured) {
        await loadSubtitles(path);
    }

    // Load audio tracks if available
    if (audioTrackStatus.canDetect) {
        await loadAudioTracks(path);
    }
}

// Load audio tracks for the current video
async function loadAudioTracks(videoPath) {
    try {
        const response = await fetch(`/api/audio-tracks?path=${encodeURIComponent(videoPath)}`);
        const data = await response.json();

        // Store video duration for custom time display
        if (data.duration) {
            videoDuration = data.duration;
            setupCustomTimeDisplay();
        }

        if (data.tracks && data.tracks.length > 1) {
            currentAudioTracks = data.tracks;
            showAudioTrackSelector(data.tracks);
            console.log(`Found ${data.tracks.length} audio tracks`);
        } else {
            hideAudioTrackSelector();
            currentAudioTracks = data.tracks || [];
        }
    } catch (error) {
        console.error('Failed to load audio tracks:', error);
        hideAudioTrackSelector();
    }
}

// Show the audio track selector UI
function showAudioTrackSelector(tracks) {
    let container = document.getElementById('audio-track-container');

    if (!container) {
        container = document.createElement('div');
        container.id = 'audio-track-container';
        container.className = 'audio-track-container';
        document.querySelector('.video-header').appendChild(container);
    }

    container.innerHTML = `
        <div class="audio-track-selector">
            <label for="audio-track-select">
                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                    <path d="M12 1a3 3 0 0 0-3 3v8a3 3 0 0 0 6 0V4a3 3 0 0 0-3-3z"></path>
                    <path d="M19 10v2a7 7 0 0 1-14 0v-2"></path>
                    <line x1="12" y1="19" x2="12" y2="23"></line>
                    <line x1="8" y1="23" x2="16" y2="23"></line>
                </svg>
                Audio
            </label>
            <select id="audio-track-select">
                ${tracks.map((track, index) => `
                    <option value="${index}" ${track.default ? 'selected' : ''}>
                        ${escapeHtml(track.label)}
                    </option>
                `).join('')}
            </select>
        </div>
    `;

    container.style.display = 'flex';

    // Add event listener for track selection
    const select = document.getElementById('audio-track-select');
    select.addEventListener('change', handleAudioTrackChange);

    // Set initial selection to default track
    const defaultTrack = tracks.findIndex(t => t.default);
    if (defaultTrack >= 0) {
        selectedAudioTrack = defaultTrack;
        select.value = defaultTrack;
    }
}

// Hide the audio track selector
function hideAudioTrackSelector() {
    const container = document.getElementById('audio-track-container');
    if (container) {
        container.style.display = 'none';
    }
}

// Handle audio track change
async function handleAudioTrackChange(event) {
    const newTrack = parseInt(event.target.value, 10);

    if (newTrack === selectedAudioTrack) {
        return;
    }

    if (!audioTrackStatus.canSwitch) {
        // Show message that switching is not available
        showAudioTrackMessage('FFmpeg required for audio switching', 'error');
        event.target.value = selectedAudioTrack;
        return;
    }

    // Store current playback state
    const currentTime = videoPlayer.currentTime;
    const wasPlaying = !videoPlayer.paused;
    const volume = videoPlayer.volume;
    const playbackRate = videoPlayer.playbackRate;

    // Show loading indicator
    showAudioTrackMessage('Switching audio track...', 'loading');

    // Update selected track
    selectedAudioTrack = newTrack;

    // Create new video URL with selected audio track
    const videoUrl = `/api/video-stream?path=${encodeURIComponent(currentVideoPath)}&audio=${newTrack}&start=${currentTime}`;

    // Remove subtitle tracks (they'll need to be re-added)
    removeSubtitleTracks();

    // Load new stream
    videoPlayer.src = videoUrl;

    // Restore settings
    videoPlayer.volume = volume;
    videoPlayer.playbackRate = playbackRate;

    // Wait for video to be ready
    videoPlayer.onloadeddata = async () => {
        showAudioTrackMessage(`Audio: ${currentAudioTracks[newTrack]?.label || 'Track ' + (newTrack + 1)}`, 'success');

        if (wasPlaying) {
            videoPlayer.play().catch(e => console.log('Autoplay prevented:', e));
        }

        // Reload subtitles if enabled
        if (userSettings?.streaming?.subtitles && subtitleStatus.configured) {
            await loadSubtitles(currentVideoPath);
        }
    };

    videoPlayer.onerror = () => {
        showAudioTrackMessage('Failed to switch audio track', 'error');
        // Revert to original stream
        selectedAudioTrack = 0;
        event.target.value = 0;
        videoPlayer.src = `/api/video?path=${encodeURIComponent(currentVideoPath)}`;
    };
}

// Show audio track status message
function showAudioTrackMessage(message, type) {
    let indicator = document.getElementById('audio-track-indicator');

    if (!indicator) {
        indicator = document.createElement('div');
        indicator.id = 'audio-track-indicator';
        indicator.className = 'audio-track-indicator';
        document.querySelector('.video-header').appendChild(indicator);
    }

    indicator.className = `audio-track-indicator ${type}`;

    let icon = '';
    switch (type) {
        case 'loading':
            icon = `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" class="spin">
                <circle cx="12" cy="12" r="10"></circle>
                <path d="M12 6v6l4 2"></path>
            </svg>`;
            break;
        case 'success':
            icon = `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <path d="M22 11.08V12a10 10 0 1 1-5.93-9.14"></path>
                <polyline points="22 4 12 14.01 9 11.01"></polyline>
            </svg>`;
            break;
        case 'error':
            icon = `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <circle cx="12" cy="12" r="10"></circle>
                <line x1="12" y1="8" x2="12" y2="12"></line>
                <line x1="12" y1="16" x2="12.01" y2="16"></line>
            </svg>`;
            break;
    }

    indicator.innerHTML = `${icon}<span>${message}</span>`;
    indicator.style.display = 'flex';

    // Auto-hide after a delay (except for loading)
    if (type !== 'loading') {
        setTimeout(() => {
            indicator.style.display = 'none';
        }, 3000);
    }
}

// Remove all existing subtitle tracks from video player
function removeSubtitleTracks() {
    const tracks = videoPlayer.querySelectorAll('track');
    tracks.forEach(track => track.remove());

    // Also update subtitle indicator
    updateSubtitleIndicator(null);
}

// Load subtitles for the current video
async function loadSubtitles(videoPath) {
    updateSubtitleIndicator('searching');

    try {
        const response = await fetch(`/api/subtitles/search?path=${encodeURIComponent(videoPath)}`);
        const data = await response.json();

        if (data.found && data.subtitleUrl) {
            // Create and add subtitle track
            const track = document.createElement('track');
            track.kind = 'subtitles';
            track.label = 'English';
            track.srclang = 'en';
            track.src = data.subtitleUrl;
            track.default = true;

            videoPlayer.appendChild(track);

            // Enable the track
            if (videoPlayer.textTracks.length > 0) {
                videoPlayer.textTracks[0].mode = 'showing';
            }

            updateSubtitleIndicator('loaded', data.cached ? 'Cached' : 'Downloaded');
            console.log('Subtitles loaded:', data.metadata?.filename || 'from cache');
        } else {
            updateSubtitleIndicator('not_found', data.message || 'No subtitles found');
            console.log('No subtitles found:', data.message);
        }
    } catch (error) {
        console.error('Failed to load subtitles:', error);
        updateSubtitleIndicator('error', 'Failed to load subtitles');
    }
}

// Update the subtitle status indicator
function updateSubtitleIndicator(status, message) {
    let indicator = document.getElementById('subtitle-indicator');

    if (!indicator) {
        // Create indicator if it doesn't exist
        indicator = document.createElement('div');
        indicator.id = 'subtitle-indicator';
        indicator.className = 'subtitle-indicator';
        document.querySelector('.video-header').appendChild(indicator);
    }

    switch (status) {
        case 'searching':
            indicator.className = 'subtitle-indicator searching';
            indicator.innerHTML = `
                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" class="spin">
                    <circle cx="12" cy="12" r="10"></circle>
                    <path d="M12 6v6l4 2"></path>
                </svg>
                <span>Searching for subtitles...</span>
            `;
            indicator.style.display = 'flex';
            break;
        case 'loaded':
            indicator.className = 'subtitle-indicator loaded';
            indicator.innerHTML = `
                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                    <path d="M22 11.08V12a10 10 0 1 1-5.93-9.14"></path>
                    <polyline points="22 4 12 14.01 9 11.01"></polyline>
                </svg>
                <span>Subtitles ${message || 'loaded'}</span>
            `;
            indicator.style.display = 'flex';
            // Auto-hide after 3 seconds
            setTimeout(() => {
                indicator.style.display = 'none';
            }, 3000);
            break;
        case 'not_found':
            indicator.className = 'subtitle-indicator not-found';
            indicator.innerHTML = `
                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                    <circle cx="12" cy="12" r="10"></circle>
                    <line x1="15" y1="9" x2="9" y2="15"></line>
                    <line x1="9" y1="9" x2="15" y2="15"></line>
                </svg>
                <span>${message || 'No subtitles found'}</span>
            `;
            indicator.style.display = 'flex';
            // Auto-hide after 4 seconds
            setTimeout(() => {
                indicator.style.display = 'none';
            }, 4000);
            break;
        case 'error':
            indicator.className = 'subtitle-indicator error';
            indicator.innerHTML = `
                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                    <circle cx="12" cy="12" r="10"></circle>
                    <line x1="12" y1="8" x2="12" y2="12"></line>
                    <line x1="12" y1="16" x2="12.01" y2="16"></line>
                </svg>
                <span>${message || 'Error'}</span>
            `;
            indicator.style.display = 'flex';
            // Auto-hide after 4 seconds
            setTimeout(() => {
                indicator.style.display = 'none';
            }, 4000);
            break;
        default:
            indicator.style.display = 'none';
    }
}

// Utility functions
function formatFileSize(bytes) {
    if (bytes === 0) return '0 Bytes';
    const k = 1024;
    const sizes = ['Bytes', 'KB', 'MB', 'GB', 'TB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
}

function capitalize(str) {
    return str ? str.charAt(0).toUpperCase() + str.slice(1) : '';
}

function escapeHtml(text) {
    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
}

// Format seconds to HH:MM:SS or MM:SS
function formatTime(seconds) {
    if (isNaN(seconds) || seconds < 0) return '0:00';
    const hrs = Math.floor(seconds / 3600);
    const mins = Math.floor((seconds % 3600) / 60);
    const secs = Math.floor(seconds % 60);
    if (hrs > 0) {
        return `${hrs}:${mins.toString().padStart(2, '0')}:${secs.toString().padStart(2, '0')}`;
    }
    return `${mins}:${secs.toString().padStart(2, '0')}`;
}

// Setup custom time display overlay
function setupCustomTimeDisplay() {
    if (!videoDuration) return;
    let timeDisplay = document.getElementById('custom-time-display');
    if (!timeDisplay) {
        timeDisplay = document.createElement('div');
        timeDisplay.id = 'custom-time-display';
        timeDisplay.className = 'custom-time-display';
        document.querySelector('.video-container').appendChild(timeDisplay);
    }
    timeDisplay.style.display = 'block';
    updateTimeDisplay();
    videoPlayer.removeEventListener('timeupdate', updateTimeDisplay);
    videoPlayer.addEventListener('timeupdate', updateTimeDisplay);
}

// Update the custom time display
function updateTimeDisplay() {
    const timeDisplay = document.getElementById('custom-time-display');
    if (!timeDisplay || !videoDuration) return;
    const currentTime = videoPlayer.currentTime || 0;
    timeDisplay.textContent = `${formatTime(currentTime)} / ${formatTime(videoDuration)}`;
}

// Hide custom time display
function hideCustomTimeDisplay() {
    const timeDisplay = document.getElementById('custom-time-display');
    if (timeDisplay) {
        timeDisplay.style.display = 'none';
    }
    videoDuration = null;
}
