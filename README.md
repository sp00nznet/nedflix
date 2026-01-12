<p align="center">
  <img src="https://img.shields.io/badge/Nedflix-Personal%20Streaming-E50914?style=for-the-badge&logo=data:image/svg+xml;base64,PHN2ZyB2aWV3Qm94PSIwIDAgMjQgMjQiIGZpbGw9IndoaXRlIiB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciPjxwb2x5Z29uIHBvaW50cz0iNSAzIDE5IDEyIDUgMjEgNSAzIj48L3BvbHlnb24+PC9zdmc+" alt="Nedflix">
</p>

<h1 align="center">Nedflix</h1>

<p align="center">
  <strong>Your personal video streaming platform</strong><br>
  Stream your media library with style - Web or Desktop
</p>

<p align="center">
  <img src="https://img.shields.io/badge/node-%3E%3D14.0.0-brightgreen?style=flat-square" alt="Node">
  <img src="https://img.shields.io/badge/docker-ready-blue?style=flat-square&logo=docker" alt="Docker">
  <img src="https://img.shields.io/badge/electron-desktop-47848F?style=flat-square&logo=electron" alt="Electron">
  <img src="https://img.shields.io/badge/windows-supported-0078D6?style=flat-square&logo=windows" alt="Windows">
  <img src="https://img.shields.io/badge/linux-supported-FCC624?style=flat-square&logo=linux&logoColor=black" alt="Linux">
  <img src="https://img.shields.io/badge/ErsatzTV-integrated-purple?style=flat-square" alt="ErsatzTV">
  <img src="https://img.shields.io/badge/license-MIT-green?style=flat-square" alt="License">
</p>

---

## Features

### Core Features
- **Multi-User Support** - Create and manage multiple users with individual permissions
- **Admin Panel** - Dedicated admin interface for user management and system controls
- **Media Search** - Fast indexed search across your entire library
- **Library Categories** - Separate Movies, TV Shows, Music, and Audiobooks
- **Secure Authentication** - Local admin login or OAuth (Google/GitHub)

### Streaming Features
- **Video Streaming** - Browse and stream from local or NFS-mounted directories
- **Automatic Subtitles** - Search and download via OpenSubtitles API
- **Multi-Audio Support** - Switch between audio tracks (requires FFmpeg)
- **Metadata Fetching** - Automatic movie/TV show information from OMDb and TVmaze
- **Audio Visualizer** - Multiple visualization modes (bars, wave, circular, particles)

### Live TV & Channels
- **Live TV (IPTV)** - Watch live streams via M3U playlists and XMLTV EPG
- **Auto-Generated Channels** - 24/7 streaming channels from your media library via ErsatzTV
- **Channel Guide** - Program schedule and now playing information

### Desktop Application
- **Windows & Linux** - Native installers for Windows (NSIS) and Linux (Debian, AppImage)
- **Xbox Controller Support** - Full gamepad navigation and control
- **Editable Media Paths** - Configure custom media directories in settings
- **Live TV (IPTV)** - Built-in IPTV support with M3U playlist and EPG
- **Auto-Channels** - Connect to ErsatzTV for 24/7 streaming channels
- **Media Key Support** - Play/pause, next/previous track, F11 fullscreen
- **No Server Required** - Standalone app with embedded streaming server
- **Auto Node.js Install** - Build scripts automatically install Node.js if missing

### User Experience
- **User Profiles** - Customizable avatars and streaming preferences
- **Dark/Light Theme** - Modern, responsive interface
- **Persistent Database** - PostgreSQL for reliable data storage

---

## Deployment Options

Nedflix can be deployed in two ways:

| Option | Best For | Features |
|--------|----------|----------|
| **Docker (Web)** | Home servers, multi-user | Full features, PostgreSQL, ErsatzTV, remote access |
| **Desktop App (Windows)** | Personal use, HTPC | Xbox controller, IPTV, ErsatzTV channels, local media |
| **Desktop App (Linux)** | Personal use, HTPC | Debian/AppImage, IPTV, ErsatzTV channels, local media |

---

## Quick Start

### Option 1: Docker Compose (Web Server)

