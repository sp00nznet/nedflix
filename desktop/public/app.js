/**
 * Nedflix Desktop - Frontend Application
 * Simplified version without authentication
 */

// State
let currentPath = '';
let libraries = [];
let mediaPaths = [];
let settings = {
    theme: 'dark',
    streaming: {
        quality: 'auto',
        volume: 80,
        playbackSpeed: 1,
        autoplay: false
    },
    controller: {
        vibration: true
    },
    iptv: {
        playlistUrl: '',
        epgUrl: ''
    }
};

// DOM Elements
const librarySelector = document.getElementById('library-selector');
const libraryCards = document.getElementById('library-cards');
const mainContent = document.getElementById('main-content');
const fileList = document.getElementById('file-list');
const currentPathEl = document.getElementById('current-path');
const parentBtn = document.getElementById('parent-btn');
const refreshBtn = document.getElementById('refresh-btn');
const browserToggle = document.getElementById('browser-toggle');
const videoPlayer = document.getElementById('video-player');
const videoName = document.getElementById('video-name');
const videoPlaceholder = document.getElementById('video-placeholder');
const userMenuToggle = document.getElementById('user-menu-toggle');
const userPanelOverlay = document.getElementById('user-panel-overlay');
const closePanel = document.getElementById('close-panel');
const saveSettingsBtn = document.getElementById('save-settings');

// Initialize
document.addEventListener('DOMContentLoaded', init);

async function init() {
    loadSettings();
    applySettings();
    await loadLibraries();
    setupEventListeners();
    setupMediaPaths();
}

// Load libraries from API
async function loadLibraries() {
    try {
        const response = await fetch('/api/libraries');
        libraries = await response.json();
        renderLibraries();
    } catch (error) {
        console.error('Failed to load libraries:', error);
        libraryCards.innerHTML = '<p class="error-state">Failed to load media libraries</p>';
    }
}

// Render library cards
function renderLibraries() {
    libraryCards.innerHTML = '';

    libraries.forEach(lib => {
        const card = document.createElement('button');
        card.className = 'library-card';
        card.dataset.path = lib.path;
        card.innerHTML = `
            <div class="library-icon">
                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5">
                    <path d="M22 19a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5l2 3h9a2 2 0 0 1 2 2z"></path>
                </svg>
            </div>
            <span class="library-name">${escapeHtml(lib.name)}</span>
        `;
        card.addEventListener('click', () => openLibrary(lib.path));
        libraryCards.appendChild(card);
    });
}

// Setup media paths display in settings
async function setupMediaPaths() {
    const mediaPathsList = document.getElementById('media-paths-list');
    const addPathBtn = document.getElementById('add-media-path-btn');
    const pathInput = document.getElementById('media-path-input');

    if (!mediaPathsList) return;

    // Load paths from Electron IPC
    if (window.nedflixDesktop) {
        mediaPaths = await window.nedflixDesktop.getMediaPaths();
    } else {
        mediaPaths = libraries.map(l => l.path);
    }

    renderMediaPaths();

    // Add path button handler
    if (addPathBtn && pathInput) {
        addPathBtn.addEventListener('click', () => addMediaPath());
        pathInput.addEventListener('keypress', (e) => {
            if (e.key === 'Enter') {
                e.preventDefault();
                addMediaPath();
            }
        });
    }
}

