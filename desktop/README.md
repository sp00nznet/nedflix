# Nedflix Desktop

A cross-platform desktop application for personal video streaming without authentication. Supports Windows, Linux, and macOS.

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

**Windows:**
```
NEDFLIX_MEDIA_PATHS=C:\Videos;D:\Movies;D:\TV Shows
```

**Linux/macOS:**
```bash
export NEDFLIX_MEDIA_PATHS="/home/user/Videos:/mnt/movies:/mnt/tv"
```

Paths are separated by semicolons (Windows) or colons (Linux/macOS).

### Default Paths

If no environment variable is set, platform-specific defaults are used:

**Windows:**
- `C:\Videos`
- `D:\Movies`
- `D:\TV Shows`

**Linux:**
- `$HOME/Videos`
- `/mnt/media`

**macOS:**
- `$HOME/Movies`
- `/Volumes/Media`

## Building

### Prerequisites

- Node.js 20 or later
- Platform-specific requirements:
  - **Windows**: Windows 10 or later
  - **Linux**: Debian-based, Fedora, Arch, or openSUSE
  - **macOS**: macOS 10.15 (Catalina) or later

### Quick Build

#### Windows
1. Double-click `build.bat`
2. Select build option:
   - **1**: Windows Installer (x64)
   - **2**: Windows Installer (x86)
   - **3**: Portable Version
   - **4**: Build All

#### Linux
```bash
chmod +x build.sh
./build.sh
```
Select from: Debian Package, AppImage, tar.gz Archive

#### macOS
```bash
chmod +x build.sh
./build.sh
```
Select from: Intel DMG, Apple Silicon DMG, Universal DMG

### Manual Build Commands

```bash
# Install dependencies
npm install

# --- Windows ---
npm run build:win          # x64 installer
npm run build:win32        # x86 installer
npm run build:portable     # Portable version

# --- Linux ---
npm run build:linux        # All Linux formats
npm run build:deb          # Debian package (x64)
npm run build:appimage     # AppImage (x64)
npm run build:linux-arm    # ARM64 builds

# --- macOS ---
npm run build:mac          # Intel (x64) DMG
npm run build:mac-arm      # Apple Silicon (ARM64) DMG
npm run build:mac-universal # Universal binary DMG

# Build all platforms (requires platform-specific environment)
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

**Windows:**
- `Nedflix Setup x.x.x.exe` - Windows installer (x64)
- `Nedflix Setup x.x.x-ia32.exe` - Windows installer (x86)
- `Nedflix-Portable-x.x.x.exe` - Portable executable

**Linux:**
- `nedflix_x.x.x_amd64.deb` - Debian package (x64)
- `nedflix_x.x.x_arm64.deb` - Debian package (ARM64)
- `Nedflix-x.x.x-x64.AppImage` - AppImage
- `nedflix-x.x.x-x64.tar.gz` - tar.gz archive

**macOS:**
- `Nedflix-x.x.x-x64.dmg` - Intel disk image
- `Nedflix-x.x.x-arm64.dmg` - Apple Silicon disk image
- `Nedflix-x.x.x-universal.dmg` - Universal binary disk image

## Project Structure

```
desktop/
├── main.js              # Electron main process
├── preload.js           # Secure API bridge
├── package.json         # Build configuration
├── build.bat            # Windows build script
├── build.sh             # Linux/macOS build script
├── build/
│   ├── icon.ico         # Windows icon
│   ├── icon.icns        # macOS icon
│   ├── icons/           # Linux icons
│   └── entitlements.mac.plist  # macOS entitlements
├── public/
│   ├── index.html       # Desktop UI
│   ├── styles.css       # Styling
│   ├── app.js           # Application logic
│   └── gamepad.js       # Controller support
└── dist/                # Build output (generated)
```

## License

MIT
