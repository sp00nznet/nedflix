# Nedflix for Android TV

Build scripts for compiling Nedflix for Android TV and Google TV devices.

## Overview

Android TV is Google's smart TV platform that runs on various devices:
- **Google TV** (Chromecast with Google TV)
- **NVIDIA Shield TV** (Pro and standard)
- **Smart TVs**: Sony Bravia, TCL, Hisense, Philips
- **Set-top boxes**: Mi Box, Xiaomi Mi Box S
- **Amazon Fire TV** (Fire TV Stick, Fire TV Cube)

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
- **Use case**: Play media from local storage, USB drives, or network shares (SMB/NFS)

## Build Requirements

### Development Environment
1. **Android Studio** (Flamingo 2022.2.1+)
   - Download: https://developer.android.com/studio
   - Install Android TV SDK components
2. **Android SDK**
   - Min API: 21 (Android 5.0 Lollipop)
   - Target API: 33 (Android 13)
   - Build Tools: 33.0.0+
3. **JDK 17+**
   - Download: https://adoptium.net/
4. **Gradle 8.0+**
   - Included via Gradle wrapper

### System Requirements
- 8 GB RAM minimum (16 GB recommended)
- 10 GB free disk space
- Windows, macOS, or Linux

## Building

### Client Version
```bash
cd androidtv
chmod +x build-client.sh
export ANDROID_HOME=/path/to/android-sdk  # if not set
./build-client.sh
```

Output: `app/build/outputs/apk/client/release/NedflixTV-client-release.apk`

### Desktop Version
```bash
cd androidtv
chmod +x build-desktop.sh
export ANDROID_HOME=/path/to/android-sdk  # if not set
./build-desktop.sh
```

Output: `app/build/outputs/apk/desktop/release/NedflixTV-desktop-release.apk`

### Build with Gradle Directly
```bash
# Client version
./gradlew :app:assembleClientRelease

# Desktop version
./gradlew :app:assembleDesktopRelease

# Both versions
./gradlew :app:assembleRelease
```

## Deployment

### Prerequisites
- Android TV device with Developer Mode enabled
- ADB (Android Debug Bridge) installed

### Enable Developer Mode on Android TV
1. Go to **Settings** > **About**
2. Scroll to **Build** and click 7 times
3. Developer options will appear in Settings
4. Enable **USB debugging** and **Install unknown apps**

### Option 1: Install via ADB (Recommended)
```bash
# Connect to Android TV over network
adb connect <android-tv-ip>:5555

# Install APK
adb install -r NedflixTV-client-release.apk

# Launch app
adb shell am start -n com.nedflix.tv/.MainActivity
```

### Option 2: Sideload via File Manager
1. Copy APK to USB drive
2. Plug USB into Android TV
3. Install a file manager (e.g., "X-plore File Manager")
4. Navigate to USB drive in file manager
5. Click APK file to install

### Option 3: Google Play Store (Production)
1. Create a Google Play Developer account ($25 one-time fee)
2. Sign APK with release keystore:
   ```bash
   ./gradlew :app:bundleClientRelease  # Creates AAB for Play Store
   ```
3. Upload AAB to Google Play Console
4. Fill in store listing (description, screenshots, etc.)
5. Submit for review
6. After approval, users can install from Play Store

## Features

### Client Version
- ✅ Leanback UI (optimized for 10-foot TV viewing)
- ✅ D-pad and remote control navigation
- ✅ ExoPlayer for advanced media playback
- ✅ HLS, DASH, and progressive streaming support
- ✅ Server authentication (OAuth 2.0)
- ✅ Content recommendations (Android TV Home screen)
- ✅ Google Cast integration
- ✅ Voice search (Google Assistant)
- ✅ Picture in Picture mode
- ✅ HDR10+ support (on compatible devices)
- ✅ Dolby Atmos passthrough
- ✅ Multiple audio tracks and subtitles
- ✅ Playback state synchronization

### Desktop Version
- ✅ All Client features, plus:
- ✅ Embedded NanoHTTPD server (lightweight Java HTTP server)
- ✅ No authentication required
- ✅ Local media playback from:
  - Internal storage
  - External USB drives (OTG)
  - SD cards
  - SMB/CIFS network shares
  - NFS network shares
  - DLNA/UPnP servers