// Render media paths list
function renderMediaPaths() {
    const mediaPathsList = document.getElementById('media-paths-list');
    if (!mediaPathsList) return;

    if (mediaPaths.length === 0) {
        mediaPathsList.innerHTML = '<div class="media-paths-empty">No media paths configured. Add a folder path above.</div>';
        return;
    }

    mediaPathsList.innerHTML = mediaPaths.map((path, index) => `
        <div class="media-path-item" data-index="${index}">
            <div class="path-icon">
                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                    <path d="M22 19a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5l2 3h9a2 2 0 0 1 2 2z"></path>
                </svg>
            </div>
            <span class="path-text">${escapeHtml(path)}</span>
            <button type="button" class="remove-path-btn" data-index="${index}" title="Remove path">
                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                    <line x1="18" y1="6" x2="6" y2="18"></line>
                    <line x1="6" y1="6" x2="18" y2="18"></line>
                </svg>
            </button>
        </div>
    `).join('');

    // Add remove button handlers
    mediaPathsList.querySelectorAll('.remove-path-btn').forEach(btn => {
        btn.addEventListener('click', (e) => {
            e.stopPropagation();
            const index = parseInt(btn.dataset.index);
            removeMediaPath(index);
        });
    });
}

// Add a new media path
async function addMediaPath() {
    const pathInput = document.getElementById('media-path-input');
    if (!pathInput) return;

    const newPath = pathInput.value.trim();
    if (!newPath) return;

    // Check for duplicates
    if (mediaPaths.includes(newPath)) {
        pathInput.value = '';
        return;
    }

    mediaPaths.push(newPath);
    pathInput.value = '';
    renderMediaPaths();

    // Save to Electron
    if (window.nedflixDesktop) {
        await window.nedflixDesktop.setMediaPaths(mediaPaths);
    }

    // Refresh libraries
    await loadLibraries();
}

// Remove a media path
async function removeMediaPath(index) {
    if (index < 0 || index >= mediaPaths.length) return;

    mediaPaths.splice(index, 1);
    renderMediaPaths();

    // Save to Electron
    if (window.nedflixDesktop) {
        await window.nedflixDesktop.setMediaPaths(mediaPaths);
    }

    // Refresh libraries
    await loadLibraries();
}

// Open a library
function openLibrary(path) {
    currentPath = path;
    librarySelector.style.display = 'none';
    mainContent.style.display = 'grid';
    loadDirectory(path);
}

// Back to library selector
function backToLibraries() {
    librarySelector.style.display = 'flex';
    mainContent.style.display = 'none';
    mainContent.classList.remove('video-playing');
    videoPlayer.pause();
    videoPlayer.src = '';
    videoPlayer.classList.remove('active');
    videoPlaceholder.classList.remove('hidden');
}

// Load directory contents
async function loadDirectory(path) {
    fileList.innerHTML = `
        <div class="loading-state">
            <div class="spinner"></div>
            <p>Loading files...</p>
        </div>
    `;
    currentPathEl.textContent = path;
    parentBtn.disabled = true;

    try {
        const response = await fetch(`/api/browse?path=${encodeURIComponent(path)}`);
        const data = await response.json();

        if (!response.ok) {
            throw new Error(data.error || 'Failed to load directory');
        }

        currentPath = data.currentPath;
        currentPathEl.textContent = currentPath;
        parentBtn.disabled = !data.canGoUp;

        renderFileList(data.items, data.parentPath);
    } catch (error) {
        console.error('Failed to load directory:', error);
        fileList.innerHTML = `<div class="error-state">Error: ${escapeHtml(error.message)}</div>`;
    }
}

// Render file list
function renderFileList(items, parentPath) {
    if (items.length === 0) {
        fileList.innerHTML = '<div class="empty-state">This folder is empty</div>';
        return;
    }

    fileList.innerHTML = '';

    items.forEach((item, index) => {
        const div = document.createElement('div');
        let itemClass = 'file-item';

        if (item.isDirectory) {
            itemClass += ' directory';
        } else if (item.isVideo) {
            itemClass += ' video';
        } else if (item.isAudio) {
            itemClass += ' audio';
        } else {
            itemClass += ' other';
        }

        div.className = itemClass;
        div.dataset.path = item.path;
        div.dataset.index = index;
        div.tabIndex = 0;

        const icon = getFileIcon(item);
        const size = item.isDirectory ? '' : formatSize(item.size);

        div.innerHTML = `
            <div class="file-icon">${icon}</div>
            <div class="file-info">
                <div class="file-name">${escapeHtml(item.name)}</div>
                <div class="file-meta">${size}</div>
            </div>
        `;

        div.addEventListener('click', () => handleFileClick(item));
        div.addEventListener('keydown', (e) => {
            if (e.key === 'Enter' || e.key === ' ') {
                e.preventDefault();
                handleFileClick(item);
            }
        });

        fileList.appendChild(div);
    });
}

