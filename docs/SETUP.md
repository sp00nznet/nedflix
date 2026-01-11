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

Docker Compose sets up both the Nedflix application and a PostgreSQL database for persistent storage.

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

This starts two containers:
- `nedflix` - The main application (ports 3000, 3443)
- `nedflix-db` - PostgreSQL database (port 5432, internal only)

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
```bash
# Point CERTS_PATH to your Let's Encrypt certificates
CERTS_PATH=/etc/letsencrypt/live/yourdomain.com
```
Note: Rename or symlink `fullchain.pem` to `server.cert` and `privkey.pem` to `server.key`.

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

*Admin credentials or OAuth provider required for authentication.

---

## Project Structure

```
nedflix/
├── server.js              # Main Express server
├── db.js                  # Database abstraction layer (SQLite/PostgreSQL)
├── user-service.js        # User management service
├── media-service.js       # Media indexing and search service
├── metadata-service.js    # Movie/TV metadata fetching
├── generate-certs.js      # SSL certificate generator
├── package.json           # Project dependencies
├── .env.example           # Environment template
├── Dockerfile             # Container build configuration
├── docker-compose.yml     # Docker Compose orchestration
├── db-init/               # PostgreSQL initialization scripts
│   └── 01-schema.sql      # Database schema
├── certs/                 # SSL certificates (generated)
│   ├── server.key
│   └── server.cert
├── docs/
│   └── SETUP.md           # This file
├── public/
│   ├── index.html         # Main application page
│   ├── login.html         # Login page
│   ├── styles.css         # Application styles
│   ├── app.js             # Client-side JavaScript
│   ├── thumbnails/        # Cached movie/TV posters
│   └── images/            # Avatar images
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
