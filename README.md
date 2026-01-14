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

| Platform | Directory | Build Script | Notes |
|----------|-----------|--------------|-------|
| **Web (Docker)** | `/` | `docker compose up` | Full features, PostgreSQL, ErsatzTV |
| **Desktop** | `/desktop` | `build.bat` / `build.sh` | Windows/Linux, Xbox controller |
| **Xbox Series X/S** | `/xbox` | `build.bat` | UWP app, Dev Mode required |
| **Original Xbox** | `/xbox-original` | `build.bat` / `build-*.sh` | nxdk toolchain, softmod required |
| **Apple TV** | `/appletv` | `build-*.sh` | Xcode required |
| **Android TV** | `/androidtv` | `build-*.sh` | Android SDK required |
| **Dreamcast** | `/dreamcast` | `build.bat` / `build-*.sh` | KallistiOS, novelty build |

> **Note:** Retro console builds (Original Xbox, Dreamcast) are experimental/novelty projects with hardware limitations.

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
├── xbox/                  # Xbox Series X/S (UWP)
├── xbox-original/         # Original Xbox (nxdk)
├── appletv/               # Apple TV
├── androidtv/             # Android TV
├── dreamcast/             # Sega Dreamcast (KallistiOS)
└── docs/                  # Documentation
```

---

## License

Open source for personal and educational use.

<p align="center">
  <sub>Built for movie nights</sub>
</p>