// Get file icon SVG
function getFileIcon(item) {
    if (item.isDirectory) {
        return `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
            <path d="M22 19a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5l2 3h9a2 2 0 0 1 2 2z"></path>
        </svg>`;
    } else if (item.isVideo) {
        return `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
            <polygon points="5 3 19 12 5 21 5 3"></polygon>
        </svg>`;
    } else if (item.isAudio) {
        return `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
            <path d="M9 18V5l12-2v13"></path>
            <circle cx="6" cy="18" r="3"></circle>
            <circle cx="18" cy="16" r="3"></circle>
        </svg>`;
    } else {
        return `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
            <path d="M13 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V9z"></path>
            <polyline points="13 2 13 9 20 9"></polyline>
        </svg>`;
    }
}

// Handle file click
function handleFileClick(item) {
    if (item.isDirectory) {
        loadDirectory(item.path);
    } else if (item.isVideo) {
        playVideo(item);
    } else if (item.isAudio) {
        playAudio(item);
    }
}

// Play video
function playVideo(item) {
    videoName.textContent = item.name;
    videoPlayer.src = `/api/video?path=${encodeURIComponent(item.path)}`;
    videoPlayer.classList.add('active');
    videoPlaceholder.classList.add('hidden');
    mainContent.classList.add('video-playing');

    // Apply settings
    videoPlayer.volume = settings.streaming.volume / 100;
    videoPlayer.playbackRate = settings.streaming.playbackSpeed;

    videoPlayer.play().catch(err => {
        console.error('Failed to play video:', err);
    });

    // Controller vibration feedback
    if (settings.controller.vibration && window.nedflixDesktop) {
        window.nedflixDesktop.gamepad.vibrate(0, 100, 0.3, 0.3);
    }
}

// Play audio
function playAudio(item) {
    videoName.textContent = item.name;
    videoPlayer.src = `/api/audio?path=${encodeURIComponent(item.path)}`;
    videoPlayer.classList.add('active');
    videoPlaceholder.classList.add('hidden');
    mainContent.classList.add('video-playing');

    // Apply settings
    videoPlayer.volume = settings.streaming.volume / 100;
    videoPlayer.playbackRate = settings.streaming.playbackSpeed;

    videoPlayer.play().catch(err => {
        console.error('Failed to play audio:', err);
    });
}

