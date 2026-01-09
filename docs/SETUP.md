# Nedflix Setup Guide

Complete setup and configuration documentation for Nedflix.

## Table of Contents

- [Prerequisites](#prerequisites)
- [Docker Deployment](#docker-deployment)
- [Manual Installation](#manual-installation)
- [Authentication](#authentication)
  - [Google OAuth](#google-oauth)
  - [GitHub OAuth](#github-oauth)
  - [Admin Local Login](#admin-local-login)
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
- Authentication: OAuth credentials (Google/GitHub) OR admin local login
- FFmpeg and FFprobe (optional, for audio track selection)

---

## Docker Deployment

### Docker Compose (Recommended)

Create a `.env` file with your settings:

```env
# Required: Session secret (use a long random string)
SESSION_SECRET=your-super-secret-random-string-here

# OAuth Providers (at least one required)
GOOGLE_CLIENT_ID=your-google-client-id
GOOGLE_CLIENT_SECRET=your-google-client-secret
GITHUB_CLIENT_ID=your-github-client-id
GITHUB_CLIENT_SECRET=your-github-client-secret

# Callback URL for OAuth (change for production)
CALLBACK_BASE_URL=https://localhost:3443

# Path to your video files on the host
NFS_PATH=/path/to/your/videos

# Optional: OpenSubtitles API key for automatic subtitles
OPENSUBTITLES_API_KEY=your-api-key
```

Then run:

```bash
docker compose up -d
```

### Docker Run (Without Compose)

```bash
# Build the image
docker build -t nedflix .

# Run the container
docker run -d \
  --name nedflix \
  -p 3000:3000 \
  -p 3443:3443 \
  -v /path/to/videos:/mnt/nfs:ro \
  -v nedflix-certs:/app/certs \
  -e SESSION_SECRET=your-secret \
  -e GOOGLE_CLIENT_ID=xxx \
  -e GOOGLE_CLIENT_SECRET=xxx \
  nedflix
```

### Docker Features

- **Auto-generates SSL certificates** on first run
- **Persists certificates** in a Docker volume
- **Read-only video mount** for security
- **Non-root user** inside container
- **Health checks** for container orchestration
- **Alpine-based** for minimal image size

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

---

## Authentication

### Google OAuth

1. Go to [Google Cloud Console](https://console.cloud.google.com/)
2. Create a new project or select existing
3. Navigate to "APIs & Services" > "Credentials"
4. Click "Create Credentials" > "OAuth 2.0 Client IDs"
5. Set Application type to "Web application"
6. Add authorized redirect URI: `https://localhost:3443/auth/google/callback`
7. Copy Client ID and Client Secret to your `.env` file

### GitHub OAuth

1. Go to [GitHub Developer Settings](https://github.com/settings/developers)
2. Click "New OAuth App"
3. Set Homepage URL: `https://localhost:3443`
4. Set Authorization callback URL: `https://localhost:3443/auth/github/callback`
5. Copy Client ID and Client Secret to your `.env` file

### Admin Local Login

For environments where OAuth is not available, you can configure a local admin account:

1. **Generate a password hash:**
```bash
node -e "console.log(require('bcrypt').hashSync('your-secure-password', 10))"
```

2. **Add credentials to your `.env` file:**
```env
ADMIN_USERNAME=admin
ADMIN_PASSWORD_HASH=$2b$10$xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
```

3. **Restart the server** - the admin login form will appear on the login page.

**Security notes:**
- Use a strong, unique password
- The password is never stored in plain text, only the bcrypt hash
- Admin login can be used alongside OAuth providers
- Consider using OAuth for production environments when possible

---

## Environment Variables

| Variable | Required | Default | Description |
|----------|----------|---------|-------------|
| `PORT` | No | `3000` | HTTP port (redirects to HTTPS) |
| `HTTPS_PORT` | No | `3443` | HTTPS port |
| `SESSION_SECRET` | Yes | - | Random string for session encryption |
| `GOOGLE_CLIENT_ID` | No* | - | Google OAuth client ID |
| `GOOGLE_CLIENT_SECRET` | No* | - | Google OAuth client secret |
| `GITHUB_CLIENT_ID` | No* | - | GitHub OAuth client ID |
| `GITHUB_CLIENT_SECRET` | No* | - | GitHub OAuth client secret |
| `ADMIN_USERNAME` | No* | - | Admin local login username |
| `ADMIN_PASSWORD_HASH` | No* | - | Admin password bcrypt hash |
| `CALLBACK_BASE_URL` | No | - | Base URL for OAuth callbacks |
| `NFS_MOUNT_PATH` | No | `/mnt/nfs` | Path to video files |
| `OPENSUBTITLES_API_KEY` | No | - | API key for automatic subtitles |

*At least one authentication method is required (OAuth provider or admin credentials).

---

## Project Structure

```
nedflix/
├── server.js              # Main Express server with OAuth
├── generate-certs.js      # SSL certificate generator
├── package.json           # Project dependencies
├── .env.example           # Environment template
├── Dockerfile             # Container build configuration
├── docker-compose.yml     # Docker Compose orchestration
├── .dockerignore          # Files excluded from Docker build
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

## Supported Video Formats

The application recognizes these video file extensions:
- `.mp4`, `.webm`, `.ogg`, `.avi`, `.mkv`, `.mov`, `.m4v`

Actual playback support depends on your browser's codec support. MP4 and WebM have the best compatibility.

---

## Security Considerations

### For Development

The included self-signed certificates are for development only. Your browser will show a security warning - you can proceed for testing purposes.

### For Production

1. **Use proper SSL certificates** from a trusted CA (e.g., Let's Encrypt)
2. **Change the SESSION_SECRET** to a long, random string
3. **Enable secure cookies** (already configured)
4. **Use a proper database** instead of in-memory storage for users/sessions
5. **Add rate limiting** to prevent abuse
6. **Configure proper CORS** if needed
7. **Run behind a reverse proxy** (nginx, Apache) for additional security
8. **Update OAuth callback URLs** to your production domain

---

## Troubleshooting

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

### Can't Access Directories
- Verify NFS mount is properly configured
- Check file system permissions
- Ensure Node.js process has read access

### Session/Login Issues
- Clear browser cookies
- Restart the server
- Check SESSION_SECRET is set

### Subtitles Not Loading
- Verify `OPENSUBTITLES_API_KEY` is set correctly
- Check server logs for API errors
- Ensure video filename contains recognizable title information

### Audio Track Selector Not Appearing
- Verify FFprobe is installed: `ffprobe -version`
- Check server logs for detection errors
- Ensure video has multiple audio tracks

### Docker Issues

**Container won't start:**
```bash
# Check logs
docker compose logs nedflix

# Verify environment variables
docker compose config
```

**Can't access videos:**
- Ensure the NFS_PATH in `.env` points to valid directory
- Check volume mount permissions
- Verify the path exists on host: `ls -la /path/to/your/videos`

**Certificate issues in Docker:**
```bash
# Regenerate certificates
docker compose down
docker volume rm nedflix-certs
docker compose up -d
```
