// Application State
let currentPath = '/mnt/nfs';
let parentPath = '';
let canGoUp = false;
let currentUser = null;
let userSettings = null;
const DEFAULT_VOLUME = 0.8; // Default 80% volume
let availableAvatars = [];
let selectedAvatar = 'cat';
let currentVideoPath = null;
let currentVideoName = null;
let subtitleStatus = { configured: false };
let audioTrackStatus = { canDetect: false, canSwitch: false };
let currentAudioTracks = [];
let selectedAudioTrack = 0;
let currentEmbeddedSubtitles = [];
let selectedEmbeddedSubtitle = -1; // -1 means off
let currentLibraryRoot = null; // Track which library we're in

// DOM Elements
const librarySelector = document.getElementById('library-selector');
const mainContent = document.getElementById('main-content');
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
const logoElement = document.querySelector('.logo');

// Settings elements
const qualitySelect = document.getElementById('quality-select');
const volumeSlider = document.getElementById('volume-slider');
const volumeValue = document.getElementById('volume-value');
const speedSelect = document.getElementById('speed-select');
const autoplayToggle = document.getElementById('autoplay-toggle');
const subtitlesToggle = document.getElementById('subtitles-toggle');
const audioLanguageSelect = document.getElementById('audio-language-select');
const subtitleLanguageSelect = document.getElementById('subtitle-language-select');

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

        // Show admin section if user is admin
        if (currentUser.isAdmin) {
            showAdminSection();
        }

        // Show library selector instead of loading directory
        showLibrarySelector();

        // Check subtitle configuration
        checkSubtitleStatus();

        // Check audio track configuration
        checkAudioTrackStatus();
    } catch (error) {
        console.error('Auth check failed:', error);
        window.location.href = '/login.html';
    }
}

// Show the library selector screen
function showLibrarySelector() {
    librarySelector.style.display = 'flex';
    mainContent.style.display = 'none';
    currentLibraryRoot = null;

    // Stop visualizer if playing
    stopVisualizer();

    // Reset video player
    videoPlayer.src = '';
    videoPlayer.classList.remove('active');
    videoPlaceholder.classList.remove('hidden');
    videoName.textContent = 'Select a video to play';
}

// Show the main content (browser + player)
function showMainContent(libraryPath) {
    librarySelector.style.display = 'none';
    mainContent.style.display = 'grid';
    currentLibraryRoot = libraryPath;
    currentPath = libraryPath;
    loadDirectory(libraryPath);
}

// Handle library card selection
function selectLibrary(path) {
    console.log(`Selected library: ${path}`);
    showMainContent(path);
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
    audioLanguageSelect.value = s.audioLanguage || '';
    subtitleLanguageSelect.value = s.subtitleLanguage || 'en';

    // Apply to video player (use setAudioVolume if available for Web Audio API support)
    const volume = s.volume ? s.volume / 100 : DEFAULT_VOLUME;
    if (typeof setAudioVolume === 'function') {
        setAudioVolume(volume);
    } else {
        videoPlayer.volume = volume;
    }
    videoPlayer.playbackRate = s.playbackSpeed || 1;
}