// Setup event listeners
function setupEventListeners() {
    // Navigation
    parentBtn.addEventListener('click', () => {
        const parentPath = currentPath.split(/[/\\]/).slice(0, -1).join('/') || '/';
        // Check if we should go back to library selector
        const isAtLibraryRoot = libraries.some(lib => lib.path === currentPath);
        if (isAtLibraryRoot) {
            backToLibraries();
        } else {
            loadDirectory(parentPath);
        }
    });

    refreshBtn.addEventListener('click', () => loadDirectory(currentPath));

    // Browser toggle
    browserToggle.addEventListener('click', () => {
        mainContent.classList.remove('video-playing');
    });

    // Settings panel
    userMenuToggle.addEventListener('click', () => {
        userPanelOverlay.classList.add('active');
    });

    closePanel.addEventListener('click', () => {
        userPanelOverlay.classList.remove('active');
    });

    userPanelOverlay.addEventListener('click', (e) => {
        if (e.target === userPanelOverlay) {
            userPanelOverlay.classList.remove('active');
        }
    });

    // Save settings
    saveSettingsBtn.addEventListener('click', saveSettings);

    // Settings inputs
    document.getElementById('theme-select')?.addEventListener('change', (e) => {
        settings.theme = e.target.value;
        applyTheme(settings.theme);
    });

    document.getElementById('volume-slider')?.addEventListener('input', (e) => {
        const value = parseInt(e.target.value);
        document.getElementById('volume-value').textContent = `${value}%`;
        settings.streaming.volume = value;
        videoPlayer.volume = value / 100;
    });

    document.getElementById('quality-select')?.addEventListener('change', (e) => {
        settings.streaming.quality = e.target.value;
    });

    document.getElementById('speed-select')?.addEventListener('change', (e) => {
        const speed = parseFloat(e.target.value);
        settings.streaming.playbackSpeed = speed;
        videoPlayer.playbackRate = speed;
    });

    document.getElementById('autoplay-toggle')?.addEventListener('change', (e) => {
        settings.streaming.autoplay = e.target.checked;
    });

    document.getElementById('controller-vibration-toggle')?.addEventListener('change', (e) => {
        settings.controller.vibration = e.target.checked;
    });

    // IPTV settings inputs
    document.getElementById('iptv-playlist-url')?.addEventListener('input', (e) => {
        settings.iptv.playlistUrl = e.target.value;
    });

    document.getElementById('iptv-epg-url')?.addEventListener('input', (e) => {
        settings.iptv.epgUrl = e.target.value;
    });

    // Keyboard shortcuts
    document.addEventListener('keydown', handleKeyboard);

    // Video events
    videoPlayer.addEventListener('ended', handleVideoEnded);

    // Fullscreen change
    if (window.nedflixDesktop) {
        window.nedflixDesktop.onFullscreenChange((isFullscreen) => {
            document.body.classList.toggle('fullscreen', isFullscreen);
        });

        // Media keys
        window.nedflixDesktop.onMediaKey((key) => {
            handleMediaKey(key);
        });
    }
}

// Handle keyboard shortcuts
function handleKeyboard(e) {
    // Don't handle if typing in input
    if (e.target.tagName === 'INPUT' || e.target.tagName === 'SELECT') {
        return;
    }

    switch (e.key) {
        case ' ':
            e.preventDefault();
            togglePlayPause();
            break;
        case 'f':
        case 'F11':
            e.preventDefault();
            toggleFullscreen();
            break;
        case 'Escape':
            if (userPanelOverlay.classList.contains('active')) {
                userPanelOverlay.classList.remove('active');
            } else if (mainContent.classList.contains('video-playing')) {
                mainContent.classList.remove('video-playing');
            }
            break;
        case 'ArrowLeft':
            if (videoPlayer.classList.contains('active')) {
                videoPlayer.currentTime -= 10;
            }
            break;
        case 'ArrowRight':
            if (videoPlayer.classList.contains('active')) {
                videoPlayer.currentTime += 10;
            }
            break;
        case 'ArrowUp':
            videoPlayer.volume = Math.min(1, videoPlayer.volume + 0.1);
            break;
        case 'ArrowDown':
            videoPlayer.volume = Math.max(0, videoPlayer.volume - 0.1);
            break;
        case 'm':
            videoPlayer.muted = !videoPlayer.muted;
            break;
    }
}

// Handle media keys
function handleMediaKey(key) {
    switch (key) {
        case 'playpause':
            togglePlayPause();
            break;
        case 'stop':
            videoPlayer.pause();
            videoPlayer.currentTime = 0;
            break;
        case 'next':
            playNextFile();
            break;
        case 'previous':
            playPreviousFile();
            break;
    }
}

// Toggle play/pause
function togglePlayPause() {
    if (videoPlayer.paused) {
        videoPlayer.play();
    } else {
        videoPlayer.pause();
    }
}

// Toggle fullscreen
function toggleFullscreen() {
    if (window.nedflixDesktop) {
        window.nedflixDesktop.toggleFullscreen();
    } else if (document.fullscreenElement) {
        document.exitFullscreen();
    } else {
        document.documentElement.requestFullscreen();
    }
}

