/**
 * Nedflix Desktop - Preload Script
 * Exposes safe APIs to renderer process
 */

const { contextBridge, ipcRenderer } = require('electron');

// Expose protected methods to renderer
contextBridge.exposeInMainWorld('nedflixDesktop', {
    // Check if running in desktop mode
    isDesktop: true,

    // Get configured media paths
    getMediaPaths: () => ipcRenderer.invoke('get-media-paths'),

    // Set media paths (save to config)
    setMediaPaths: (paths) => ipcRenderer.invoke('set-media-paths', paths),

    // Toggle fullscreen
    toggleFullscreen: () => ipcRenderer.invoke('toggle-fullscreen'),

    // Listen for fullscreen changes
    onFullscreenChange: (callback) => {
        ipcRenderer.on('fullscreen-change', (event, isFullscreen) => {
            callback(isFullscreen);
        });
    },

    // Listen for media key presses
    onMediaKey: (callback) => {
        ipcRenderer.on('media-key', (event, key) => {
            callback(key);
        });
    },

    // Gamepad API wrapper for Xbox controller
    gamepad: {
        // Poll gamepad state
        getGamepads: () => navigator.getGamepads(),

        // Vibrate controller (if supported)
        vibrate: (index, duration, weakMagnitude, strongMagnitude) => {
            const gamepads = navigator.getGamepads();
            const gamepad = gamepads[index];
            if (gamepad && gamepad.vibrationActuator) {
                gamepad.vibrationActuator.playEffect('dual-rumble', {
                    duration: duration || 200,
                    weakMagnitude: weakMagnitude || 0.5,
                    strongMagnitude: strongMagnitude || 0.5
                });
            }
        }
    }
});

// Gamepad event listeners
window.addEventListener('gamepadconnected', (e) => {
    console.log('Gamepad connected:', e.gamepad.id);
});

window.addEventListener('gamepaddisconnected', (e) => {
    console.log('Gamepad disconnected:', e.gamepad.id);
});