// Setup event listeners
function setupEventListeners() {
    // Library card selection
    document.querySelectorAll('.library-card').forEach(card => {
        card.addEventListener('click', () => {
            const path = card.dataset.path;
            if (path) {
                selectLibrary(path);
            }
        });
    });

    // Logo click to return to library selector
    if (logoElement) {
        logoElement.style.cursor = 'pointer';
        logoElement.addEventListener('click', () => {
            showLibrarySelector();
        });
    }

    // Navigation
    parentBtn.addEventListener('click', () => {
        // If we're at the library root, go back to library selector
        if (currentPath === currentLibraryRoot || !canGoUp) {
            showLibrarySelector();
        } else if (parentPath) {
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

    // Metadata scan buttons
    document.querySelectorAll('.scan-btn').forEach(btn => {
        btn.addEventListener('click', () => startMetadataScan(btn.dataset.path, btn));
    });

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
            subtitles: subtitlesToggle.checked,
            audioLanguage: audioLanguageSelect.value,
            subtitleLanguage: subtitleLanguageSelect.value
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

            // Always enable back button when in a library (to return to selector)
            parentBtn.disabled = !currentLibraryRoot;
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

        // Build display name from metadata
        let displayName = item.cleanTitle || item.name;
        let subtitle = '';

        if (item.isVideo || item.isAudio) {
            // For TV episodes, show "S01E02 - Episode Title"
            if (item.season && item.episode) {
                const epNum = `S${String(item.season).padStart(2, '0')}E${String(item.episode).padStart(2, '0')}`;
                if (item.episodeTitle) {
                    subtitle = `${epNum} - ${item.episodeTitle}`;
                } else {
                    subtitle = epNum;
                }
            }
            // Add year for movies
            if (item.year && !item.season) {
                displayName = `${displayName} (${item.year})`;
            }
        }

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
            if (item.hasMetadata) div.classList.add('has-metadata');

            // Use poster thumbnail if available
            const iconHtml = item.poster
                ? `<div class="file-poster"><img src="${escapeHtml(item.poster)}" alt="" loading="lazy"></div>`
                : `<div class="file-icon">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                        <polygon points="5 3 19 12 5 21 5 3"></polygon>
                    </svg>
                </div>`;

            // Build meta line
            let metaParts = [];
            if (item.rating) metaParts.push(`<span class="rating">${item.rating}</span>`);
            if (item.genre) metaParts.push(item.genre.split(',')[0].trim());
            metaParts.push(formatFileSize(item.size));

            div.innerHTML = `
                ${iconHtml}
                <div class="file-info">
                    <div class="file-name">${escapeHtml(displayName)}</div>
                    ${subtitle ? `<div class="file-subtitle">${escapeHtml(subtitle)}</div>` : ''}
                    <div class="file-meta">${metaParts.join(' &bull; ')}</div>
                </div>
            `;

            const itemDisplayName = subtitle ? `${displayName} - ${subtitle}` : displayName;
            div.addEventListener('click', () => playVideo(item.path, itemDisplayName));
        } else if (item.isAudio) {
            div.classList.add('audio');
            if (item.hasMetadata) div.classList.add('has-metadata');

            const iconHtml = item.poster
                ? `<div class="file-poster"><img src="${escapeHtml(item.poster)}" alt="" loading="lazy"></div>`
                : `<div class="file-icon">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                        <path d="M9 18V5l12-2v13"></path>
                        <circle cx="6" cy="18" r="3"></circle>
                        <circle cx="18" cy="16" r="3"></circle>
                    </svg>
                </div>`;

            div.innerHTML = `
                ${iconHtml}
                <div class="file-info">
                    <div class="file-name">${escapeHtml(displayName)}</div>
                    ${subtitle ? `<div class="file-subtitle">${escapeHtml(subtitle)}</div>` : ''}
                    <div class="file-meta">${formatFileSize(item.size)}</div>
                </div>
            `;

            div.addEventListener('click', () => playAudio(item.path, displayName));
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

    // Stop visualizer if it was playing
    stopVisualizer();

    // Store current video path for subtitle and audio operations
    currentVideoPath = path;
    currentVideoName = name;
    selectedAudioTrack = 0;
    currentAudioTracks = [];
    currentEmbeddedSubtitles = [];
    selectedEmbeddedSubtitle = -1;

    // Remove any existing subtitle tracks
    removeSubtitleTracks();

    // Hide audio and subtitle selectors while loading
    hideAudioTrackSelector();
    hideEmbeddedSubtitleSelector();

    // Pre-fetch audio track info to find preferred language
    let preferredAudioIndex = 0;
    if (audioTrackStatus.canDetect) {
        const trackInfo = await fetchAudioTrackInfo(path);
        if (trackInfo && trackInfo.tracks && trackInfo.tracks.length > 1) {
            currentAudioTracks = trackInfo.tracks;
            showAudioTrackSelector(trackInfo.tracks);

            // Find preferred language track
            const preferredLang = userSettings?.streaming?.audioLanguage || 'eng';
            const foundIndex = trackInfo.tracks.findIndex(t =>
                t.language && t.language.toLowerCase().startsWith(preferredLang.toLowerCase())
            );
            if (foundIndex >= 0) {
                preferredAudioIndex = foundIndex;
                console.log(`Will switch to preferred audio: ${trackInfo.tracks[foundIndex].label}`);
            }
        }
    }

    videoName.textContent = name;
    videoPlaceholder.classList.add('hidden');
    videoPlayer.classList.add('active');

    // Set up native audio track switching when metadata loads
    const onMetadataLoaded = () => {
        videoPlayer.removeEventListener('loadedmetadata', onMetadataLoaded);

        // Try native switching to preferred track
        if (preferredAudioIndex > 0 && videoPlayer.audioTracks && videoPlayer.audioTracks.length > 0) {
            console.log(`Native audioTracks available: ${videoPlayer.audioTracks.length}`);
            for (let i = 0; i < videoPlayer.audioTracks.length; i++) {
                videoPlayer.audioTracks[i].enabled = (i === preferredAudioIndex);
            }
            selectedAudioTrack = preferredAudioIndex;
            // Update the dropdown to show selected track
            const select = document.getElementById('audio-track-select');
            if (select) select.value = preferredAudioIndex;
            showAudioTrackMessage(`Audio: ${currentAudioTracks[preferredAudioIndex]?.label || 'Track ' + (preferredAudioIndex + 1)}`, 'success');
        }
    };

    videoPlayer.addEventListener('loadedmetadata', onMetadataLoaded);

    // Load the video
    videoPlayer.src = videoUrl;

    // Apply user settings with fallback defaults
    videoPlayer.volume = userSettings?.streaming?.volume
        ? userSettings.streaming.volume / 100
        : DEFAULT_VOLUME;
    videoPlayer.playbackRate = userSettings?.streaming?.playbackSpeed || 1;

    videoPlayer.play().catch(e => console.log('Autoplay prevented:', e));

    // Mark active item
    document.querySelectorAll('.file-item').forEach(el => el.classList.remove('active'));
    event.currentTarget.classList.add('active');

    // Scroll to player on mobile
    if (window.innerWidth <= 1200) {
        document.getElementById('player-section').scrollIntoView({ behavior: 'smooth' });
    }

    // Load embedded subtitles
    await loadEmbeddedSubtitles(path);
}

// Play audio file
function playAudio(path, name) {
    const audioUrl = `/api/audio?path=${encodeURIComponent(path)}`;

    // Reset video-related state
    currentVideoPath = null;
    currentVideoName = null;
    selectedAudioTrack = 0;
    currentAudioTracks = [];
    currentEmbeddedSubtitles = [];
    selectedEmbeddedSubtitle = -1;

    // Remove any existing subtitle tracks
    removeSubtitleTracks();

    // Hide audio and subtitle selectors (not relevant for audio files)
    hideAudioTrackSelector();
    hideEmbeddedSubtitleSelector();

    // Use the video player element for audio (it handles audio files fine)
    videoPlayer.src = audioUrl;
    videoName.textContent = name;

    videoPlaceholder.classList.add('hidden');
    videoPlayer.classList.add('active');

    // Apply user settings with fallback defaults
    const targetVolume = userSettings?.streaming?.volume
        ? userSettings.streaming.volume / 100
        : DEFAULT_VOLUME;
    videoPlayer.volume = targetVolume;
    videoPlayer.playbackRate = userSettings?.streaming?.playbackSpeed || 1;

    // Initialize and start visualizer for audio playback
    if (initVisualizer()) {
        // Need to wait for audio to load before connecting
        videoPlayer.addEventListener('canplay', function onCanPlay() {
            videoPlayer.removeEventListener('canplay', onCanPlay);
            connectAudioSource();
            // Apply volume through Web Audio API gain node
            setAudioVolume(targetVolume);
            startVisualizer();
        }, { once: true });
    }

    videoPlayer.play().catch(e => console.log('Autoplay prevented:', e));

    // Mark active item
    document.querySelectorAll('.file-item').forEach(el => el.classList.remove('active'));
    event.currentTarget.classList.add('active');

    // Scroll to player on mobile
    if (window.innerWidth <= 1200) {
        document.getElementById('player-section').scrollIntoView({ behavior: 'smooth' });
    }
}

// Fetch audio track info from server (without switching)
async function fetchAudioTrackInfo(videoPath) {
    try {
        const response = await fetch(`/api/audio-tracks?path=${encodeURIComponent(videoPath)}`);
        return await response.json();
    } catch (error) {
        console.error('Failed to fetch audio track info:', error);
        return null;
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

    // The direct stream always uses the first audio track (index 0)
    // so we show that as selected by default
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
                    <option value="${index}" ${index === 0 ? 'selected' : ''}>
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

    // selectedAudioTrack is already 0 from playVideo initialization
}

// Hide the audio track selector
function hideAudioTrackSelector() {
    const container = document.getElementById('audio-track-container');
    if (container) {
        container.style.display = 'none';
    }
}

// Check if browser supports native audio track switching
function supportsNativeAudioTracks() {
    return videoPlayer.audioTracks && videoPlayer.audioTracks.length > 0;
}

// Try native audio track switching (preserves seek functionality)
function tryNativeAudioSwitch(trackIndex) {
    if (!supportsNativeAudioTracks()) {
        return false;
    }

    const audioTracks = videoPlayer.audioTracks;
    if (trackIndex >= audioTracks.length) {
        return false;
    }

    // Disable all tracks, then enable the selected one
    for (let i = 0; i < audioTracks.length; i++) {
        audioTracks[i].enabled = (i === trackIndex);
    }

    console.log(`Native audio switch to track ${trackIndex}`);
    return true;
}

async function switchToAudioTrack(trackIndex) {
    // Update the dropdown if it exists
    const select = document.getElementById('audio-track-select');
    if (select) {
        select.value = trackIndex;
    }

    // Try native switching first (preserves seek/duration)
    if (tryNativeAudioSwitch(trackIndex)) {
        selectedAudioTrack = trackIndex;
        showAudioTrackMessage(`Audio: ${currentAudioTracks[trackIndex]?.label || 'Track ' + (trackIndex + 1)}`, 'success');
        return Promise.resolve();
    }

    // Fall back to FFmpeg streaming (loses seek functionality)
    return new Promise((resolve, reject) => {
        // Store current playback state
        const currentTime = videoPlayer.currentTime;
        const wasPlaying = !videoPlayer.paused;
        const volume = videoPlayer.volume;
        const playbackRate = videoPlayer.playbackRate;

        // Show loading indicator
        showAudioTrackMessage('Switching audio track...', 'loading');

        // Update selected track
        selectedAudioTrack = trackIndex;

        // Create new video URL with selected audio track
        const videoUrl = `/api/video-stream?path=${encodeURIComponent(currentVideoPath)}&audio=${trackIndex}&start=${currentTime}`;

        // Remove subtitle tracks (they'll need to be re-added)
        removeSubtitleTracks();

        // Load new stream
        videoPlayer.src = videoUrl;

        // Restore settings
        videoPlayer.volume = volume;
        videoPlayer.playbackRate = playbackRate;

        // Wait for video to be ready
        videoPlayer.onloadeddata = async () => {
            showAudioTrackMessage(`Audio: ${currentAudioTracks[trackIndex]?.label || 'Track ' + (trackIndex + 1)}`, 'success');

            if (wasPlaying) {
                videoPlayer.play().catch(e => console.log('Autoplay prevented:', e));
            }

            // Reload subtitles if enabled
            if (userSettings?.streaming?.subtitles && subtitleStatus.configured) {
                await loadSubtitles(currentVideoPath);
            }
            resolve();
        };

        videoPlayer.onerror = () => {
            showAudioTrackMessage('Failed to switch audio track', 'error');
            // Revert to original stream
            selectedAudioTrack = 0;
            if (select) {
                select.value = 0;
            }
            videoPlayer.src = `/api/video?path=${encodeURIComponent(currentVideoPath)}`;
            reject(new Error('Failed to switch audio track'));
        };
    });
}

// Handle audio track change from UI
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

    try {
        await switchToAudioTrack(newTrack);
    } catch (error) {
        console.error('Failed to switch audio track:', error);
    }
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

// Load embedded subtitles for the current video
async function loadEmbeddedSubtitles(videoPath) {
    try {
        const response = await fetch(`/api/embedded-subtitles?path=${encodeURIComponent(videoPath)}`);
        const data = await response.json();

        if (data.tracks && data.tracks.length > 0) {
            // Filter to text-based subtitles that can be extracted
            const extractableTracks = data.tracks.filter(t => t.isTextBased);

            if (extractableTracks.length > 0) {
                currentEmbeddedSubtitles = extractableTracks;
                showEmbeddedSubtitleSelector(extractableTracks);
                console.log(`Found ${extractableTracks.length} extractable subtitle track(s)`);

                // Auto-select preferred language subtitle
                const preferredLang = userSettings?.streaming?.subtitleLanguage || 'en';
                const preferredIndex = extractableTracks.findIndex(t =>
                    t.language && (
                        t.language.toLowerCase() === preferredLang.toLowerCase() ||
                        t.language.toLowerCase().startsWith(preferredLang.toLowerCase())
                    )
                );

                // If preferred language found, automatically enable it
                if (preferredIndex >= 0) {
                    console.log(`Auto-selecting subtitle: ${extractableTracks[preferredIndex].label}`);
                    await applyEmbeddedSubtitle(preferredIndex);
                }
            } else {
                console.log('No text-based subtitles found (PGS/DVD subtitles cannot be extracted)');
                hideEmbeddedSubtitleSelector();
            }
        } else {
            hideEmbeddedSubtitleSelector();
            currentEmbeddedSubtitles = [];
        }
    } catch (error) {
        console.error('Failed to load embedded subtitles:', error);
        hideEmbeddedSubtitleSelector();
    }
}

// Show the embedded subtitle selector UI
function showEmbeddedSubtitleSelector(tracks) {
    let container = document.getElementById('embedded-subtitle-container');

    if (!container) {
        container = document.createElement('div');
        container.id = 'embedded-subtitle-container';
        container.className = 'embedded-subtitle-container';
        document.querySelector('.video-header').appendChild(container);
    }

    container.innerHTML = `
        <div class="embedded-subtitle-selector">
            <label for="embedded-subtitle-select">
                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                    <rect x="2" y="4" width="20" height="16" rx="2" ry="2"></rect>
                    <line x1="6" y1="12" x2="18" y2="12"></line>
                    <line x1="6" y1="16" x2="14" y2="16"></line>
                </svg>
                Subtitles
            </label>
            <select id="embedded-subtitle-select">
                <option value="-1">Off</option>
                ${tracks.map((track, index) => `
                    <option value="${index}">
                        ${escapeHtml(track.label)}
                    </option>
                `).join('')}
            </select>
        </div>
    `;

    container.style.display = 'flex';

    // Add event listener for subtitle selection
    const select = document.getElementById('embedded-subtitle-select');
    select.addEventListener('change', handleEmbeddedSubtitleChange);
}

// Hide the embedded subtitle selector
function hideEmbeddedSubtitleSelector() {
    const container = document.getElementById('embedded-subtitle-container');
    if (container) {
        container.style.display = 'none';
    }
}

// Apply an embedded subtitle track
async function applyEmbeddedSubtitle(trackIndex) {
    // Remove existing subtitle tracks
    removeSubtitleTracks();

    if (trackIndex < 0) {
        selectedEmbeddedSubtitle = -1;
        updateSubtitleIndicator(null);
        return;
    }

    const track = currentEmbeddedSubtitles[trackIndex];
    if (!track) return;

    selectedEmbeddedSubtitle = trackIndex;

    // Update dropdown
    const select = document.getElementById('embedded-subtitle-select');
    if (select) {
        select.value = trackIndex;
    }

    updateSubtitleIndicator('searching');

    try {
        // Create subtitle track element
        const subtitleTrack = document.createElement('track');
        subtitleTrack.kind = 'subtitles';
        subtitleTrack.label = track.label;
        subtitleTrack.srclang = track.language || 'en';
        subtitleTrack.src = `/api/embedded-subtitles/extract?path=${encodeURIComponent(currentVideoPath)}&track=${track.streamIndex}`;
        subtitleTrack.default = true;

        videoPlayer.appendChild(subtitleTrack);

        // Wait for track to load then enable it
        subtitleTrack.addEventListener('load', () => {
            if (videoPlayer.textTracks.length > 0) {
                videoPlayer.textTracks[0].mode = 'showing';
            }
            updateSubtitleIndicator('loaded', track.label);
        });

        subtitleTrack.addEventListener('error', () => {
            updateSubtitleIndicator('error', 'Failed to load subtitle');
        });

    } catch (error) {
        console.error('Failed to apply embedded subtitle:', error);
        updateSubtitleIndicator('error', 'Failed to load subtitle');
    }
}

// Handle embedded subtitle change from UI
async function handleEmbeddedSubtitleChange(event) {
    const newTrack = parseInt(event.target.value, 10);

    if (newTrack === selectedEmbeddedSubtitle) {
        return;
    }

    await applyEmbeddedSubtitle(newTrack);
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

    const subtitleLang = userSettings?.streaming?.subtitleLanguage || 'en';

    try {
        const response = await fetch(`/api/subtitles/search?path=${encodeURIComponent(videoPath)}&language=${subtitleLang}`);
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

// ========== METADATA SCANNING ==========
let scanPollInterval = null;

async function startMetadataScan(dirPath, button) {
    const progressDiv = document.getElementById('scan-progress');
    const progressFill = document.getElementById('scan-progress-fill');
    const progressText = document.getElementById('scan-progress-text');

    // Disable all scan buttons
    document.querySelectorAll('.scan-btn').forEach(btn => {
        btn.disabled = true;
    });
    button.classList.add('scanning');
    button.textContent = 'Scanning...';
    progressDiv.style.display = 'block';
    progressFill.style.width = '0%';
    progressText.textContent = 'Starting scan...';

    try {
        const response = await fetch('/api/metadata/scan', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ path: dirPath })
        });

        const data = await response.json();

        if (data.error) {
            progressText.textContent = `Error: ${data.error}`;
            resetScanButtons(button);
            return;
        }

        // Start polling for progress
        scanPollInterval = setInterval(() => pollScanProgress(button), 1000);
    } catch (error) {
        progressText.textContent = `Error: ${error.message}`;
        resetScanButtons(button);
    }
}

async function pollScanProgress(button) {
    const progressFill = document.getElementById('scan-progress-fill');
    const progressText = document.getElementById('scan-progress-text');

    try {
        const response = await fetch('/api/metadata/scan/progress');
        const data = await response.json();

        if (data.status === 'running') {
            const percent = data.total > 0 ? Math.round((data.scanned / data.total) * 100) : 0;
            progressFill.style.width = `${percent}%`;
            progressText.textContent = `Scanning: ${data.scanned}/${data.total} - ${data.current || '...'}`;
        } else if (data.status === 'completed') {
            clearInterval(scanPollInterval);
            scanPollInterval = null;
            progressFill.style.width = '100%';
            progressText.textContent = `Completed! Scanned ${data.scanned} files.`;
            resetScanButtons(button);

            // Reload current directory to show updated metadata
            if (currentLibraryRoot) {
                loadDirectory(currentPath);
            }

            // Hide progress after a delay
            setTimeout(() => {
                document.getElementById('scan-progress').style.display = 'none';
            }, 3000);
        } else if (data.status === 'error') {
            clearInterval(scanPollInterval);
            scanPollInterval = null;
            progressText.textContent = `Error: ${data.error}`;
            resetScanButtons(button);
        }
    } catch (error) {
        clearInterval(scanPollInterval);
        scanPollInterval = null;
        progressText.textContent = `Error: ${error.message}`;
        resetScanButtons(button);
    }
}

function resetScanButtons(activeButton) {
    document.querySelectorAll('.scan-btn').forEach(btn => {
        btn.disabled = false;
        btn.classList.remove('scanning');
    });
    activeButton.textContent = activeButton.id === 'scan-movies-btn' ? 'Scan Movies' : 'Scan TV Shows';
}

// ========== USER MANAGEMENT (Admin) ==========

// Show admin section and load users
function showAdminSection() {
    const adminSection = document.getElementById('admin-section');
    if (adminSection) {
        adminSection.style.display = 'block';
        loadUsers();
        setupUserManagementListeners();
    }
}

// Setup user management event listeners
function setupUserManagementListeners() {
    const addUserBtn = document.getElementById('add-user-btn');
    if (addUserBtn && !addUserBtn.hasAttribute('data-listener')) {
        addUserBtn.setAttribute('data-listener', 'true');
        addUserBtn.addEventListener('click', addUser);
    }
}

// Track if users have been loaded
let usersLoaded = false;

// Load all users from the server
async function loadUsers(forceReload = false) {
    // Skip if already loaded and not forcing reload
    if (usersLoaded && !forceReload) return;

    try {
        const response = await fetch('/api/admin/users');
        const data = await response.json();

        if (data.users) {
            renderUsersList(data.users);
            usersLoaded = true;
        }
    } catch (error) {
        console.error('Failed to load users:', error);
    }
}

// Render the users list
function renderUsersList(users) {
    const usersList = document.getElementById('users-list');
    if (!usersList) return;

    if (users.length === 0) {
        usersList.innerHTML = '<div class="no-users-message">No users added yet</div>';
        return;
    }

    usersList.innerHTML = users.map(user => {
        const isCurrentUser = user.id === currentUser.id;

        return `
            <div class="user-item" data-user-id="${user.id}">
                <div class="user-avatar">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5">
                        <path d="M20 21v-2a4 4 0 0 0-4-4H8a4 4 0 0 0-4 4v2"></path>
                        <circle cx="12" cy="7" r="4"></circle>
                    </svg>
                </div>
                <div class="user-info">
                    <div class="user-name">
                        ${user.displayName || user.username || 'User'}
                        ${user.isAdmin ? '<span class="user-badge admin">Admin</span>' : ''}
                    </div>
                    <div class="user-email">${user.username || user.provider}</div>
                </div>
                <div class="user-actions">
                    ${!isCurrentUser ? `
                        <button class="user-action-btn toggle-allowed ${!user.isAllowed ? 'disabled' : ''}"
                                onclick="toggleUserAllowed('${user.id}', ${!user.isAllowed})"
                                title="${user.isAllowed ? 'Disable access' : 'Enable access'}">
                            ${user.isAllowed ? 'Enabled' : 'Disabled'}
                        </button>
                        <button class="user-action-btn delete"
                                onclick="deleteUser('${user.id}')"
                                title="Delete user">
                            Delete
                        </button>
                    ` : '<span style="color: var(--color-text-dim); font-size: 12px;">You</span>'}
                </div>
            </div>
        `;
    }).join('');
}

// Add a new user
async function addUser() {
    const usernameInput = document.getElementById('new-user-username');
    const passwordInput = document.getElementById('new-user-password');
    const nameInput = document.getElementById('new-user-name');
    const adminCheckbox = document.getElementById('new-user-admin');

    const username = usernameInput.value.trim();
    const password = passwordInput.value;
    const displayName = nameInput.value.trim();
    const isAdmin = adminCheckbox.checked;

    if (!username) {
        alert('Please enter a username');
        return;
    }
    if (!password) {
        alert('Please enter a password');
        return;
    }
    if (password.length < 4) {
        alert('Password must be at least 4 characters');
        return;
    }

    try {
        const response = await fetch('/api/admin/users', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ username, password, displayName, isAdmin })
        });

        const data = await response.json();

        if (data.success) {
            usernameInput.value = '';
            passwordInput.value = '';
            nameInput.value = '';
            adminCheckbox.checked = false;
            loadUsers(true);
        } else {
            alert(data.error || 'Failed to add user');
        }
    } catch (error) {
        console.error('Failed to add user:', error);
        alert('Failed to add user');
    }
}