// Play next/previous file in list
function playNextFile() {
    navigateFiles(1);
}

function playPreviousFile() {
    navigateFiles(-1);
}

function navigateFiles(direction) {
    const items = fileList.querySelectorAll('.file-item.video, .file-item.audio');
    const currentActive = fileList.querySelector('.file-item.active');

    if (items.length === 0) return;

    let currentIndex = currentActive ? Array.from(items).indexOf(currentActive) : -1;
    let nextIndex = currentIndex + direction;

    if (nextIndex >= items.length) nextIndex = 0;
    if (nextIndex < 0) nextIndex = items.length - 1;

    const nextItem = items[nextIndex];
    if (nextItem) {
        nextItem.click();
    }
}

// Handle video ended
function handleVideoEnded() {
    if (settings.streaming.autoplay) {
        playNextFile();
    }
}

// Settings management
function loadSettings() {
    const saved = localStorage.getItem('nedflix-desktop-settings');
    if (saved) {
        try {
            settings = { ...settings, ...JSON.parse(saved) };
        } catch (e) {
            console.error('Failed to parse settings:', e);
        }
    }
}

function saveSettings() {
    localStorage.setItem('nedflix-desktop-settings', JSON.stringify(settings));
    applySettings();
    userPanelOverlay.classList.remove('active');

    // Vibration feedback
    if (settings.controller.vibration && window.nedflixDesktop) {
        window.nedflixDesktop.gamepad.vibrate(0, 50, 0.2, 0.2);
    }
}

function applySettings() {
    // Theme
    applyTheme(settings.theme);

    // Update form controls
    const themeSelect = document.getElementById('theme-select');
    if (themeSelect) themeSelect.value = settings.theme;

    const qualitySelect = document.getElementById('quality-select');
    if (qualitySelect) qualitySelect.value = settings.streaming.quality;

    const volumeSlider = document.getElementById('volume-slider');
    if (volumeSlider) {
        volumeSlider.value = settings.streaming.volume;
        document.getElementById('volume-value').textContent = `${settings.streaming.volume}%`;
    }

    const speedSelect = document.getElementById('speed-select');
    if (speedSelect) speedSelect.value = settings.streaming.playbackSpeed;

    const autoplayToggle = document.getElementById('autoplay-toggle');
    if (autoplayToggle) autoplayToggle.checked = settings.streaming.autoplay;

    const vibrationToggle = document.getElementById('controller-vibration-toggle');
    if (vibrationToggle) vibrationToggle.checked = settings.controller.vibration;

    // IPTV settings
    const iptvPlaylistUrl = document.getElementById('iptv-playlist-url');
    if (iptvPlaylistUrl) iptvPlaylistUrl.value = settings.iptv?.playlistUrl || '';

    const iptvEpgUrl = document.getElementById('iptv-epg-url');
    if (iptvEpgUrl) iptvEpgUrl.value = settings.iptv?.epgUrl || '';

    // Apply to player
    videoPlayer.volume = settings.streaming.volume / 100;
    videoPlayer.playbackRate = settings.streaming.playbackSpeed;
}

function applyTheme(theme) {
    if (theme === 'light') {
        document.documentElement.setAttribute('data-theme', 'light');
    } else {
        document.documentElement.removeAttribute('data-theme');
    }
    localStorage.setItem('nedflix-theme', theme);
}

// Utility functions
function escapeHtml(text) {
    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
}

function formatSize(bytes) {
    if (bytes === 0) return '0 B';
    const k = 1024;
    const sizes = ['B', 'KB', 'MB', 'GB', 'TB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
}

// Export for gamepad module
window.nedflixApp = {
    togglePlayPause,
    toggleFullscreen,
    playNextFile,
    playPreviousFile,
    navigateFiles,
    backToLibraries,
    openLibrary,
    loadDirectory,
    settings,
    videoPlayer
};
