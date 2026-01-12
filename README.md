<p align="center">
  <img src="https://img.shields.io/badge/Nedflix-Personal%20Streaming-E50914?style=for-the-badge&logo=data:image/svg+xml;base64,PHN2ZyB2aWV3Qm94PSIwIDAgMjQgMjQiIGZpbGw9IndoaXRlIiB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciPjxwb2x5Z29uIHBvaW50cz0iNSAzIDE5IDEyIDUgMjEgNSAzIj48L3BvbHlnb24+PC9zdmc+" alt="Nedflix">
</p>

<h1 align="center">Nedflix</h1>

<p align="center">
  <strong>Your personal video streaming platform</strong><br>
  Stream your NFS-mounted media library with style
</p>

<p align="center">
  <img src="https://img.shields.io/badge/node-%3E%3D14.0.0-brightgreen?style=flat-square" alt="Node">
  <img src="https://img.shields.io/badge/docker-ready-blue?style=flat-square&logo=docker" alt="Docker">
  <img src="https://img.shields.io/badge/PostgreSQL-supported-336791?style=flat-square&logo=postgresql" alt="PostgreSQL">
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
- **Video Streaming** - Browse and stream from NFS-mounted directories
- **Automatic Subtitles** - Search and download via OpenSubtitles API
- **Multi-Audio Support** - Switch between audio tracks (requires FFmpeg)
- **Metadata Fetching** - Automatic movie/TV show information from OMDb and TVmaze

### Live TV & Channels
- **Live TV (IPTV)** - Watch live streams via M3U playlists and XMLTV EPG
- **Auto-Generated Channels** - 24/7 streaming channels from your media library via ErsatzTV
- **Channel Guide** - Program schedule and now playing information

### User Experience
- **User Profiles** - Customizable avatars and streaming preferences
- **Dark/Light Theme** - Modern, responsive interface
- **Persistent Database** - PostgreSQL for reliable data storage

---

## Quick Start

### Using Docker Compose (Recommended)

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

### Minimal Setup (Local Admin Only)

Add to your `.env` file:

```env
SESSION_SECRET=your-random-secret-string
ADMIN_USERNAME=admin
ADMIN_PASSWORD=your-secure-password
NFS_PATH=/path/to/your/videos
```

No OAuth setup required - just username and password login.

---

## Architecture

Nedflix runs as a multi-container Docker application:

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

**Services:**
- **Nedflix** - Main streaming application (ports 3000/3443)
- **ErsatzTV** - Auto-generated IPTV channels (port 8409)
- **PostgreSQL** - Persistent database storage

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

### Features

| Feature | Description |
|---------|-------------|
| **Movies 24/7** | Continuous movie playback, shuffled |
| **TV Shows Marathon** | Binge your TV library, shuffled |
| **Music Videos** | Background music channel |
| **Program Guide** | EPG with now playing info |

### Setup

1. **Navigate to Channels** - Click the "Channels" card on the home screen
2. **Auto-Setup** - Admin users click "Setup Auto-Channels" button
3. **Wait for Scan** - ErsatzTV scans your media library
4. **Start Watching** - Channels appear automatically

### Technical Details

- ErsatzTV runs as a separate Docker container
- Shares the same NFS mount as Nedflix
- Provides M3U playlist and XMLTV EPG
- Channels stream in real-time from your local media

---

## Admin Panel

The admin panel provides centralized management for:

### User Management
- Create, edit, and delete users
- Set admin privileges
- Control library access per user (Movies, TV, Music, Audiobooks)
- Enable/disable user accounts
- Reset passwords

### Media Library Index
- Scan your media library to build a searchable index
- View scan statistics (total files, videos, audio)
- Monitor scan progress and history
- Download scan logs

### ErsatzTV / Channels
- View channel status and health
- Setup auto-channels from media
- Monitor library synchronization

### How to Access
1. Log in as an admin user
2. Click the gear icon in the header (next to your profile)
3. The admin panel opens as an overlay

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