- ✅ On-device media library scanning
- ✅ Offline playback support
- ✅ Network share browser

### Supported Formats
- **Video**: MP4, MKV, AVI, MOV, WebM, TS
- **Codecs**: H.264, H.265 (HEVC), VP9, AV1
- **Audio**: AAC, MP3, Opus, Vorbis, FLAC, AC3, EAC3, Dolby Atmos
- **Subtitles**: SRT, VTT, ASS, SSA, embedded subtitles
- **Containers**: MP4, Matroska, FLV, WebM, TS

### HDR Support
- **HDR10**: Supported on compatible devices
- **HDR10+**: Dynamic metadata support
- **Dolby Vision**: Passthrough (device-dependent)
- **HLG**: Hybrid Log-Gamma

## Project Structure

```
androidtv/
├── build-client.sh              # Client build script
├── build-desktop.sh             # Desktop build script
├── README.md                    # This file
├── build.gradle                 # Root build configuration
├── settings.gradle              # Module settings
├── gradle.properties            # Gradle properties
├── gradlew                      # Gradle wrapper (Unix)
├── gradlew.bat                  # Gradle wrapper (Windows)
└── app/                         # Main app module
    ├── build.gradle             # App build configuration
    ├── src/
    │   ├── main/                # Shared code
    │   │   ├── java/            # Java/Kotlin source
    │   │   │   └── com/nedflix/tv/
    │   │   │       ├── MainActivity.kt
    │   │   │       ├── ui/      # Leanback UI
    │   │   │       ├── player/  # ExoPlayer integration
    │   │   │       └── data/    # Data layer
    │   │   ├── res/             # Resources
    │   │   │   ├── layout/      # XML layouts
    │   │   │   ├── values/      # Strings, colors
    │   │   │   └── drawable/    # Images, icons
    │   │   └── AndroidManifest.xml
    │   ├── client/              # Client-specific code
    │   │   └── java/
    │   │       └── com/nedflix/tv/
    │   │           ├── ServerConfig.kt
    │   │           └── AuthManager.kt
    │   └── desktop/             # Desktop-specific code
    │       ├── java/
    │       │   └── com/nedflix/tv/
    │       │       ├── EmbeddedServer.kt
    │       │       ├── LocalMediaManager.kt
    │       │       └── NetworkShareManager.kt
    │       └── assets/
    │           └── webui/       # Web UI files (copied from desktop/public)
    └── proguard-rules.pro       # ProGuard configuration
```

## Technical Details

### Client Architecture
- **UI**: Jetpack Compose for TV + Leanback libraries
- **Navigation**: Leanback navigation with D-pad support
- **Networking**: Retrofit + OkHttp with HTTP/2
- **Streaming**: ExoPlayer with HLS/DASH support
- **Authentication**: OAuth 2.0 via Custom Tabs
- **Image Loading**: Coil with caching
- **Storage**: Room database for metadata

### Desktop Architecture
- **HTTP Server**: NanoHTTPD
  - Lightweight Java HTTP server (~100KB)
  - Port: 3000 (localhost)
  - RESTful API for media browsing/playback
- **Web UI**: Bundled in `assets/webui/`
  - Served by NanoHTTPD
  - HTML/CSS/JS from desktop/public
- **Media Access**:
  - **Local**: MediaStore API, Storage Access Framework
  - **USB**: USB OTG support
  - **SMB**: jCIFS-NG library for Windows shares
  - **NFS**: NFS client library
  - **DLNA**: Cling library for UPnP discovery
- **Media Scanning**: Background service with MediaScanner
- **Playback**: ExoPlayer with local file support

### Build Variants
The app uses Gradle product flavors:

```gradle
productFlavors {
    client {
        applicationIdSuffix ".client"
        buildConfigField "Boolean", "IS_DESKTOP", "false"
    }
    desktop {
        applicationIdSuffix ".desktop"
        buildConfigField "Boolean", "IS_DESKTOP", "true"
    }
}
```

### Permissions Required
- **Client**:
  - `INTERNET` - Network access
  - `ACCESS_NETWORK_STATE` - Check connectivity
  - `WAKE_LOCK` - Keep screen on during playback
- **Desktop**:
  - All Client permissions, plus:
  - `READ_EXTERNAL_STORAGE` - Access local media
  - `WRITE_EXTERNAL_STORAGE` - Cache metadata
  - `MANAGE_EXTERNAL_STORAGE` - Full storage access (Android 11+)
  - `USB_PERMISSION` - Access USB drives