// Toggle user allowed status
async function toggleUserAllowed(userId, newStatus) {
    try {
        const response = await fetch(`/api/admin/users/${encodeURIComponent(userId)}`, {
            method: 'PUT',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ isAllowed: newStatus })
        });

        const data = await response.json();

        if (data.success) {
            loadUsers(true);
        } else {
            alert(data.error || 'Failed to update user');
        }
    } catch (error) {
        console.error('Failed to update user:', error);
        alert('Failed to update user');
    }
}

// Delete a user
async function deleteUser(userId) {
    if (!confirm('Are you sure you want to delete this user?')) {
        return;
    }

    try {
        const response = await fetch(`/api/admin/users/${encodeURIComponent(userId)}`, {
            method: 'DELETE'
        });

        const data = await response.json();

        if (data.success) {
            loadUsers(true);
        } else {
            alert(data.error || 'Failed to delete user');
        }
    } catch (error) {
        console.error('Failed to delete user:', error);
        alert('Failed to delete user');
    }
}

// ========== AUDIO VISUALIZER ==========
let audioContext = null;
let analyser = null;
let audioSource = null;
let gainNode = null;
let visualizerCanvas = null;
let visualizerCtx = null;
let animationFrameId = null;
let isVisualizerActive = false;
let audioSourceConnected = false;
let visualizerType = 'bars';
let particles = [];