## Search

Nedflix includes a powerful search feature:

- **Indexed Search** - Searches a database index instead of scanning disks
- **Category Scoped** - Results are filtered to your current library (Movies, TV, etc.)
- **Fast Results** - Instant results from the PostgreSQL database
- **Click to Navigate** - Jump directly to search results

To enable search:
1. Open the Admin Panel
2. Click "Scan Library" under Media Library Index
3. Wait for the scan to complete
4. Use the search bar in the file browser

---

## Configuration

### Environment Variables

| Variable | Description |
|----------|-------------|
| `SESSION_SECRET` | Random string for session encryption |
| `ADMIN_USERNAME` | Local admin username |
| `ADMIN_PASSWORD` | Local admin password |
| `NFS_PATH` | Path to your video library |
| `CERTS_PATH` | Path to SSL certificates (default: ./certs) |
| `POSTGRES_USER` | PostgreSQL username (default: nedflix) |
| `POSTGRES_PASSWORD` | PostgreSQL password (default: nedflix_secret) |
| `ERSATZTV_URL` | ErsatzTV API URL (default: http://ersatztv:8409) |
| `TIMEZONE` | Timezone for ErsatzTV EPG (default: America/New_York) |
| `GOOGLE_CLIENT_ID` | Google OAuth client ID (optional) |
| `GITHUB_CLIENT_ID` | GitHub OAuth client ID (optional) |
| `OPENSUBTITLES_API_KEY` | For automatic subtitles (optional) |
| `OMDB_API_KEY` | For movie/TV metadata (optional) |

---

## Database

Nedflix uses PostgreSQL for persistent storage of:

- **Users** - Account information and credentials
- **User Settings** - Streaming preferences and profile pictures
- **File Index** - Cached media library for fast browsing and search
- **Scan Logs** - History of library scan operations
- **Media Metadata** - Cached movie/TV show information

Data is stored in Docker volumes that persist across container restarts:
- `nedflix-db-data` - PostgreSQL database
- `ersatztv-config` - ErsatzTV configuration and channel data

### Local Development (SQLite)

When running without Docker or without PostgreSQL configured, Nedflix automatically falls back to SQLite for local development.

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

## Documentation

For detailed setup instructions, see **[docs/SETUP.md](docs/SETUP.md)**:

- SSL certificate configuration
- OAuth provider setup (Google/GitHub)
- Docker deployment options
- PostgreSQL configuration
- ErsatzTV setup and configuration
- User management guide
- FFmpeg installation for audio tracks
- Troubleshooting guide

---

## Requirements

| Component | Required | Notes |
|-----------|----------|-------|
| Docker | Recommended | Or Node.js v14+ |
| Auth | Yes | Local admin OR OAuth provider |
| PostgreSQL | Included | Via Docker Compose |
| ErsatzTV | Included | Via Docker Compose (for Channels) |
| FFmpeg | Optional | For audio track switching |
| OpenSubtitles API | Optional | For automatic subtitles |
| OMDb API | Optional | For movie/TV metadata |

---

## Project Structure

```
nedflix/
├── server.js              # Main Express server
├── ersatztv-service.js    # ErsatzTV API integration
├── iptv-service.js        # IPTV/Live TV service
├── db.js                  # Database abstraction layer
├── user-service.js        # User management service
├── media-service.js       # Media indexing and search
├── metadata-service.js    # Movie/TV metadata fetching
├── docker-compose.yml     # Docker orchestration (Nedflix + ErsatzTV + PostgreSQL)
├── public/
│   ├── index.html         # Main application
│   ├── app.js             # Core client-side logic
│   ├── livetv.js          # Live TV functionality
│   ├── channels.js        # Channels (ErsatzTV) functionality
│   └── styles.css         # Application styles
└── docs/
    └── SETUP.md           # Detailed setup guide
```

---

## License

Open source for personal and educational use.

---

<p align="center">
  <sub>Built for movie nights | Powered by ErsatzTV</sub>
</p>