## Troubleshooting

### Build Issues

**"ANDROID_HOME not set"**
```bash
# Linux/macOS
export ANDROID_HOME=$HOME/Android/Sdk

# Windows
set ANDROID_HOME=C:\Users\<username>\AppData\Local\Android\Sdk
```

**"SDK location not found"**
- Create `local.properties` in project root:
  ```properties
  sdk.dir=/path/to/android-sdk
  ```

**"Gradle sync failed"**
- Update Gradle wrapper: `./gradlew wrapper --gradle-version=8.2`
- Invalidate caches in Android Studio: File > Invalidate Caches / Restart

**"Build tools not found"**
```bash
# Install via sdkmanager
sdkmanager "build-tools;33.0.0"
```

### Runtime Issues

**"App crashes on launch"**
- Check Logcat in Android Studio for stack traces
- Verify all permissions are granted in app settings
- Clear app data and cache

**"Cannot connect to server" (Client)**
- Verify server URL is correct and accessible
- Check network connectivity
- Disable VPN if interfering

**"No media found" (Desktop)**
- Grant storage permissions in app settings
- Check USB drive is connected and mounted
- Verify media is in supported format

**"Playback stutters"**
- Lower video quality in settings
- Check network bandwidth (Client)
- Clear ExoPlayer cache

**"SMB shares not appearing" (Desktop)**
- Verify SMB server is accessible on network
- Check username/password for share
- Enable SMB1 if required (older NAS devices)

## Performance Optimization

### Client Version
- Enable HTTP/2 for better streaming performance
- Configure ExoPlayer buffer sizes based on network
- Use adaptive bitrate streaming (HLS/DASH)
- Implement playback state caching

### Desktop Version
- Index media library in background service
- Cache thumbnails and metadata in SQLite
- Use ExoPlayer's caching support for local files
- Optimize NanoHTTPD with thread pooling

## Google Play Store Submission

### Requirements
1. **App Bundle (AAB)**: Use `bundleRelease` instead of `assembleRelease`
2. **Screenshots**: At least 2 screenshots (1920x1080)
3. **Banner**: TV banner image (1280x720)
4. **Icon**: High-res icon (512x512)
5. **Privacy Policy**: URL to privacy policy
6. **Content Rating**: Complete questionnaire
7. **Target Audience**: Specify age groups

### Store Listing Tips
- **Title**: "Nedflix - Media Player" (max 30 chars)
- **Short Description**: Highlight key features (max 80 chars)
- **Full Description**: Detailed features, installation steps
- **Categories**: Video Players & Editors
- **Tags**: media player, streaming, IPTV, TV, movies

## Development Roadmap

### Planned Features
- [ ] Live TV guide (EPG)
- [ ] Parental controls with PIN
- [ ] Multi-user profiles
- [ ] Chromecast sender support
- [ ] Offline download manager
- [ ] 4K/8K playback optimization
- [ ] Dolby Vision dynamic metadata
- [ ] Game controller support
- [ ] Kodi/Plex integration

## References

- **Android TV Developer Guide**: https://developer.android.com/tv
- **Leanback Library**: https://developer.android.com/jetpack/androidx/releases/leanback
- **ExoPlayer**: https://exoplayer.dev/
- **NanoHTTPD**: https://github.com/NanoHttpd/nanohttpd
- **jCIFS-NG**: https://github.com/AgNO3/jcifs-ng
- **Jetpack Compose for TV**: https://developer.android.com/jetpack/compose/tv

## Testing

### Android TV Emulator
```bash
# Create AVD with Android TV system image
avdmanager create avd -n AndroidTV -k "system-images;android-33;google_atv;x86_64"

# Launch emulator
emulator -avd AndroidTV
```

### Physical Device Testing
- Use ADB over network for wireless debugging
- Test on various devices (Shield TV, Chromecast, Fire TV)
- Verify D-pad navigation works correctly
- Test with different remote controls

## Support

For issues or questions:
- Check Android TV documentation: https://developer.android.com/tv
- Stack Overflow: Tag `android-tv`
- Reddit: r/AndroidTV
- File issues on main Nedflix repository

## License

Same as parent Nedflix project.
