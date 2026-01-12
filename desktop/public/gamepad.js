/**
 * Nedflix Desktop - Xbox/Gamepad Controller Support
 * Handles navigation and playback control with gamepads
 */

(function() {
    'use strict';

    // Gamepad state
    let gamepadIndex = null;
    let lastButtonStates = {};
    let lastAxisStates = {};
    let pollInterval = null;
    let focusedIndex = 0;
    let currentView = 'library'; // 'library', 'browser', 'player', 'settings'

    // Button mappings (Xbox controller layout)
    const BUTTONS = {
        A: 0,           // Select/Confirm
        B: 1,           // Back/Cancel
        X: 2,           // Play/Pause
        Y: 3,           // Fullscreen
        LB: 4,          // Previous
        RB: 5,          // Next
        LT: 6,          // Volume Down
        RT: 7,          // Volume Up
        BACK: 8,        // Menu/Settings
        START: 9,       // Play/Pause (alternate)
        L_STICK: 10,    // Left stick press
        R_STICK: 11,    // Right stick press
        DPAD_UP: 12,
        DPAD_DOWN: 13,
        DPAD_LEFT: 14,
        DPAD_RIGHT: 15
    };

    // Axis thresholds
    const AXIS_THRESHOLD = 0.5;
    const AXIS_REPEAT_DELAY = 300;
    let axisRepeatTimers = {};

    // Initialize
    document.addEventListener('DOMContentLoaded', initGamepad);

    function initGamepad() {
        // Gamepad connected
        window.addEventListener('gamepadconnected', (e) => {
            console.log('Gamepad connected:', e.gamepad.id);
            gamepadIndex = e.gamepad.index;
            showGamepadIndicator(e.gamepad.id);
            updateControllerStatus(e.gamepad.id);
            startPolling();
        });

        // Gamepad disconnected
        window.addEventListener('gamepaddisconnected', (e) => {
            console.log('Gamepad disconnected:', e.gamepad.id);
            if (e.gamepad.index === gamepadIndex) {
                gamepadIndex = null;
                hideGamepadIndicator();
                updateControllerStatus(null);
                stopPolling();
            }
        });

        // Check for already connected gamepads
        const gamepads = navigator.getGamepads();
        for (let i = 0; i < gamepads.length; i++) {
            if (gamepads[i]) {
                gamepadIndex = gamepads[i].index;
                showGamepadIndicator(gamepads[i].id);
                updateControllerStatus(gamepads[i].id);
                startPolling();
                break;
            }
        }
    }

    function startPolling() {
        if (pollInterval) return;
        pollInterval = setInterval(pollGamepad, 16); // ~60fps
    }

    function stopPolling() {
        if (pollInterval) {
            clearInterval(pollInterval);
            pollInterval = null;
        }
    }

    function pollGamepad() {
        if (gamepadIndex === null) return;

        const gamepads = navigator.getGamepads();
        const gamepad = gamepads[gamepadIndex];
        if (!gamepad) return;

        // Process buttons
        gamepad.buttons.forEach((button, index) => {
            const wasPressed = lastButtonStates[index];
            const isPressed = button.pressed;

            // Button just pressed
            if (isPressed && !wasPressed) {
                handleButtonPress(index, button.value);
            }

            lastButtonStates[index] = isPressed;
        });

        // Process axes (left stick for navigation)
        handleAxisInput(gamepad.axes);
    }

    function handleButtonPress(buttonIndex, value) {
        // Determine current view
        updateCurrentView();

        switch (buttonIndex) {
            case BUTTONS.A:
                handleConfirm();
                break;
            case BUTTONS.B:
                handleBack();
                break;
            case BUTTONS.X:
            case BUTTONS.START:
                handlePlayPause();
                break;
            case BUTTONS.Y:
                handleFullscreen();
                break;
            case BUTTONS.LB:
                handlePrevious();
                break;
            case BUTTONS.RB:
                handleNext();
                break;
            case BUTTONS.LT:
                handleVolumeDown();
                break;
            case BUTTONS.RT:
                handleVolumeUp();
                break;
            case BUTTONS.BACK:
                handleSettings();
                break;
            case BUTTONS.DPAD_UP:
                navigateFocus(-1, 'vertical');
                break;
            case BUTTONS.DPAD_DOWN:
                navigateFocus(1, 'vertical');
                break;
            case BUTTONS.DPAD_LEFT:
                navigateFocus(-1, 'horizontal');
                break;
            case BUTTONS.DPAD_RIGHT:
                navigateFocus(1, 'horizontal');
                break;
        }
    }

    function handleAxisInput(axes) {
        // Left stick X (0) and Y (1)
        const leftX = axes[0] || 0;
        const leftY = axes[1] || 0;

        // Horizontal navigation
        if (Math.abs(leftX) > AXIS_THRESHOLD) {
            const direction = leftX > 0 ? 'right' : 'left';
            if (!axisRepeatTimers[direction]) {
                navigateFocus(leftX > 0 ? 1 : -1, 'horizontal');
                axisRepeatTimers[direction] = setTimeout(() => {
                    axisRepeatTimers[direction] = null;
                }, AXIS_REPEAT_DELAY);
            }
        } else {
            clearTimeout(axisRepeatTimers['left']);
            clearTimeout(axisRepeatTimers['right']);
            axisRepeatTimers['left'] = null;
            axisRepeatTimers['right'] = null;
        }

        // Vertical navigation
        if (Math.abs(leftY) > AXIS_THRESHOLD) {
            const direction = leftY > 0 ? 'down' : 'up';
            if (!axisRepeatTimers[direction]) {
                navigateFocus(leftY > 0 ? 1 : -1, 'vertical');
                axisRepeatTimers[direction] = setTimeout(() => {
                    axisRepeatTimers[direction] = null;
                }, AXIS_REPEAT_DELAY);
            }
        } else {
            clearTimeout(axisRepeatTimers['up']);
            clearTimeout(axisRepeatTimers['down']);
            axisRepeatTimers['up'] = null;
            axisRepeatTimers['down'] = null;
        }

        // Right stick for seeking (axes 2 and 3)
        const rightX = axes[2] || 0;
        if (Math.abs(rightX) > AXIS_THRESHOLD && window.nedflixApp) {
            const video = window.nedflixApp.videoPlayer;
            if (video && !video.paused) {
                video.currentTime += rightX * 0.5; // Seek 0.5 seconds per tick
            }
        }
    }

    function updateCurrentView() {
        const librarySelector = document.getElementById('library-selector');
        const mainContent = document.getElementById('main-content');
        const settingsPanel = document.getElementById('user-panel-overlay');

        if (settingsPanel && settingsPanel.classList.contains('active')) {
            currentView = 'settings';
        } else if (mainContent && mainContent.style.display !== 'none') {
            if (mainContent.classList.contains('video-playing')) {
                currentView = 'player';
            } else {
                currentView = 'browser';
            }
        } else {
            currentView = 'library';
        }
    }

    function navigateFocus(direction, axis) {
        updateCurrentView();

        let items = [];
        let columns = 1;

        switch (currentView) {
            case 'library':
                items = document.querySelectorAll('.library-card');
                columns = 4; // Grid has 4 columns
                break;
            case 'browser':
                items = document.querySelectorAll('#file-list .file-item');
                columns = 1;
                break;
            case 'settings':
                items = document.querySelectorAll('#user-panel select, #user-panel input, #user-panel button');
                columns = 1;
                break;
            case 'player':
                // In player view, arrow keys control video
                if (window.nedflixApp) {
                    const video = window.nedflixApp.videoPlayer;
                    if (video) {
                        if (axis === 'horizontal') {
                            video.currentTime += direction * 10;
                        } else {
                            video.volume = Math.max(0, Math.min(1, video.volume + direction * -0.1));
                        }
                    }
                }
                return;
        }

        if (items.length === 0) return;

        // Calculate new focus index
        if (axis === 'horizontal') {
            focusedIndex += direction;
        } else {
            focusedIndex += direction * columns;
        }

        // Wrap around
        if (focusedIndex < 0) focusedIndex = items.length - 1;
        if (focusedIndex >= items.length) focusedIndex = 0;

        // Apply focus
        items.forEach((item, i) => {
            item.classList.toggle('gamepad-focus', i === focusedIndex);
            if (i === focusedIndex) {
                item.focus();
                item.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
            }
        });

        // Vibration feedback
        vibrateShort();
    }

    function handleConfirm() {
        const focused = document.querySelector('.gamepad-focus');
        if (focused) {
            focused.click();
            vibrateMedium();
        }
    }

    function handleBack() {
        updateCurrentView();

        switch (currentView) {
            case 'settings':
                document.getElementById('user-panel-overlay')?.classList.remove('active');
                break;
            case 'player':
                document.getElementById('main-content')?.classList.remove('video-playing');
                break;
            case 'browser':
                if (window.nedflixApp) {
                    window.nedflixApp.backToLibraries();
                }
                break;
        }
        vibrateShort();
    }

    function handlePlayPause() {
        if (window.nedflixApp) {
            window.nedflixApp.togglePlayPause();
            vibrateMedium();
        }
    }

    function handleFullscreen() {
        if (window.nedflixApp) {
            window.nedflixApp.toggleFullscreen();
            vibrateMedium();
        }
    }

    function handlePrevious() {
        if (window.nedflixApp) {
            window.nedflixApp.playPreviousFile();
            vibrateMedium();
        }
    }

    function handleNext() {
        if (window.nedflixApp) {
            window.nedflixApp.playNextFile();
            vibrateMedium();
        }
    }

    function handleVolumeUp() {
        if (window.nedflixApp) {
            const video = window.nedflixApp.videoPlayer;
            if (video) {
                video.volume = Math.min(1, video.volume + 0.1);
                vibrateShort();
            }
        }
    }

    function handleVolumeDown() {
        if (window.nedflixApp) {
            const video = window.nedflixApp.videoPlayer;
            if (video) {
                video.volume = Math.max(0, video.volume - 0.1);
                vibrateShort();
            }
        }
    }

    function handleSettings() {
        const settingsPanel = document.getElementById('user-panel-overlay');
        if (settingsPanel) {
            settingsPanel.classList.toggle('active');
            vibrateShort();
        }
    }

    // Vibration helpers
    function vibrateShort() {
        if (window.nedflixDesktop && window.nedflixApp?.settings?.controller?.vibration) {
            window.nedflixDesktop.gamepad.vibrate(gamepadIndex, 50, 0.2, 0.1);
        }
    }

    function vibrateMedium() {
        if (window.nedflixDesktop && window.nedflixApp?.settings?.controller?.vibration) {
            window.nedflixDesktop.gamepad.vibrate(gamepadIndex, 100, 0.4, 0.2);
        }
    }

    // UI indicators
    function showGamepadIndicator(name) {
        const indicator = document.getElementById('gamepad-indicator');
        const nameEl = document.getElementById('gamepad-name');
        if (indicator && nameEl) {
            nameEl.textContent = formatControllerName(name);
            indicator.style.display = 'block';
            setTimeout(() => {
                indicator.style.display = 'none';
            }, 3000);
        }
    }

    function hideGamepadIndicator() {
        const indicator = document.getElementById('gamepad-indicator');
        if (indicator) {
            indicator.style.display = 'none';
        }
    }

    function updateControllerStatus(name) {
        const statusEl = document.getElementById('controller-status');
        if (statusEl) {
            if (name) {
                statusEl.textContent = formatControllerName(name);
                statusEl.style.color = 'var(--color-success)';
            } else {
                statusEl.textContent = 'No controller detected';
                statusEl.style.color = '';
            }
        }
    }

    function formatControllerName(name) {
        if (!name) return 'Unknown Controller';

        // Simplify common controller names
        if (name.includes('Xbox')) return 'Xbox Controller';
        if (name.includes('PlayStation') || name.includes('DualShock') || name.includes('DualSense')) {
            return 'PlayStation Controller';
        }
        if (name.includes('Nintendo') || name.includes('Pro Controller')) {
            return 'Nintendo Controller';
        }

        // Truncate long names
        if (name.length > 30) {
            return name.substring(0, 27) + '...';
        }

        return name;
    }

    // Add gamepad focus styles
    const style = document.createElement('style');
    style.textContent = `
        .gamepad-focus {
            outline: 3px solid var(--color-primary) !important;
            outline-offset: 2px;
            box-shadow: 0 0 20px var(--color-primary-glow) !important;
        }

        .library-card.gamepad-focus {
            transform: translateY(-4px) scale(1.02);
        }

        .file-item.gamepad-focus {
            background: var(--color-primary-glow) !important;
        }
    `;
    document.head.appendChild(style);

})();
