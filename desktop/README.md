# Nedflix Desktop

A standalone Windows desktop application for personal video streaming without authentication.

## Features

- **No Authentication Required**: Direct access to your media libraries
- **Xbox Controller Support**: Full gamepad navigation and playback control
- **Media Key Support**: Play/pause, next, previous using keyboard media keys
- **Dark/Light Themes**: Toggle between themes in settings
- **Video & Audio Playback**: Support for MP4, MKV, AVI, MOV, MP3, FLAC, and more

## Controller Button Mapping

| Button | Action |
|--------|--------|
| A | Select/Confirm |
| B | Back/Cancel |
| X / Start | Play/Pause |
| Y | Toggle Fullscreen |
| LB | Previous File |
| RB | Next File |
| LT | Volume Down |
| RT | Volume Up |
| Back | Open Settings |
| D-Pad / Left Stick | Navigate |
| Right Stick | Seek (while playing) |

## Keyboard Shortcuts

| Key | Action |
|-----|--------|
| Space | Play/Pause |
| F / F11 | Toggle Fullscreen |
| Escape | Back / Close Panel |
| Arrow Left/Right | Seek ±10 seconds |
| Arrow Up/Down | Volume Up/Down |
| M | Toggle Mute |

## Configuration

### Media Paths

Set the `NEDFLIX_MEDIA_PATHS` environment variable to configure your media directories:

```
NEDFLIX_MEDIA_PATHS=C:\Videos;D:\Movies;D:\TV Shows
```

Multiple paths are separated by semicolons.

### Default Paths

If no environment variable is set, these default paths are used:
- `C:\Videos`
- `D:\Movies`
- `D:\TV Shows`

## Building

### Prerequisites

- Node.js 18 or later
- Windows 10 or later (for building Windows installers)

### Quick Build (Windows)

1. Double-click `build.bat`
2. Select build option:
   - **1**: Windows Installer (x64)
   - **2**: Windows Installer (x86)
   - **3**: Portable Version
   - **4**: Build All

### Manual Build

```bash
# Install dependencies
npm install

# Build Windows x64 installer
npm run build:win

# Build Windows x86 installer
npm run build:win32

# Build portable version
npm run build:portable

# Build all targets
npm run build
```

### Development

```bash
# Start in development mode
npm run dev

# Or just run
npm start
```

## Output

Built files are placed in the `dist` folder:
- `Nedflix Setup x.x.x.exe` - Windows installer
- `Nedflix-Portable-x.x.x.exe` - Portable executable

## Project Structure

```
desktop/
├── main.js          # Electron main process
├── preload.js       # Secure API bridge
├── package.json     # Build configuration
├── build.bat        # Windows build script
├── public/
│   ├── index.html   # Desktop UI
│   ├── styles.css   # Styling
│   ├── app.js       # Application logic
│   └── gamepad.js   # Controller support
└── dist/            # Build output (generated)
```

## License

MIT
