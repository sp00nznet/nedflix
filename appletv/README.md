# Nedflix for Apple TV

Build scripts for compiling Nedflix for Apple TV (tvOS).

## Overview

Apple TV is a digital media player and streaming device running tvOS (based on iOS). Current models include:
- **Apple TV 4K (3rd gen, 2022)**: A15 Bionic, HDR10+, Dolby Vision
- **Apple TV 4K (2nd gen, 2021)**: A12 Bionic, Thread, HDMI 2.1
- **Apple TV 4K (1st gen, 2017)**: A10X Fusion
- **Apple TV HD (4th gen, 2015)**: A8 chip

## Versions

### Client Version
- **Purpose**: Connect to remote Nedflix server
- **Network**: Required for streaming
- **Authentication**: OAuth/Local supported
- **Use case**: Access your media library from your Nedflix server

### Desktop Version
- **Purpose**: Standalone media player
- **Network**: Optional (local playback)
- **Authentication**: None (direct access)
- **Use case**: Play media from iCloud Drive, SMB shares, or local cache

## Build Requirements

### Development Environment
1. **macOS** (Ventura 13.0+ recommended)
2. **Xcode 14+**
   - Install from Mac App Store
   - Install tvOS SDK (Xcode > Preferences > Platforms)
3. **Apple Developer Account**
   - Required for device deployment
   - Required for App Store submission

### Dependencies
- **Swift**: 5.7+ (included with Xcode)
- **CocoaPods** or **Swift Package Manager** (optional)
- **GCDWebServer**: For Desktop version embedded HTTP server

### System Requirements
- macOS 13.0+
- Xcode 14.0+
- 10 GB free disk space
- Apple Developer account ($99/year for distribution)

## Building

### Client Version
```bash
cd appletv
chmod +x build-client.sh
./build-client.sh
```

Output: `build/client/NedflixTV-Client.ipa`

### Desktop Version
```bash
cd appletv
chmod +x build-desktop.sh
./build-desktop.sh
```

Output: `build/desktop/NedflixTV-Desktop.ipa`

## Deployment

### Prerequisites
1. Apple Developer account
2. App-specific provisioning profile
3. Code signing certificate
4. Apple TV device in Developer Mode (for testing)

### Option 1: TestFlight (Recommended for Testing)
1. Open Xcode and go to **Organizer**
2. Upload the IPA to App Store Connect
3. In App Store Connect, create a TestFlight build
4. Add internal/external testers
5. Testers install via TestFlight app on Apple TV

### Option 2: App Store (Public Release)
1. Upload IPA to App Store Connect
2. Fill in app metadata (description, screenshots, etc.)
3. Submit for App Store review
4. After approval, available on tvOS App Store

### Option 3: Direct Install (Development Only)
1. Enable Developer Mode on Apple TV:
   - Settings > Remotes and Devices > Remote App and Devices
2. Connect Apple TV to Mac via USB-C (on older models) or network
3. Open Xcode > Window > Devices and Simulators
4. Select your Apple TV
5. Click "+" to add app, select IPA
6. App installs directly on device

## Features

### Client Version
- ✅ Native tvOS UI with Focus Engine
- ✅ Siri Remote support (swipe, click, voice search)
- ✅ Top Shelf support (recently watched content)
- ✅ Picture in Picture
- ✅ Server authentication (OAuth 2.0)
- ✅ HTTP/2 and HLS streaming
- ✅ Dolby Atmos & Dolby Vision (4K models)
- ✅ AirPlay 2 integration
- ✅ iCloud sync for watch progress
- ✅ Handoff support (continue watching on other devices)
- ✅ SharePlay support (watch together)

### Desktop Version
- ✅ All Client features, plus:
- ✅ Embedded HTTP server (GCDWebServer)
- ✅ No authentication required
- ✅ Local media from iCloud Drive
- ✅ SMB/NFS network share support
- ✅ Files app integration
- ✅ Offline playback support
- ✅ On-device media library
- ✅ AirPlay 2 server (stream to other devices)

### Supported Formats
- **Video**: MP4, MOV, M4V, MKV (via AVFoundation)
- **Audio**: AAC, MP3, FLAC, ALAC, Dolby Digital, Dolby Atmos
- **Subtitles**: SRT, VTT, embedded subtitles
- **HDR**: Dolby Vision, HDR10, HLG (4K models only)