```bash
# Clone the repository
git clone https://github.com/sp00nznet/nedflix.git
cd nedflix

# Configure environment
cp .env.example .env
nano .env  # Add your credentials

# Generate SSL certificates (or use your own)
npm run generate-certs

# Launch (includes PostgreSQL and ErsatzTV)
docker compose up -d

# Access at https://localhost:3443
```

### Option 2: Windows Desktop App

```bash
# Navigate to desktop folder
cd nedflix/desktop

# Run the build script (auto-installs Node.js if needed)
build.bat

# Choose option 1 for Windows x64 installer
# Or option 3 for portable executable
```

The installer will be created in `desktop/dist/`.

### Option 3: Linux Desktop App

```bash
# Navigate to desktop folder
cd nedflix/desktop

# Make the build script executable and run it
chmod +x build.sh
./build.sh

# Choose:
# 1 for Debian Package (x64)
# 2 for Debian Package (ARM64)
# 3 for AppImage (x64)
# 4 for tar.gz Archive
```

Packages will be created in `desktop/dist/`.

---

## Architecture

### Web Deployment (Docker)

```
┌───────────────────────────────────────────────────────────────┐
│                         Docker                                 │
│                                                                │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────────┐    │
│  │   Nedflix   │    │  ErsatzTV   │    │   PostgreSQL    │    │
│  │  (Node.js)  │◄──►│  (Channels) │    │   (Database)    │    │
│  │  Port 3443  │    │  Port 8409  │    │   Port 5432     │    │
│  └──────┬──────┘    └──────┬──────┘    └────────┬────────┘    │
│         │                  │                    │              │
│         ▼                  ▼                    ▼              │
│  ┌────────────────────────────────┐    ┌─────────────────┐    │
│  │        NFS Volume              │    │    DB Volume    │    │
│  │        (read-only)             │    │   (persistent)  │    │
│  └────────────────────────────────┘    └─────────────────┘    │
└───────────────────────────────────────────────────────────────┘
```

### Desktop Application (Electron)

```
┌─────────────────────────────────────────────────────┐
│              Nedflix Desktop (Electron)              │
│                                                      │
│  ┌──────────────────┐    ┌───────────────────────┐  │
│  │   Main Process   │    │   Renderer Process    │  │
│  │                  │    │                       │  │
│  │  - Express Server│◄──►│  - UI (HTML/CSS/JS)   │  │
│  │  - IPC Handlers  │    │  - Gamepad Support    │  │
│  │  - Media Keys    │    │  - Audio Visualizer   │  │
│  │  - Config Store  │    │  - Settings Panel     │  │
│  └────────┬─────────┘    └───────────────────────┘  │
│           │                                          │
│           ▼                                          │
│  ┌──────────────────────────────────────────────┐   │
│  │         Local Media Directories               │   │
│  │   C:\Videos, D:\Movies, Custom Paths...       │   │
│  └──────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────┘
```

---

## Desktop Application

The desktop version is a standalone application built with Electron for Windows and Linux.

### Features

| Feature | Description |
|---------|-------------|
| **No Server Setup** | Embedded Express server, just run and play |
| **Cross-Platform** | Windows (installer/portable) and Linux (Debian/AppImage/tar.gz) |
| **Editable Media Paths** | Add/remove media directories in Settings |
| **Xbox Controller** | Full navigation with gamepad support |
| **Media Keys** | Hardware play/pause, skip, volume controls |
| **Live TV (IPTV)** | Configure M3U playlist and EPG URLs for IPTV |
| **Channels (ErsatzTV)** | Connect to ErsatzTV server for auto-generated channels |
| **Audio Visualizer** | Bars, waveform, circular, and particle modes |
| **Portable Mode** | Optional portable executable (Windows) or tar.gz (Linux) |

### Xbox/Gamepad Controls

| Button | Action |
|--------|--------|
| **A** | Select / Confirm |
| **B** | Back / Cancel |
| **X / Start** | Play / Pause |
| **Y** | Toggle Fullscreen |
| **LB / RB** | Previous / Next File |
| **LT / RT** | Volume Down / Up |
| **Back** | Open Settings |
| **D-Pad** | Navigate menus |
| **Left Stick** | Navigate (with repeat) |
| **Right Stick** | Seek video (while playing) |

