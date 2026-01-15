<p align="center">
  <img src="https://img.shields.io/badge/Nedflix-Personal%20Streaming-E50914?style=for-the-badge&logo=data:image/svg+xml;base64,PHN2ZyB2aWV3Qm94PSIwIDAgMjQgMjQiIGZpbGw9IndoaXRlIiB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciPjxwb2x5Z29uIHBvaW50cz0iNSAzIDE5IDEyIDUgMjEgNSAzIj48L3BvbHlnb24+PC9zdmc+" alt="Nedflix">
</p>

<h1 align="center">Nedflix</h1>

<p align="center">
  <strong>Your personal video streaming platform</strong><br>
  Stream your media library with style - Web, Desktop, or Retro Consoles
</p>

<p align="center">
  <img src="https://img.shields.io/badge/node-%3E%3D14.0.0-brightgreen?style=flat-square" alt="Node">
  <img src="https://img.shields.io/badge/docker-ready-blue?style=flat-square&logo=docker" alt="Docker">
  <img src="https://img.shields.io/badge/electron-desktop-47848F?style=flat-square&logo=electron" alt="Electron">
  <img src="https://img.shields.io/badge/license-MIT-green?style=flat-square" alt="License">
</p>

---

## Features

- **Multi-User Support** - User accounts with individual permissions and profiles
- **Media Streaming** - Movies, TV Shows, Music, and Audiobooks with metadata
- **Live TV (IPTV)** - M3U playlists with XMLTV EPG support
- **Auto-Channels** - 24/7 streaming channels via [ErsatzTV](https://ersatztv.org/) integration
- **Automatic Subtitles** - OpenSubtitles API integration
- **Xbox Controller** - Full gamepad navigation (Desktop app)
- **Audio Visualizer** - Multiple visualization modes for music playback

---

## Quick Start

### Docker (Web Server)

```bash
git clone https://github.com/sp00nznet/nedflix.git
cd nedflix
cp .env.example .env
# Edit .env with your credentials
docker compose up -d
# Access at https://localhost:3443
```

### Desktop App (Windows/Linux)

```bash
cd nedflix/desktop

# Windows
build.bat

# Linux
chmod +x build.sh && ./build.sh
```

Output files are created in `desktop/dist/`.

---

## Platforms

### Modern Platforms

| Platform | Directory | Build Script | Notes |
|----------|-----------|--------------|-------|
| **Web (Docker)** | `/` | `docker compose up` | Full features, PostgreSQL, ErsatzTV |
| **Desktop** | `/desktop` | `build.bat` / `build.sh` | Windows/Linux, Xbox controller |
| **Xbox Series X/S** | `/xbox` | `build.bat` / `build.ps1` | UWP app, Dev Mode required |
| **iOS** | `/ios` | `build.sh` | macOS + Xcode required |
| **Android** | `/android` | `build.sh` | Android SDK required |
| **Apple TV** | `/appletv` | `build-*.sh` | macOS + Xcode required |
| **Android TV** | `/androidtv` | `build-*.sh` | Android SDK required |

### Retro Console Ports (Technical Demos)

| Platform | Directory | Build Script | SDK | Notes |
|----------|-----------|--------------|-----|-------|
| **Dreamcast** | `/ports/retro/dreamcast` | `build.sh` | KallistiOS | Audio streaming focus |
| **GameCube** | `/ports/retro/gamecube` | `build.sh` | devkitPPC | Audio playback only |
| **Xbox Original** | `/ports/retro/xbox-original` | `build.sh` / `build.bat` | nxdk | Full client, softmod required |
| **PlayStation 3** | `/ports/retro/ps3` | `build.sh` | PSL1GHT | Full HD client, CFW required |
| **Xbox 360** | `/ports/retro/xbox360` | `build.sh` | libxenon | Full HD client, JTAG/RGH required |

> **Note:** Retro console ports are experimental/novelty projects demonstrating homebrew development. They have hardware limitations and may require modified console firmware.

---

## Gamepad Controls

| Button | Action |
|--------|--------|
| **A** | Select |
| **B** | Back |
| **X / Start** | Play/Pause |
| **Y** | Fullscreen |
| **LB / RB** | Prev/Next |
| **LT / RT** | Volume |
| **D-Pad / Stick** | Navigate |

---

## Configuration

### Environment Variables (Docker)

| Variable | Description |
|----------|-------------|
| `SESSION_SECRET` | Session encryption key |
| `ADMIN_USERNAME` / `ADMIN_PASSWORD` | Local admin credentials |
| `NFS_PATH` | Media library path |
| `ERSATZTV_URL` | ErsatzTV API URL |
| `GOOGLE_CLIENT_ID` | OAuth (optional) |
| `OPENSUBTITLES_API_KEY` | Subtitles (optional) |
| `OMDB_API_KEY` | Metadata (optional) |

### Desktop Settings

Configure in the Settings panel:
- **Media Paths** - Directories to scan
- **IPTV** - M3U playlist and EPG URLs
- **ErsatzTV** - Server URL for auto-channels
- **Theme** - Dark/Light mode

Config stored in:
- Windows: `%APPDATA%/nedflix/nedflix-config.json`
- Linux: `~/.config/nedflix/nedflix-config.json`

---

## Documentation

See **[docs/SETUP.md](docs/SETUP.md)** for detailed instructions:
- SSL certificates and OAuth setup
- ErsatzTV channel configuration
- User management
- Troubleshooting

---

## Project Structure

```
nedflix/
├── server.js              # Express server
├── docker-compose.yml     # Docker orchestration
├── public/                # Web UI
├── desktop/               # Electron app (Windows/Linux)
├── ios/                   # iOS app (Swift/UIKit)
├── android/               # Android app (Kotlin/Compose)
├── xbox/                  # Xbox Series X/S (UWP)
├── appletv/               # Apple TV
├── androidtv/             # Android TV
├── ports/retro/           # Retro console ports
│   ├── dreamcast/         # Sega Dreamcast (KallistiOS)
│   ├── gamecube/          # Nintendo GameCube (devkitPPC)
│   ├── xbox-original/     # Original Xbox (nxdk)
│   ├── ps3/               # PlayStation 3 (PSL1GHT)
│   └── xbox360/           # Xbox 360 (libxenon)
└── docs/                  # Documentation
```

---

## License

Open source for personal and educational use.

<p align="center">
  <sub>Built for movie nights</sub>
</p>
