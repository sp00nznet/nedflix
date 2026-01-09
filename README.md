# Nedflix - NFS Video Browser

A secure HTML5 and CSS compliant web application for browsing NFS mount points and playing video files, featuring HTTPS support, OAuth authentication, and user profiles.

> **Nedflix** - Your personal video streaming platform for NFS-mounted media libraries.

## Features

- **Secure HTTPS connection** with automatic HTTP redirect
- **OAuth authentication** supporting Google and GitHub
- **User profiles** with customizable avatar pictures
- **Streaming settings** for quality, volume, playback speed, and more
- Browse directories on your NFS mount
- Play video files directly in the browser using HTML5 video player
- Support for common video formats (MP4, WebM, OGG, AVI, MKV, MOV, M4V)
- Responsive design that works on desktop and mobile
- Modern, dark-themed UI

## Prerequisites

- Node.js (v14 or higher) - *or Docker*
- npm (Node Package Manager) - *or Docker*
- OpenSSL (for generating SSL certificates) - *included in Docker image*
- An NFS mount point or video directory (default: `/mnt/nfs`)
- OAuth credentials from Google and/or GitHub

---

## Docker Deployment (Recommended)

The easiest way to run Nedflix is with Docker.

### Quick Start

```bash
# Clone the repository
git clone <repository-url>
cd nedflix

# Create your environment file
cp .env.example .env
# Edit .env with your OAuth credentials

# Run with Docker Compose
docker compose up -d

# Access the application
# https://localhost:3443
```

### Docker Compose Configuration

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
Edit `.env` with your settings (see Configuration section below).

4. **Mount your NFS share (if not already mounted):**
```bash
sudo mount -t nfs server:/path/to/share /mnt/nfs
```

## Configuration

### Environment Variables (.env)

```env
# Server Configuration
PORT=3000                    # HTTP port (redirects to HTTPS)
HTTPS_PORT=3443              # HTTPS port
SESSION_SECRET=your-secret   # Change this to a random string!

# Google OAuth
GOOGLE_CLIENT_ID=xxx
GOOGLE_CLIENT_SECRET=xxx

# GitHub OAuth
GITHUB_CLIENT_ID=xxx
GITHUB_CLIENT_SECRET=xxx

# Callback URL (change for production)
CALLBACK_BASE_URL=https://localhost:3443

# NFS Mount Point
NFS_MOUNT_PATH=/mnt/nfs
```

### Setting up OAuth Providers

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

## Usage

1. **Start the server:**
```bash
npm start
```

2. **Access the application:**
   - HTTPS: `https://localhost:3443`
   - HTTP requests are automatically redirected to HTTPS

3. **Sign in** using Google or GitHub

4. **Configure your profile:**
   - Click your avatar in the top-right corner
   - Choose a profile picture from the available avatars
   - Adjust streaming settings (quality, volume, playback speed)
   - Save your settings

5. **Browse and play videos:**
   - Navigate directories using the file browser
   - Click video files to play them
   - Use the "Parent" button to go up one directory

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
├── public/
│   ├── index.html        # Main application page
│   ├── login.html        # Login page
│   ├── styles.css        # Application styles
│   ├── app.js            # Client-side JavaScript
│   └── images/           # Avatar images
│       ├── cat.svg
│       ├── dog.svg
│       ├── cow.svg
│       ├── fox.svg
│       ├── owl.svg
│       ├── bear.svg
│       ├── rabbit.svg
│       └── penguin.svg
└── README.md
```

## User Settings

### Streaming Options

- **Video Quality**: Auto, High (1080p), Medium (720p), Low (480p)
- **Default Volume**: 0-100%
- **Playback Speed**: 0.5x to 2x
- **Autoplay**: Automatically play next video
- **Subtitles**: Show subtitles when available

### Profile Options

- Choose from 8 default avatar pictures (cat, dog, cow, fox, owl, bear, rabbit, penguin)

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

## Supported Video Formats

The application recognizes these video file extensions:
- .mp4
- .webm
- .ogg
- .avi
- .mkv
- .mov
- .m4v

Note: Actual playback support depends on your browser's codec support. MP4 and WebM have the best compatibility.

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

## License

This project is open source and available for personal and educational use.
