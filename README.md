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
  <img src="https://img.shields.io/badge/license-MIT-green?style=flat-square" alt="License">
</p>

---

## Features

- **Secure Authentication** - Local admin login or OAuth (Google/GitHub)
- **Video Streaming** - Browse and stream from NFS-mounted directories
- **Automatic Subtitles** - Search and download via OpenSubtitles API
- **Multi-Audio Support** - Switch between audio tracks (requires FFmpeg)
- **User Profiles** - Customizable avatars and streaming preferences
- **Dark Theme** - Modern, responsive interface

---

## Quick Start

### Using Docker (Recommended)

```bash
# Clone the repository
git clone https://github.com/sp00nznet/nedflix.git
cd nedflix

# Configure environment
cp .env.example .env
nano .env  # Add your credentials

# Launch
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

## Configuration

### Environment Variables

| Variable | Description |
|----------|-------------|
| `SESSION_SECRET` | Random string for session encryption |
| `ADMIN_USERNAME` | Local admin username |
| `ADMIN_PASSWORD` | Local admin password |
| `NFS_PATH` | Path to your video library |
| `GOOGLE_CLIENT_ID` | Google OAuth client ID (optional) |
| `GOOGLE_CLIENT_SECRET` | Google OAuth secret (optional) |
| `GITHUB_CLIENT_ID` | GitHub OAuth client ID (optional) |
| `GITHUB_CLIENT_SECRET` | GitHub OAuth secret (optional) |
| `OPENSUBTITLES_API_KEY` | For automatic subtitles (optional) |

---

## Documentation

For detailed setup instructions, see **[docs/SETUP.md](docs/SETUP.md)**:

- SSL certificate configuration
- OAuth provider setup (Google/GitHub)
- Docker deployment options
- FFmpeg installation for audio tracks
- Troubleshooting guide

---

## Requirements

| Component | Required | Notes |
|-----------|----------|-------|
| Docker | Recommended | Or Node.js v14+ |
| Auth | Yes | Local admin OR OAuth provider |
| FFmpeg | Optional | For audio track switching |
| OpenSubtitles API | Optional | For automatic subtitles |

---

## License

Open source for personal and educational use.

---

<p align="center">
  <sub>Built for movie nights</sub>
</p>