### Building the Desktop App

The build process auto-installs Node.js if not present:

**Windows:**
```bash
cd desktop
build.bat

# Or run specific commands:
npm run build:win      # Windows x64 installer
npm run build:win32    # Windows x86 installer
npm run build:portable # Portable executable
npm run dev            # Development mode with DevTools
```

**Linux:**
```bash
cd desktop
chmod +x build.sh
./build.sh

# Or run specific commands:
npm run build:linux    # All Linux formats
npm run build:deb      # Debian package only
npm run build:appimage # AppImage only
npm run dev            # Development mode with DevTools
```

**Output files** (in `desktop/dist/`):
- Windows: `Nedflix Setup x.x.x.exe`, `Nedflix-Portable-x.x.x.exe`
- Linux: `nedflix_x.x.x_amd64.deb`, `Nedflix-x.x.x.AppImage`, `nedflix-x.x.x.tar.gz`

### Media Path Configuration

The desktop app stores media paths in:
- **Windows:** `%APPDATA%/nedflix/nedflix-config.json`
- **Linux:** `~/.config/nedflix/nedflix-config.json`
- **Environment:** `NEDFLIX_MEDIA_PATHS` (semicolon-separated on Windows, colon-separated on Linux)

Configure paths in the Settings panel:
1. Click the user menu (top right)
2. Scroll to "Media Paths"
3. Click "Add Path" and select directories
4. Paths are saved automatically

---

## Auto-Generated Channels