// Initialize the audio visualizer
function initVisualizer() {
    visualizerCanvas = document.getElementById('audio-visualizer');
    if (!visualizerCanvas) return false;

    visualizerCtx = visualizerCanvas.getContext('2d');

    // Create audio context if not exists
    if (!audioContext) {
        audioContext = new (window.AudioContext || window.webkitAudioContext)();
    }

    // Create analyser node
    if (!analyser) {
        analyser = audioContext.createAnalyser();
        analyser.fftSize = 512;
        analyser.smoothingTimeConstant = 0.8;
    }

    // Create gain node for volume control
    if (!gainNode) {
        gainNode = audioContext.createGain();
        gainNode.gain.value = DEFAULT_VOLUME;
    }

    // Set up visualizer type selector
    const visualizerSelect = document.getElementById('visualizer-select');
    if (visualizerSelect) {
        visualizerSelect.addEventListener('change', (e) => {
            visualizerType = e.target.value;
            if (visualizerType === 'none') {
                visualizerCanvas.classList.remove('active');
            } else if (isVisualizerActive) {
                visualizerCanvas.classList.add('active');
            }
        });
    }

    return true;
}

// Connect the video player to the analyser (only once per session)
function connectAudioSource() {
    if (audioSourceConnected && audioSource) {
        return true;
    }

    try {
        audioSource = audioContext.createMediaElementSource(videoPlayer);
        // Route: source -> analyser -> gain -> destination
        audioSource.connect(analyser);
        analyser.connect(gainNode);
        gainNode.connect(audioContext.destination);
        audioSourceConnected = true;
        return true;
    } catch (e) {
        if (e.name === 'InvalidStateError') {
            audioSourceConnected = true;
            return true;
        }
        console.log('Audio source connection error:', e.message);
        return false;
    }
}

