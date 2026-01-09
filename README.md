# Nedflix

A personal video streaming platform for NFS-mounted media libraries with HTTPS, OAuth authentication, automatic subtitles, and multi-language audio support.

## Features

- Secure HTTPS with OAuth (Google/GitHub)
- Browse and stream videos from NFS mounts
- Automatic subtitle search via OpenSubtitles
- Audio track selection for multi-language videos
- User profiles with customizable settings
- Dark-themed responsive UI

## Quick Start

```bash
# Clone and configure
git clone <repository-url>
cd nedflix
cp .env.example .env
# Edit .env with your OAuth credentials

# Run with Docker
docker compose up -d

# Access at https://localhost:3443
```

## Configuration

Create a `.env` file with:

```env
SESSION_SECRET=your-random-secret-string
GOOGLE_CLIENT_ID=your-google-client-id
GOOGLE_CLIENT_SECRET=your-google-client-secret
CALLBACK_BASE_URL=https://localhost:3443
NFS_PATH=/path/to/your/videos

# Optional: for automatic subtitles
OPENSUBTITLES_API_KEY=your-api-key
```

## Documentation

See [docs/SETUP.md](docs/SETUP.md) for:
- Detailed installation instructions
- OAuth provider setup
- Docker configuration options
- Audio/subtitle feature requirements
- Security considerations
- Troubleshooting guide

## Requirements

- Docker (recommended) or Node.js v14+
- OAuth credentials (Google and/or GitHub)
- FFmpeg/FFprobe (optional, for audio track selection)

## License

Open source for personal and educational use.
