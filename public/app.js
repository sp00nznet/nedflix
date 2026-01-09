// Application State
let currentPath = '/mnt/nfs';
let parentPath = '';
let canGoUp = false;
let currentUser = null;
let userSettings = null;
let availableAvatars = [];
let selectedAvatar = 'cat';
let currentVideoPath = null;
let subtitleStatus = { configured: false };

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

    // Store current video path for subtitle operations
    currentVideoPath = path;

    // Remove any existing subtitle tracks
    removeSubtitleTracks();

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