// Set audio volume (works with Web Audio API)
function setAudioVolume(volume) {
    // Set HTML5 element volume (for video playback without visualizer)
    videoPlayer.volume = volume;
    // Set Web Audio API gain (for audio playback with visualizer)
    if (gainNode) {
        gainNode.gain.value = volume;
    }
}

// Show visualizer controls
function showVisualizerControls() {
    const controls = document.getElementById('visualizer-controls');
    if (controls) {
        controls.classList.add('active');
    }
}

// Hide visualizer controls
function hideVisualizerControls() {
    const controls = document.getElementById('visualizer-controls');
    if (controls) {
        controls.classList.remove('active');
    }
}

// Start the visualizer animation
function startVisualizer() {
    if (!visualizerCanvas || !analyser) return;

    isVisualizerActive = true;
    showVisualizerControls();

    if (visualizerType !== 'none') {
        visualizerCanvas.classList.add('active');
    }

    if (audioContext && audioContext.state === 'suspended') {
        audioContext.resume();
    }

    // Initialize particles for particle mode
    particles = [];

    drawVisualizer();
}

// Stop the visualizer animation
function stopVisualizer() {
    isVisualizerActive = false;
    hideVisualizerControls();

    if (animationFrameId) {
        cancelAnimationFrame(animationFrameId);
        animationFrameId = null;
    }

    if (visualizerCanvas) {
        visualizerCanvas.classList.remove('active');
        if (visualizerCtx) {
            visualizerCtx.clearRect(0, 0, visualizerCanvas.width, visualizerCanvas.height);
        }
    }
}

