# Nedflix Setup Guide

Complete setup and configuration documentation for Nedflix.

## Table of Contents

- [Prerequisites](#prerequisites)
- [Docker Deployment](#docker-deployment)
- [Manual Installation](#manual-installation)
- [Database Configuration](#database-configuration)
- [Authentication](#authentication)
- [Admin Panel](#admin-panel)
- [Media Library Setup](#media-library-setup)
- [ErsatzTV / Auto-Channels](#ersatztv--auto-channels)
- [Desktop Application](#desktop-application)
- [Environment Variables](#environment-variables)
- [Project Structure](#project-structure)
- [Security Considerations](#security-considerations)
- [Troubleshooting](#troubleshooting)

---

## Prerequisites

- Node.js (v14 or higher) - *or Docker*
- npm (Node Package Manager) - *or Docker*
- OpenSSL (for generating SSL certificates) - *included in Docker image*
- An NFS mount point or video directory (default: `/mnt/nfs`)
- FFmpeg and FFprobe (optional, for audio track selection)

---

## Docker Deployment

### Docker Compose (Recommended)

Docker Compose sets up the complete Nedflix stack:
- **Nedflix** - Main streaming application
- **PostgreSQL** - Persistent database
- **ErsatzTV** - Auto-generated 24/7 channels

Create a `.env` file with your settings:

```env
# Required: Session secret (use a long random string)
SESSION_SECRET=your-super-secret-random-string-here

# Required: Admin credentials
ADMIN_USERNAME=admin
ADMIN_PASSWORD=your-secure-password

# Path to your video files on the host
NFS_PATH=/path/to/your/videos

# PostgreSQL credentials (optional - defaults provided)
POSTGRES_USER=nedflix
POSTGRES_PASSWORD=nedflix_secret
POSTGRES_DB=nedflix

# ErsatzTV Configuration (optional - defaults provided)
ERSATZTV_URL=http://ersatztv:8409
TIMEZONE=America/New_York

# OAuth Providers (optional)
GOOGLE_CLIENT_ID=your-google-client-id
GOOGLE_CLIENT_SECRET=your-google-client-secret
GITHUB_CLIENT_ID=your-github-client-id
GITHUB_CLIENT_SECRET=your-github-client-secret

# Callback URL for OAuth (change for production)
CALLBACK_BASE_URL=https://localhost:3443

# Optional: API keys for enhanced features
OPENSUBTITLES_API_KEY=your-api-key
OMDB_API_KEY=your-omdb-key
```

Generate SSL certificates before starting (or provide your own):

```bash
# Generate self-signed certificates for development
npm run generate-certs

# Or copy your own certificates to ./certs/
# - server.key (private key)
# - server.cert (certificate)
```

Then run:

```bash
docker compose up -d
```

This starts three containers:
- `nedflix` - The main application (ports 3000, 3443)
- `nedflix-db` - PostgreSQL database (port 5432, internal only)
- `ersatztv` - Auto-channel generation (port 8409)

### Docker Run (Without Compose)

```bash
# Start PostgreSQL first
docker run -d \
  --name nedflix-db \
  -e POSTGRES_USER=nedflix \
  -e POSTGRES_PASSWORD=nedflix_secret \
  -e POSTGRES_DB=nedflix \
  -v nedflix-db-data:/var/lib/postgresql/data \
  postgres:16-alpine

# Build and run Nedflix
docker build -t nedflix .

docker run -d \
  --name nedflix \
  --link nedflix-db:db \
  -p 3000:3000 \
  -p 3443:3443 \
  -v /path/to/videos:/mnt/nfs:ro \
  -v nedflix-certs:/app/certs \
  -e SESSION_SECRET=your-secret \
  -e ADMIN_USERNAME=admin \
  -e ADMIN_PASSWORD=your-password \
  -e DB_HOST=db \
  -e DB_USER=nedflix \
  -e DB_PASSWORD=nedflix_secret \
  -e DB_NAME=nedflix \
  nedflix
```

### Docker Features

- **PostgreSQL database** for persistent storage
- **Host SSL certificates** - Mount your own certificates from the host
- **Persists data** in Docker volumes (database)
- **Read-only video mount** for security
- **Non-root user** inside container
- **Health checks** for container orchestration
- **Alpine-based** for minimal image size

### SSL Certificate Options

**Option 1: Self-signed (Development)**
```bash
npm run generate-certs
```

**Option 2: Let's Encrypt (Production)**

The server auto-detects Let's Encrypt certificate filenames (`privkey.pem` and `fullchain.pem`).

```bash
# Copy Let's Encrypt certs to local directory (recommended)
mkdir -p ./certs
cp /etc/letsencrypt/live/yourdomain.com/privkey.pem ./certs/
cp /etc/letsencrypt/live/yourdomain.com/fullchain.pem ./certs/

# Set permissions for container access (required!)
# The container runs as non-root user and needs read access
chmod 644 ./certs/privkey.pem ./certs/fullchain.pem
```

**Important:** Don't mount `/etc/letsencrypt/live/` directly - those files are symlinks that won't resolve inside the container. Copy the actual certificate files instead.

**Option 3: Custom path**
```env
CERTS_PATH=/path/to/your/certs
```

---

## Manual Installation

1. **Install dependencies:**
```bash
npm install
```

2. **Generate SSL certificates:**
```bash
npm run generate-certs
```
This creates self-signed certificates in `./certs/`. For production, use certificates from a trusted CA like Let's Encrypt.

3. **Configure environment variables:**
```bash
cp .env.example .env
```
Edit `.env` with your settings.

4. **Mount your NFS share (if not already mounted):**
```bash
sudo mount -t nfs server:/path/to/share /mnt/nfs
```

5. **Start the server:**
```bash
npm start
```

6. **Access the application:**
   - HTTPS: `https://localhost:3443`
   - HTTP requests are automatically redirected to HTTPS

**Note:** Without PostgreSQL configured, Nedflix uses SQLite for local development. Data is stored in `users.db` in the application directory.

---

## Database Configuration

### PostgreSQL (Production)

Nedflix uses PostgreSQL when the following environment variables are set:

```env
DB_HOST=localhost
DB_PORT=5432
DB_USER=nedflix
DB_PASSWORD=your-password
DB_NAME=nedflix
```

Or using a connection string:
```env
DATABASE_URL=postgresql://nedflix:password@localhost:5432/nedflix
```

### SQLite (Development)

When no PostgreSQL configuration is provided, Nedflix automatically uses SQLite. This is suitable for development and single-user scenarios.

### Database Tables

Nedflix creates and manages these tables:

| Table | Purpose |
|-------|---------|
| `users` | User accounts, credentials, permissions |
| `user_settings` | Streaming preferences, profile pictures |
| `file_index` | Media library index for fast browsing/search |
| `scan_logs` | History of library scan operations |
| `media_metadata` | Cached movie/TV show information |

---

## Authentication

Nedflix supports multiple authentication methods:

### Local Admin Account

The simplest way to get started. Add these to your `.env` file:

```env
ADMIN_USERNAME=admin
ADMIN_PASSWORD=your-secure-password
```

**Security notes:**
- Use a strong, unique password
- The admin account has full access to all features
- Can create additional users via the Admin Panel

### Database Users

Additional users can be created through the Admin Panel:

1. Log in as admin
2. Open Admin Panel (gear icon)
3. Go to "User Management"
4. Click "Add User"
5. Set username, password, and permissions

Database users can have:
- Admin or regular user status
- Custom library access (Movies, TV, Music, Audiobooks)
- Enabled/disabled status

### OAuth Providers (Optional)

#### Google OAuth

1. Go to [Google Cloud Console](https://console.cloud.google.com/)
2. Create a new project or select existing
3. Navigate to "APIs & Services" > "Credentials"
4. Click "Create Credentials" > "OAuth 2.0 Client IDs"
5. Set Application type to "Web application"
6. Add authorized redirect URI: `https://localhost:3443/auth/google/callback`
7. Copy Client ID and Client Secret to your `.env` file

#### GitHub OAuth

1. Go to [GitHub Developer Settings](https://github.com/settings/developers)
2. Click "New OAuth App"
3. Set Homepage URL: `https://localhost:3443`
4. Set Authorization callback URL: `https://localhost:3443/auth/github/callback`
5. Copy Client ID and Client Secret to your `.env` file

---

## Admin Panel

The Admin Panel provides centralized management for administrators.

### Accessing the Admin Panel

1. Log in with an admin account
2. Click the gear icon in the header (left of your profile picture)
3. The admin panel opens as an overlay

### User Management

- **View all users** - See username, display name, provider, admin status
- **Add users** - Create new local accounts with passwords
- **Edit users** - Change display name, password, admin status
- **Library access** - Control which libraries each user can access
- **Enable/disable** - Temporarily disable user accounts
- **Delete users** - Remove user accounts (except the main admin)

### Media Library Index

The media index enables fast browsing and search:

- **Scan Library** - Index all files in your media directory
- **View statistics** - Total files, videos, audio files
- **Last scan info** - When the library was last indexed
- **Scan logs** - History of all scan operations
- **Download logs** - Export scan logs as CSV

### Metadata Scanning

Fetch movie and TV show information from online databases:

- **Scan Movies** - Fetch metadata from OMDb for movies
- **Scan TV Shows** - Fetch metadata from OMDb and TVmaze for TV shows

Requires `OMDB_API_KEY` to be configured.

---

## Media Library Setup

### Directory Structure

Organize your media library with these top-level directories:

```
/mnt/nfs/
├── Movies/
│   ├── Movie Name (2020).mp4
│   └── Another Movie (2019)/
│       └── Another Movie (2019).mkv
├── TV Shows/
│   └── Show Name/
│       ├── Season 1/
│       │   ├── Show Name S01E01.mp4
│       │   └── Show Name S01E02.mp4
│       └── Season 2/
├── Music/
│   └── Artist/
│       └── Album/
│           └── track.mp3
└── Audiobooks/
    └── Book Name/
        └── chapters.mp3
```

### Library Categories

Each top-level directory maps to a library category:

| Directory | Category | Access Key |
|-----------|----------|------------|
| `Movies` | Movies | `movies` |
| `TV Shows` | TV Shows | `tv` |
| `Music` | Music | `music` |
| `Audiobooks` | Audiobooks | `audiobooks` |

### Indexing Your Library

After setting up your media:

1. Log in as admin
2. Open Admin Panel
3. Click "Scan Library" under Media Library Index
4. Wait for the scan to complete

The index enables:
- Fast file browsing (no disk access required)
- Instant search across your library
- Library-scoped search results

### Search Usage

1. Navigate to a library (Movies, TV Shows, etc.)
2. Use the search bar in the file browser header
3. Results are filtered to the current library
4. Click a result to navigate to its location

---

## ErsatzTV / Auto-Channels

Nedflix integrates with [ErsatzTV](https://ersatztv.org/) to create 24/7 streaming channels from your media library.

### What is ErsatzTV?

ErsatzTV is an open-source application that creates custom IPTV channels from your local media. It generates:
- Continuous 24/7 playback streams
- M3U playlists compatible with any IPTV player
- XMLTV EPG (Electronic Program Guide) data

### Architecture

```
┌─────────────────┐      ┌─────────────────┐      ┌─────────────────┐
│   NFS Media     │      │    ErsatzTV     │      │    Nedflix      │
│   /mnt/nfs      │◄────►│  Port 8409      │◄────►│  Port 3443      │
│                 │      │                 │      │                 │
│  - Movies/      │      │  Creates:       │      │  Displays:      │
│  - TV Shows/    │      │  - Libraries    │      │  - Channel List │
│  - Music/       │      │  - Channels     │      │  - Video Player │
│                 │      │  - M3U/EPG      │      │  - EPG Info     │
└─────────────────┘      └─────────────────┘      └─────────────────┘
```

### How It Works

1. **ErsatzTV scans** the same NFS mount as Nedflix
2. **Creates libraries** for Movies, TV Shows, and Music
3. **Generates channels** with shuffled content playback
4. **Nedflix displays** channels with a built-in player

### Default Channels

When you run "Setup Auto-Channels", these channels are created:

| Channel | Number | Content | Playback Mode |
|---------|--------|---------|---------------|
| Movies 24/7 | 1 | All movies | Shuffled |
| TV Shows Marathon | 2 | All TV episodes | Shuffled |
| Music Videos | 3 | All music files | Shuffled |

### Setting Up Channels

#### Quick Setup (Recommended)

1. **Start the Docker stack** - `docker compose up -d`
2. **Log in as admin** to Nedflix
3. **Click "Channels"** card on home screen
4. **Click "Setup Auto-Channels"** button
5. **Wait** for ErsatzTV to scan your media
6. **Enjoy** your 24/7 channels!

#### Manual ErsatzTV Configuration

For advanced users, you can access ErsatzTV directly:

1. **Open ErsatzTV UI** - `http://localhost:8409`
2. **Add Local Libraries**:
   - Movies: `/mnt/nfs/Movies`
   - TV Shows: `/mnt/nfs/TV Shows`
   - Music: `/mnt/nfs/Music`
3. **Create Channels** manually with custom names/numbers
4. **Configure Schedules** with specific content

### Configuration Options

| Variable | Default | Description |
|----------|---------|-------------|
| `ERSATZTV_URL` | `http://ersatztv:8409` | ErsatzTV API endpoint |
| `TIMEZONE` | `America/New_York` | Timezone for EPG scheduling |

### API Endpoints

Nedflix provides these endpoints for ErsatzTV integration:

| Endpoint | Method | Auth | Description |
|----------|--------|------|-------------|
| `/api/ersatztv/status` | GET | User | Get status, libraries, channels |
| `/api/ersatztv/health` | GET | User | Check ErsatzTV availability |
| `/api/ersatztv/channels` | GET | User | List all channels |
| `/api/ersatztv/setup` | POST | Admin | Auto-setup libraries and channels |
| `/api/ersatztv/libraries` | GET | User | List ErsatzTV libraries |
| `/api/ersatztv/libraries` | POST | Admin | Create a new library |
| `/api/ersatztv/libraries/:id/scan` | POST | Admin | Trigger library scan |
| `/api/ersatztv/channels` | POST | Admin | Create a new channel |
| `/api/ersatztv/channels/:id` | DELETE | Admin | Delete a channel |
| `/api/ersatztv/channels/:id/guide` | GET | User | Get channel program guide |
| `/api/ersatztv/channels/:id/rebuild` | POST | Admin | Rebuild channel playout |
| `/api/ersatztv/playlist-url` | GET | User | Get M3U playlist URL |
| `/api/ersatztv/epg-url` | GET | User | Get XMLTV EPG URL |

### Using Channels in External Players

ErsatzTV generates standard IPTV outputs that work with any compatible player:

**M3U Playlist:**
```
http://localhost:8409/iptv/channels.m3u
```

**XMLTV EPG:**
```
http://localhost:8409/iptv/xmltv.xml
```

Compatible players:
- VLC Media Player
- Kodi (with IPTV Simple Client)
- Jellyfin (with IPTV plugin)
- Plex (with IPTV plugin)
- Any M3U-compatible player

### Disabling ErsatzTV

If you don't need auto-channels, you can disable ErsatzTV:

1. Comment out the `ersatztv` service in `docker-compose.yml`
2. Remove the `ersatztv` dependency from the `nedflix` service
3. Run `docker compose up -d`

The "Channels" card will show "ErsatzTV unavailable" but Nedflix will function normally.

---

## Desktop Application

Nedflix includes a standalone Windows desktop application built with Electron. This is ideal for personal use, HTPCs, and living room setups with Xbox controllers.

### Features Overview

| Feature | Description |
|---------|-------------|
| **Standalone** | No server required - embedded Express server |
| **Xbox Controller** | Full gamepad navigation support |
| **Editable Media Paths** | Configure directories in Settings |
| **Media Keys** | Hardware play/pause, skip controls |
| **IPTV Support** | M3U playlist and EPG configuration |
| **Audio Visualizer** | Multiple visualization modes |
| **Portable Mode** | Optional no-install executable |
| **Auto Node.js Install** | Build scripts install Node.js automatically |

### Building the Desktop App

#### Prerequisites

The build script automatically installs Node.js if not present. Just run the script!

#### Using build.bat (Recommended)

```batch
cd desktop
build.bat
```

The interactive menu offers:
1. **Build Windows x64 installer** - For 64-bit Windows
2. **Build Windows x86 installer** - For 32-bit Windows
3. **Build portable version** - No installation required
4. **Build all versions** - Creates all variants
5. **Run development mode** - With DevTools enabled
6. **Exit**

#### Using npm Commands

```bash
cd desktop
npm install              # Install dependencies
npm run build:win        # Windows x64 installer
npm run build:win32      # Windows x86 installer
npm run build:portable   # Portable executable
npm run dev              # Development mode
npm start                # Production mode
```

#### Output Files

Built applications are placed in `desktop/dist/`:
- `Nedflix Setup x.x.x.exe` - NSIS installer with uninstaller
- `Nedflix-Portable-x.x.x.exe` - Standalone portable executable

### Xbox Controller Support

The desktop app includes comprehensive gamepad support for living room use.

#### Button Mapping

| Button | Action |
|--------|--------|
| **A** | Select / Confirm |
| **B** | Back / Cancel |
| **X** | Play / Pause |
| **Y** | Toggle Fullscreen |
| **Start** | Play / Pause |
| **Back/Select** | Open Settings |
| **LB** | Previous File |
| **RB** | Next File |
| **LT** | Volume Down |
| **RT** | Volume Up |
| **D-Pad Up/Down** | Navigate vertically |
| **D-Pad Left/Right** | Navigate horizontally |
| **Left Stick** | Navigate (with repeat delay) |
| **Right Stick X** | Seek video (while playing) |

#### Controller Features

- **60fps polling** for responsive input
- **Vibration feedback** with dual rumble support
- **Auto-detection** of controller connection/disconnection
- **Focus indicators** with visual glow effects
- **Multi-controller support** (Xbox, PlayStation, Nintendo)

#### Enabling Vibration

1. Open Settings (click user menu or press Back button)
2. Scroll to "Controller Settings"
3. Toggle "Controller Vibration" on

### Media Path Configuration

#### Adding Media Paths

1. Click the user menu (top right corner)
2. Scroll to "Media Paths" section
3. Click "Add Path"
4. Select a directory in the file dialog
5. The path is saved automatically

#### Removing Media Paths

1. Open Settings
2. Find the path in the Media Paths list
3. Click the X button next to the path

#### Configuration Storage

Paths are stored in:
- **Windows:** `%APPDATA%/nedflix/nedflix-config.json`

Example configuration file:
```json
{
  "mediaPaths": [
    "C:\\Videos",
    "D:\\Movies",
    "D:\\TV Shows",
    "E:\\Music"
  ]
}
```

#### Environment Variable

You can also set paths via environment variable:
```batch
set NEDFLIX_MEDIA_PATHS=C:\Videos;D:\Movies;D:\TV Shows
```

Paths are semicolon-separated on Windows.

### Live TV Configuration (Desktop)

The desktop app supports IPTV with configurable URLs:

1. Open Settings
2. Find "Live TV (IPTV)" section
3. Enter your playlist URL (M3U/M3U8)
4. Optionally enter EPG URL (XMLTV)
5. Settings are saved automatically

### Media Key Support

The desktop app responds to hardware media keys:

| Key | Action |
|-----|--------|
| **Play/Pause** | Toggle video playback |
| **Stop** | Stop playback |
| **Previous Track** | Go to previous file |
| **Next Track** | Go to next file |
| **F11** | Toggle fullscreen |

### Audio Visualizer

When playing audio files, the visualizer offers multiple modes:

| Mode | Description |
|------|-------------|
| **Bars** | Classic equalizer bars |
| **Wave** | Waveform visualization |
| **Circular** | Radial frequency display |
| **Particles** | Particle system reacting to audio |
| **None** | Disable visualizer |

Select the mode from the dropdown above the player.

### Desktop Architecture

```
Electron App
├── Main Process (main.js)
│   ├── BrowserWindow - Main app window
│   ├── Express Server - Media streaming API
│   ├── IPC Handlers - Config, fullscreen, paths
│   ├── Global Shortcuts - Media keys
│   └── Config Store - %APPDATA%/nedflix/
│
├── Preload Script (preload.js)
│   └── Secure IPC bridge (nedflixDesktop API)
│
└── Renderer Process (public/)
    ├── index.html - UI structure
    ├── app.js - Application logic
    ├── gamepad.js - Controller support
    └── styles.css - Styling
```

### Desktop API Endpoints

The embedded server provides these endpoints:

| Endpoint | Description |
|----------|-------------|
| `/api/libraries` | List configured media directories |
| `/api/browse?path=` | Browse directory contents |
| `/api/video?path=` | Stream video file (range requests) |
| `/api/audio?path=` | Stream audio file |
| `/api/user` | Mock user (always authenticated) |
| `/api/settings` | Get/set user settings |

### Security Features

- **Context Isolation** - Renderer cannot access Node modules
- **Preload Script** - Secure IPC bridge
- **Path Validation** - Prevents directory traversal
- **No Node Integration** - Enhanced security
- **Whitelisted Paths** - Only configured directories accessible

### Supported Formats

#### Video
- MP4, WebM, Ogg, AVI, MKV, MOV, M4V, WMV

#### Audio
- MP3, M4A, FLAC, WAV, AAC, OGG, WMA, Opus, AIFF

Actual playback depends on system codecs. MP4/WebM have best compatibility.

---

## Environment Variables

| Variable | Required | Default | Description |
|----------|----------|---------|-------------|
| `PORT` | No | `3000` | HTTP port (redirects to HTTPS) |
| `HTTPS_PORT` | No | `3443` | HTTPS port |
| `SESSION_SECRET` | Yes | - | Random string for session encryption |
| `ADMIN_USERNAME` | Yes* | - | Local admin username |
| `ADMIN_PASSWORD` | Yes* | - | Local admin password |
| `NFS_MOUNT_PATH` | No | `/mnt/nfs` | Path to video files |
| `CERTS_PATH` | No | `./certs` | Path to SSL certificates |
| `DB_HOST` | No | - | PostgreSQL host |
| `DB_PORT` | No | `5432` | PostgreSQL port |
| `DB_USER` | No | - | PostgreSQL username |
| `DB_PASSWORD` | No | - | PostgreSQL password |
| `DB_NAME` | No | - | PostgreSQL database name |
| `DATABASE_URL` | No | - | PostgreSQL connection string |
| `POSTGRES_USER` | No | `nedflix` | Docker Compose PostgreSQL user |
| `POSTGRES_PASSWORD` | No | `nedflix_secret` | Docker Compose PostgreSQL password |
| `POSTGRES_DB` | No | `nedflix` | Docker Compose PostgreSQL database |
| `GOOGLE_CLIENT_ID` | No | - | Google OAuth client ID |
| `GOOGLE_CLIENT_SECRET` | No | - | Google OAuth client secret |
| `GITHUB_CLIENT_ID` | No | - | GitHub OAuth client ID |
| `GITHUB_CLIENT_SECRET` | No | - | GitHub OAuth client secret |
| `CALLBACK_BASE_URL` | No | - | Base URL for OAuth callbacks |
| `OPENSUBTITLES_API_KEY` | No | - | API key for automatic subtitles |
| `OMDB_API_KEY` | No | - | API key for movie/TV metadata |
| `ERSATZTV_URL` | No | `http://ersatztv:8409` | ErsatzTV API endpoint |
| `TIMEZONE` | No | `America/New_York` | Timezone for ErsatzTV EPG |

*Admin credentials or OAuth provider required for authentication.

---

## Project Structure

```
nedflix/
├── server.js              # Main Express server (web)
├── db.js                  # Database abstraction layer (SQLite/PostgreSQL)
├── user-service.js        # User management service
├── media-service.js       # Media indexing and search service
├── metadata-service.js    # Movie/TV metadata fetching
├── iptv-service.js        # Live TV (IPTV) service
├── ersatztv-service.js    # ErsatzTV API integration for auto-channels
├── generate-certs.js      # SSL certificate generator
├── package.json           # Project dependencies
├── .env.example           # Environment template
├── Dockerfile             # Container build configuration
├── docker-compose.yml     # Docker Compose (Nedflix + PostgreSQL + ErsatzTV)
├── db-init/               # PostgreSQL initialization scripts
│   └── 01-schema.sql      # Database schema
├── certs/                 # SSL certificates (generated)
│   ├── server.key
│   └── server.cert
├── docs/
│   └── SETUP.md           # This file
├── public/                # Web application UI
│   ├── index.html         # Main application page
│   ├── login.html         # Login page
│   ├── styles.css         # Application styles
│   ├── app.js             # Core client-side JavaScript
│   ├── livetv.js          # Live TV (IPTV) functionality
│   ├── channels.js        # Channels (ErsatzTV) functionality
│   ├── thumbnails/        # Cached movie/TV posters
│   └── images/            # Avatar images
├── desktop/               # Electron desktop application
│   ├── main.js            # Electron main process
│   ├── preload.js         # Secure IPC bridge script
│   ├── package.json       # Desktop app configuration
│   ├── build.bat          # Windows build script (auto-installs Node.js)
│   ├── install.bat        # Node.js auto-installer utility
│   ├── README.md          # Desktop-specific documentation
│   └── public/            # Desktop UI files
│       ├── index.html     # Desktop application UI
│       ├── app.js         # Desktop application logic
│       ├── gamepad.js     # Xbox/gamepad controller support
│       └── styles.css     # Desktop styling
└── README.md
```

---

## Features Configuration

### Automatic Subtitles

Nedflix can automatically search and load subtitles from OpenSubtitles.

1. Get an API key from [OpenSubtitles](https://www.opensubtitles.com/)
2. Add to your `.env` file:
   ```env
   OPENSUBTITLES_API_KEY=your-api-key
   ```

Subtitles are cached in `subtitle_cache/` directory after first download.

### Media Metadata

Fetch movie and TV show information automatically.

1. Get an API key from [OMDb](https://www.omdbapi.com/)
2. Add to your `.env` file:
   ```env
   OMDB_API_KEY=your-api-key
   ```

Metadata includes:
- Clean titles
- Year, rating, genre
- Plot summaries
- Poster images
- Director, actors
- Episode information for TV shows

### Audio Track Selection

For videos with multiple audio tracks, Nedflix can detect and switch between them.

**Requirements:**
- FFprobe (for detecting audio tracks)
- FFmpeg (for transcoding with selected audio)

Install on Ubuntu/Debian:
```bash
sudo apt install ffmpeg
```

Install on macOS:
```bash
brew install ffmpeg
```

---

## User Settings

### Streaming Options

- **Video Quality**: Auto, High (1080p), Medium (720p), Low (480p)
- **Default Volume**: 0-100%
- **Playback Speed**: 0.5x to 2x
- **Autoplay**: Automatically play next video
- **Subtitles**: Show subtitles when available

### Profile Options

- Choose from 8 avatar pictures (cat, dog, cow, fox, owl, bear, rabbit, penguin)

---

## Supported Formats

### Video
- `.mp4`, `.webm`, `.ogg`, `.avi`, `.mkv`, `.mov`, `.m4v`, `.flv`

### Audio
- `.mp3`, `.flac`, `.wav`, `.m4a`, `.aac`, `.ogg`, `.wma`, `.opus`, `.aiff`

Actual playback support depends on your browser's codec support. MP4 and WebM have the best compatibility.

---

## Security Considerations

### For Development

The included self-signed certificates are for development only. Your browser will show a security warning - you can proceed for testing purposes.

### For Production

1. **Use proper SSL certificates** from a trusted CA (e.g., Let's Encrypt)
2. **Change the SESSION_SECRET** to a long, random string
3. **Set strong database passwords** for PostgreSQL
4. **Use strong admin passwords** - at least 12 characters
5. **Run behind a reverse proxy** (nginx, Apache) for additional security
6. **Update OAuth callback URLs** to your production domain
7. **Restrict network access** to PostgreSQL (internal only)
8. **Regular backups** of the database volume

---

## Troubleshooting

### Database Issues

**Tables not created:**
- Check PostgreSQL container logs: `docker compose logs nedflix-db`
- Ensure database credentials match between app and database
- The app creates tables automatically on startup

**Data not persisting:**
- Verify Docker volume exists: `docker volume ls | grep nedflix`
- Check volume mount in docker-compose.yml
- Don't use `docker compose down -v` (removes volumes)

**Connection refused:**
- Ensure PostgreSQL container is running
- Check `DB_HOST` matches container name or network alias
- Wait for database health check to pass before starting app

### Admin Panel Issues

**Can't see admin button:**
- Verify you're logged in as an admin user
- Check user's `is_admin` status in database
- Clear browser cache and re-login

**Scan not completing:**
- Check server logs for errors
- Verify NFS mount is accessible
- Large libraries may take several minutes

### Search Not Working

**No results:**
- Run a library scan from Admin Panel first
- Check if file index has data in Admin Panel stats
- Verify search query is at least 2 characters

**Wrong results:**
- Search is scoped to current library
- Navigate to correct library before searching

### SSL Certificate Errors
- For development, accept the self-signed certificate warning
- For production, use certificates from a trusted CA

### OAuth Callback Errors
- Ensure callback URLs in OAuth provider settings match your `.env` configuration
- Check that `CALLBACK_BASE_URL` includes the correct protocol and port

### Videos Won't Play
- Check browser console for errors
- Ensure the video format is supported by your browser
- Verify NFS mount permissions

### Docker Issues

**Container won't start:**
```bash
# Check logs
docker compose logs nedflix
docker compose logs nedflix-db

# Verify environment variables
docker compose config
```

**Database connection errors:**
```bash
# Check database is running
docker compose ps

# Test database connection
docker compose exec nedflix-db psql -U nedflix -d nedflix -c "SELECT 1"
```

**Reset database:**
```bash
# Warning: This deletes all data
docker compose down -v
docker compose up -d
```

**Certificate issues:**
```bash
# Regenerate self-signed certificates
npm run generate-certs

# Restart containers
docker compose restart nedflix

# Check certificate files exist
ls -la ./certs/
# Should show: server.key and server.cert
```

### ErsatzTV / Channels Issues

**Channels card shows "ErsatzTV unavailable":**
- Check ErsatzTV container is running: `docker compose ps ersatztv`
- View ErsatzTV logs: `docker compose logs ersatztv`
- Verify health check: `curl http://localhost:8409/api/health`
- Ensure `ERSATZTV_URL` is correct in environment

**No channels after setup:**
- ErsatzTV needs time to scan media (can take several minutes)
- Check ErsatzTV UI at `http://localhost:8409` for scan progress
- Verify media paths are accessible inside container
- Check ErsatzTV logs for errors: `docker compose logs ersatztv`

**Channels not playing:**
- Verify stream URL format in browser console
- Check if ErsatzTV has built playout for channel
- Try accessing M3U directly: `http://localhost:8409/iptv/channels.m3u`
- Ensure media files are playable format (MP4 recommended)

**Setup Auto-Channels fails:**
```bash
# Check ErsatzTV API is responding
curl http://localhost:8409/api/health

# View Nedflix logs for API errors
docker compose logs nedflix | grep -i ersatz

# Restart ErsatzTV
docker compose restart ersatztv
```

**Media not showing in ErsatzTV:**
- Verify NFS mount is accessible: `docker compose exec ersatztv ls /mnt/nfs`
- Check library paths match your directory structure
- Run a manual scan from ErsatzTV UI
- Ensure file permissions allow read access

### Desktop Application Issues

**Build fails - Node.js not found:**
- Run `build.bat` which auto-installs Node.js
- Or manually install Node.js v20+ from nodejs.org
- Restart command prompt after Node.js installation

**Controller not detected:**
- Ensure controller is connected before starting app
- Check browser console for gamepad events
- Try reconnecting the controller
- Xbox controllers work best; other controllers may vary

**Controller buttons not working:**
- Click inside the app window first to give it focus
- Check Settings > Controller Settings > Vibration toggle
- Verify gamepad is detected (indicator shows in UI)

**Media paths not saving:**
- Check write permissions to `%APPDATA%/nedflix/`
- Delete `nedflix-config.json` and reconfigure
- Run app as administrator if needed

**Videos won't play:**
- Check browser/Electron codec support
- Convert to MP4 (H.264) for best compatibility
- Verify file path doesn't contain special characters

**App crashes on startup:**
```batch
# Run in development mode to see errors
cd desktop
npm run dev
```

**Build creates empty installer:**
- Clear `desktop/dist/` folder
- Run `npm install` to refresh dependencies
- Check for build errors in console output

**Media keys not working:**
- Some keyboards require Fn key for media keys
- Check if another app is capturing media keys
- Try restarting the app