### Resolution Support
- **4K models**: Up to 4K (3840 x 2160) @ 60 fps
- **HD model**: Up to 1080p (1920 x 1080) @ 60 fps

## Project Structure

```
appletv/
├── build-client.sh          # Client build script
├── build-desktop.sh         # Desktop build script
├── README.md                # This file
├── NedflixTV/               # Xcode project (to be created)
│   ├── NedflixTV.xcodeproj
│   ├── Client/              # Client-specific code
│   │   ├── ServerConfig.swift
│   │   ├── AuthManager.swift
│   │   └── StreamingService.swift
│   ├── Desktop/             # Desktop-specific code
│   │   ├── EmbeddedServer.swift
│   │   ├── LocalMediaManager.swift
│   │   └── NetworkShareBrowser.swift
│   ├── Shared/              # Shared code
│   │   ├── Views/           # SwiftUI views
│   │   ├── Models/          # Data models
│   │   ├── Services/        # Media services
│   │   └── Resources/       # Assets, Web UI
│   └── Info.plist
└── build/                   # Build output
    ├── client/
    └── desktop/
```

## Technical Details

### Client Architecture
- **UI**: SwiftUI with UIKit for custom controls
- **Networking**: URLSession with HTTP/2
- **Streaming**: AVFoundation with HLS (HTTP Live Streaming)
- **Authentication**: OAuth 2.0 via ASWebAuthenticationSession
- **Storage**: Core Data for cached metadata

### Desktop Architecture
- **HTTP Server**: GCDWebServer (lightweight, Swift-compatible)
  - Port: 3000 (localhost)
  - Routes: RESTful API for media browsing/playback
- **Web UI**: Bundled HTML/CSS/JS (from desktop/public)
- **Media Access**:
  - iCloud Drive: UIDocumentPickerViewController
  - SMB Shares: Network.framework
  - Local Cache: App sandbox Documents folder
- **Playback**: AVPlayer with AVFoundation

### App Capabilities Required
- **Client**:
  - App Transport Security exception (for HTTP servers)
  - Network access
  - iCloud (optional, for sync)
- **Desktop**:
  - All Client capabilities
  - Local Network (for SMB/NFS)
  - File Provider extensions
  - Background modes (for HTTP server)

## Troubleshooting

### Build Issues
- **"tvOS SDK not found"**: Open Xcode > Preferences > Platforms, install tvOS support
- **"Code signing failed"**: Configure signing in Xcode project settings with your Developer account
- **"Command not found: xcodebuild"**: Install Xcode Command Line Tools:
  ```bash
  xcode-select --install
  ```

### Runtime Issues
- **"App crashes on launch"**: Check Console.app for crash logs, ensure all entitlements are configured
- **"Cannot connect to server"** (Client): Verify server URL and network connectivity
- **"No media found"** (Desktop): Grant Files app permission, check iCloud Drive sync
- **"Playback stutters"**: Reduce video quality or check network bandwidth (Client)

### Siri Remote Issues
- Ensure Focus Engine is properly configured in SwiftUI views
- Test focus navigation in tvOS Simulator
- Use `preferredFocusEnvironments` for default focus

## App Store Guidelines

When submitting to App Store, ensure compliance with:
- **Content Rights**: Verify you have rights to distribute media
- **Privacy**: Include privacy policy URL
- **Parental Controls**: Implement age ratings for content
- **Accessibility**: Support VoiceOver and accessibility features
- **Top Shelf**: Provide engaging Top Shelf content
- **Screenshots**: Provide required screenshots (1920x1080)

## Development Roadmap

### Planned Features
- [ ] SharePlay support for watch parties
- [ ] Live TV integration
- [ ] Multi-user profiles
- [ ] Parental controls
- [ ] Dolby Vision dynamic metadata support
- [ ] 3D audio (Spatial Audio)
- [ ] Game controller support

## References

- **Apple Developer**: https://developer.apple.com/tvos/
- **tvOS HIG**: https://developer.apple.com/design/human-interface-guidelines/tvos
- **AVFoundation**: https://developer.apple.com/av-foundation/
- **GCDWebServer**: https://github.com/swisspol/GCDWebServer
- **App Store Connect**: https://appstoreconnect.apple.com/

## Support

For issues or questions:
- Check Apple Developer Forums: https://developer.apple.com/forums/
- Review tvOS documentation: https://developer.apple.com/documentation/tvos
- File issues on the main Nedflix repository

## License

Same as parent Nedflix project.