// Main draw function - dispatches to the selected visualizer
function drawVisualizer() {
    if (!isVisualizerActive || !analyser || !visualizerCanvas || !visualizerCtx) {
        return;
    }

    animationFrameId = requestAnimationFrame(drawVisualizer);

    // Update canvas size
    const container = visualizerCanvas.parentElement;
    if (container) {
        visualizerCanvas.width = container.clientWidth;
        visualizerCanvas.height = container.clientHeight - 50; // Leave space for controls
    }

    // Get frequency data
    const bufferLength = analyser.frequencyBinCount;
    const frequencyData = new Uint8Array(bufferLength);
    const waveformData = new Uint8Array(bufferLength);
    analyser.getByteFrequencyData(frequencyData);
    analyser.getByteTimeDomainData(waveformData);

    // Clear canvas
    visualizerCtx.clearRect(0, 0, visualizerCanvas.width, visualizerCanvas.height);

    // Draw based on selected type
    switch (visualizerType) {
        case 'bars':
            drawBars(frequencyData);
            break;
        case 'wave':
            drawWaveform(waveformData);
            break;
        case 'circular':
            drawCircular(frequencyData);
            break;
        case 'particles':
            drawParticles(frequencyData);
            break;
        case 'none':
        default:
            break;
    }
}