Nedflix integrates with [ErsatzTV](https://ersatztv.org/) to create 24/7 streaming channels from your media library.

### How It Works

```
┌─────────────────┐      ┌─────────────────┐      ┌─────────────────┐
│   Your Media    │      │    ErsatzTV     │      │    Nedflix      │
│                 │      │                 │      │                 │
│  /Movies        │─────►│  Creates        │─────►│  Displays       │
│  /TV Shows      │      │  Libraries &    │      │  Channels UI    │
│  /Music         │      │  24/7 Channels  │      │  with Player    │
└─────────────────┘      └─────────────────┘      └─────────────────┘
```

### Default Channels

| Channel | Number | Content | Playback Mode |
|---------|--------|---------|---------------|
| **Movies 24/7** | 1 | All movies | Shuffled |
| **TV Shows Marathon** | 2 | All TV episodes | Shuffled |
| **Music Videos** | 3 | All music files | Shuffled |

### Setup

1. **Navigate to Channels** - Click the "Channels" card on the home screen
2. **Auto-Setup** - Admin users click "Setup Auto-Channels" button
3. **Wait for Scan** - ErsatzTV scans your media library
4. **Start Watching** - Channels appear automatically

---

## Library Cards

<table>
<tr>
<td width="33%">

### Movies
Browse and stream your movie collection with poster art and metadata.

</td>
<td width="33%">

### TV Shows
Navigate seasons and episodes with show information and artwork.

</td>
<td width="33%">

### Music
Play your music library with audio visualizer options.

</td>
</tr>
<tr>
<td>

### Audiobooks
Listen to your audiobook collection with playback controls.

</td>
<td>

### Live TV
Watch IPTV streams with M3U playlist support and EPG guide.

</td>
<td>

### Channels
24/7 auto-generated channels from your media via ErsatzTV.

</td>
</tr>
</table>

---

## Configuration

### Web (Docker) Environment Variables

| Variable | Description |
|----------|-------------|
| `SESSION_SECRET` | Random string for session encryption |
| `ADMIN_USERNAME` | Local admin username |
| `ADMIN_PASSWORD` | Local admin password |
| `NFS_PATH` | Path to your video library |
| `ERSATZTV_URL` | ErsatzTV API URL (default: http://ersatztv:8409) |
| `TIMEZONE` | Timezone for ErsatzTV EPG (default: America/New_York) |
| `GOOGLE_CLIENT_ID` | Google OAuth client ID (optional) |
| `OPENSUBTITLES_API_KEY` | For automatic subtitles (optional) |
| `OMDB_API_KEY` | For movie/TV metadata (optional) |

### Desktop Configuration

| Setting | Location | Description |
|---------|----------|-------------|
| Media Paths | Settings Panel | Directories to scan for media |
| IPTV Playlist | Settings Panel | M3U/M3U8 URL for Live TV |
| IPTV EPG | Settings Panel | XMLTV URL for program guide |
| ErsatzTV URL | Settings Panel | Server URL for auto-generated channels |
| Theme | Settings Panel | Dark or Light mode |
| Controller | Settings Panel | Enable/disable vibration feedback |

---

## API Endpoints

### Channels API (ErsatzTV)

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/api/ersatztv/status` | GET | Get ErsatzTV status and channels |
| `/api/ersatztv/channels` | GET | List all channels |
| `/api/ersatztv/setup` | POST | Setup auto-channels (admin) |
| `/api/ersatztv/libraries` | GET | List ErsatzTV libraries |
| `/api/ersatztv/playlist-url` | GET | Get M3U playlist URL |
| `/api/ersatztv/epg-url` | GET | Get XMLTV EPG URL |

### Live TV API (IPTV)

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/api/iptv/channels` | GET | Get IPTV channels |
| `/api/iptv/epg` | GET | Get EPG data |
| `/api/iptv/stream` | GET | Proxy IPTV stream |

---

## Requirements

### Web Deployment

| Component | Required | Notes |
|-----------|----------|-------|
| Docker | Recommended | Or Node.js v14+ |
| Auth | Yes | Local admin OR OAuth provider |
| PostgreSQL | Included | Via Docker Compose |
| ErsatzTV | Included | Via Docker Compose (for Channels) |
| FFmpeg | Optional | For audio track switching |

### Desktop Application

| Component | Required | Notes |
|-----------|----------|-------|
| Windows | Supported | Windows 10/11 (x64 or x86) |
| Linux | Supported | Debian/Ubuntu, Fedora, Arch (x64 or ARM64) |
| Node.js | Auto-installed | v20.11.0 (via build script) |
| Xbox Controller | Optional | For gamepad navigation |
| ErsatzTV Server | Optional | For auto-generated channels feature |

---

## Project Structure

```
nedflix/
├── server.js              # Main Express server (web)
├── ersatztv-service.js    # ErsatzTV API integration
├── iptv-service.js        # IPTV/Live TV service
├── db.js                  # Database abstraction layer
├── docker-compose.yml     # Docker orchestration
├── public/                # Web UI files
│   ├── index.html
│   ├── app.js
│   ├── livetv.js
│   ├── channels.js
│   └── styles.css
├── desktop/               # Electron desktop app
│   ├── main.js            # Electron main process
│   ├── preload.js         # Secure IPC bridge
│   ├── package.json       # Desktop dependencies
│   ├── build.bat          # Windows build script
│   ├── build.sh           # Linux build script
│   ├── install.bat        # Node.js auto-installer (Windows)
│   └── public/
│       ├── index.html     # Desktop UI
│       ├── app.js         # Desktop app logic
│       ├── gamepad.js     # Xbox controller support
│       ├── livetv.js      # Live TV (IPTV) module
│       ├── channels.js    # Channels (ErsatzTV) module
│       └── styles.css     # Desktop styles
└── docs/
    └── SETUP.md           # Detailed setup guide
```

---

## Documentation

For detailed setup instructions, see **[docs/SETUP.md](docs/SETUP.md)**:

- SSL certificate configuration
- OAuth provider setup (Google/GitHub)
- Docker deployment options
- Desktop application build guide (Windows & Linux)
- Desktop Live TV (IPTV) configuration
- Desktop Channels (ErsatzTV) configuration
- Xbox controller configuration
- ErsatzTV setup and configuration
- User management guide
- Troubleshooting guide

---

## License

Open source for personal and educational use.

---

<p align="center">
  <sub>Built for movie nights | Web + Desktop (Windows & Linux) | Gamepad Ready | Live TV & Auto-Channels</sub>
</p>
