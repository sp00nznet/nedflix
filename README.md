# NFS Video Browser

A secure HTML5 and CSS compliant web application for browsing NFS mount points and playing video files, featuring HTTPS support, OAuth authentication, and user profiles.

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

- Node.js (v14 or higher)
- npm (Node Package Manager)
- OpenSSL (for generating SSL certificates)
- An NFS mount point (default: `/mnt/nfs`)
- OAuth credentials from Google and/or GitHub

## Installation

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
nfs-video-browser/
├── server.js              # Main Express server with OAuth
├── generate-certs.js      # SSL certificate generator
├── package.json           # Project dependencies
├── .env.example           # Environment template
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

## License

This project is open source and available for personal and educational use.