// Bar visualizer
function drawBars(dataArray) {
    const barCount = 64;
    const barWidth = visualizerCanvas.width / barCount;
    const barGap = 2;
    const topPadding = 20; // Small gap from top
    const maxBarHeight = visualizerCanvas.height * 0.6; // Bars hang from top, use 60% of height
    const step = Math.floor(dataArray.length / barCount);

    for (let i = 0; i < barCount; i++) {
        let sum = 0;
        for (let j = 0; j < step; j++) {
            sum += dataArray[i * step + j];
        }
        const average = sum / step;
        const barHeight = (average / 255) * maxBarHeight;

        const x = i * barWidth + barGap / 2;
        const y = topPadding; // Bars start from top

        // Gradient from top to bottom of bar
        const gradient = visualizerCtx.createLinearGradient(x, y, x, y + barHeight);
        gradient.addColorStop(0, 'rgba(124, 92, 255, 0.9)');
        gradient.addColorStop(0.5, 'rgba(0, 212, 170, 0.7)');
        gradient.addColorStop(1, 'rgba(0, 212, 170, 0.3)');

        visualizerCtx.fillStyle = gradient;
        visualizerCtx.fillRect(x, y, barWidth - barGap, barHeight);
    }
}

// Waveform visualizer
function drawWaveform(dataArray) {
    const width = visualizerCanvas.width;
    const height = visualizerCanvas.height;
    const centerY = height / 2;

    visualizerCtx.lineWidth = 3;
    visualizerCtx.lineCap = 'round';
    visualizerCtx.lineJoin = 'round';

    // Create gradient stroke
    const gradient = visualizerCtx.createLinearGradient(0, 0, width, 0);
    gradient.addColorStop(0, 'rgba(124, 92, 255, 0.9)');
    gradient.addColorStop(0.5, 'rgba(0, 212, 170, 0.9)');
    gradient.addColorStop(1, 'rgba(124, 92, 255, 0.9)');
    visualizerCtx.strokeStyle = gradient;

    visualizerCtx.beginPath();

    const sliceWidth = width / dataArray.length;
    let x = 0;

    for (let i = 0; i < dataArray.length; i++) {
        const v = dataArray[i] / 128.0;
        const y = (v * height) / 2;

        if (i === 0) {
            visualizerCtx.moveTo(x, y);
        } else {
            visualizerCtx.lineTo(x, y);
        }
        x += sliceWidth;
    }

    visualizerCtx.stroke();

    // Draw mirrored wave
    visualizerCtx.globalAlpha = 0.3;
    visualizerCtx.beginPath();
    x = 0;

    for (let i = 0; i < dataArray.length; i++) {
        const v = dataArray[i] / 128.0;
        const y = height - (v * height) / 2;

        if (i === 0) {
            visualizerCtx.moveTo(x, y);
        } else {
            visualizerCtx.lineTo(x, y);
        }
        x += sliceWidth;
    }

    visualizerCtx.stroke();
    visualizerCtx.globalAlpha = 1;
}

// Circular visualizer
function drawCircular(dataArray) {
    const centerX = visualizerCanvas.width / 2;
    const centerY = visualizerCanvas.height / 2;
    const radius = Math.min(centerX, centerY) * 0.4;
    const bars = 180;
    const step = Math.floor(dataArray.length / bars);

    for (let i = 0; i < bars; i++) {
        let sum = 0;
        for (let j = 0; j < step; j++) {
            sum += dataArray[i * step + j];
        }
        const average = sum / step;
        const barHeight = (average / 255) * radius * 1.5;

        const angle = (i / bars) * Math.PI * 2 - Math.PI / 2;
        const x1 = centerX + Math.cos(angle) * radius;
        const y1 = centerY + Math.sin(angle) * radius;
        const x2 = centerX + Math.cos(angle) * (radius + barHeight);
        const y2 = centerY + Math.sin(angle) * (radius + barHeight);

        const hue = (i / bars) * 60 + 250; // Purple to teal
        visualizerCtx.strokeStyle = `hsla(${hue}, 70%, 60%, 0.8)`;
        visualizerCtx.lineWidth = 3;
        visualizerCtx.lineCap = 'round';

        visualizerCtx.beginPath();
        visualizerCtx.moveTo(x1, y1);
        visualizerCtx.lineTo(x2, y2);
        visualizerCtx.stroke();
    }

    // Draw center circle
    visualizerCtx.beginPath();
    visualizerCtx.arc(centerX, centerY, radius * 0.3, 0, Math.PI * 2);
    visualizerCtx.fillStyle = 'rgba(124, 92, 255, 0.3)';
    visualizerCtx.fill();
}

// Particle visualizer
function drawParticles(dataArray) {
    const width = visualizerCanvas.width;
    const height = visualizerCanvas.height;

    // Calculate average frequency
    let sum = 0;
    for (let i = 0; i < dataArray.length; i++) {
        sum += dataArray[i];
    }
    const average = sum / dataArray.length;
    const intensity = average / 255;

    // Spawn new particles based on intensity
    if (intensity > 0.3 && particles.length < 150) {
        const count = Math.floor(intensity * 5);
        for (let i = 0; i < count; i++) {
            particles.push({
                x: width / 2 + (Math.random() - 0.5) * 100,
                y: height / 2 + (Math.random() - 0.5) * 100,
                vx: (Math.random() - 0.5) * intensity * 10,
                vy: (Math.random() - 0.5) * intensity * 10,
                size: Math.random() * 4 + 2,
                life: 1,
                hue: Math.random() * 60 + 250
            });
        }
    }

    // Update and draw particles
    for (let i = particles.length - 1; i >= 0; i--) {
        const p = particles[i];

        // Update position
        p.x += p.vx;
        p.y += p.vy;
        p.life -= 0.015;

        // Remove dead particles
        if (p.life <= 0) {
            particles.splice(i, 1);
            continue;
        }

        // Draw particle
        visualizerCtx.beginPath();
        visualizerCtx.arc(p.x, p.y, p.size * p.life, 0, Math.PI * 2);
        visualizerCtx.fillStyle = `hsla(${p.hue}, 70%, 60%, ${p.life * 0.8})`;
        visualizerCtx.fill();

        // Add glow
        visualizerCtx.shadowColor = `hsla(${p.hue}, 70%, 60%, 0.5)`;
        visualizerCtx.shadowBlur = 10;
    }

    visualizerCtx.shadowBlur = 0;

    // Draw center glow based on intensity
    const glowRadius = 50 + intensity * 100;
    const gradient = visualizerCtx.createRadialGradient(
        width / 2, height / 2, 0,
        width / 2, height / 2, glowRadius
    );
    gradient.addColorStop(0, `rgba(124, 92, 255, ${intensity * 0.5})`);
    gradient.addColorStop(0.5, `rgba(0, 212, 170, ${intensity * 0.3})`);
    gradient.addColorStop(1, 'rgba(0, 0, 0, 0)');

    visualizerCtx.beginPath();
    visualizerCtx.arc(width / 2, height / 2, glowRadius, 0, Math.PI * 2);
    visualizerCtx.fillStyle = gradient;
    visualizerCtx.fill();
}
