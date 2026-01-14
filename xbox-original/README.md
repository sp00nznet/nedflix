# Nedflix for Original Xbox (2001)

Build scripts for compiling Nedflix for the original Xbox console (2001).

## ⚠️ Novelty Build - Experimental

**IMPORTANT:** This is an experimental "for fun" build targeting 23-year-old hardware. While the Original Xbox is more capable than the Dreamcast, it still has significant limitations (64MB RAM, 733MHz CPU, DirectX 8.1) that make full modern Nedflix functionality very challenging. This build is provided for:

- **Retro gaming enthusiasts** and Xbox homebrew fans
- **Educational purposes** (embedded development, DirectX 8 programming)
- **Nostalgia projects** and preservation
- **Technical challenge** / "because we can" projects

**Realistic expectations:** Basic media playback might work with heavily optimized content, but don't expect HD streaming or modern codec support!

## Overview

The original Xbox was a gaming console released by Microsoft in 2001. It featured:
- **CPU**: 733 MHz Intel Pentium III "Coppermine-based" processor
- **GPU**: 233 MHz NVIDIA NV2A (DirectX 8.1)
- **RAM**: 64 MB DDR SDRAM
- **Storage**: 8/10 GB internal HDD
- **OS**: Modified Windows 2000 kernel

## Versions

### Client Version
- **Purpose**: Connect to remote Nedflix server
- **Network**: Required for streaming
- **Authentication**: OAuth/Local supported
- **Use case**: Access your media library over home network

### Desktop Version
- **Purpose**: Standalone media player
- **Network**: Not required (offline mode)
- **Authentication**: None (direct access)
- **Use case**: Play media from Xbox HDD or connected USB drives

## Build Requirements

### Development Environment
1. **nxdk Toolchain**
   - Open Xbox Development Kit
   - Download: https://github.com/XboxDev/nxdk
   - Set `NXDK_DIR` environment variable

2. **Dependencies**
   - SDL 1.2 for Xbox
   - DirectX 8.1 libraries
   - Mongoose HTTP server (for Desktop mode)
   - DirectShow filters

### System Requirements
- Linux or Windows with WSL
- GCC cross-compiler for i686-xbox
- 500MB free disk space

## Building

### Windows (Recommended)

Run the interactive build script:
```cmd
cd xbox-original
build.bat
```

The menu provides options for:
- Client builds (Debug/Release)
- Desktop builds (Debug/Release)
- Creating XBE packages
- Installing nxdk toolchain

### Linux/macOS

Use the Unix build script:
```bash
cd xbox-original
chmod +x build.sh
./build.sh
```

Or build directly with make:
```bash
export NXDK_DIR=/path/to/nxdk
cd xbox-original/src
make CLIENT=1          # Client version
make CLIENT=0          # Desktop version
make DEBUG=1 CLIENT=1  # Debug client
```

Output: `src/default.xbe`

## Deployment

### Prerequisites
1. Modded Xbox with custom dashboard (EvolutionX, UnleashX, or XBMC)
2. FTP access enabled
3. Network connection (for Client version)

### Installation Steps
1. **Transfer XBE to Xbox**
   ```bash
   # FTP to your Xbox
   ftp <xbox-ip>
   # Login: xbox / xbox (default)
   cd E:\Apps
   mkdir Nedflix
   cd Nedflix
   put nedflix-client.xbe  # or nedflix-desktop.xbe
   ```

2. **Launch Application**
   - Boot Xbox to dashboard
   - Navigate to Apps > Nedflix
   - Launch the XBE file

3. **First-Time Setup**
   - **Client**: Enter your Nedflix server URL and credentials
   - **Desktop**: Configure local media paths (E:\ or F:\ drives)

## Features

### Supported Features
- Video playback (MPEG-4, XviD, DivX)
- Audio playback (MP3, WMA, AAC)
- Gamepad navigation (Xbox controller)
- Network streaming (Client version)
- Local media playback (Desktop version)
- Resolution support: 480i/480p/720p/1080i
- Surround sound (Dolby Digital 5.1)

### Limitations
- 64 MB RAM limits HD video quality
- No hardware H.264 decoding
- Maximum 1080i output resolution
- Limited to DirectX 8.1 capabilities

## Troubleshooting

### Build Issues
- **"NXDK_DIR not set"**: Export the nxdk installation path
  ```bash
  export NXDK_DIR=/path/to/nxdk
  ```

- **Missing dependencies**: Install nxdk dependencies
  ```bash
  cd $NXDK_DIR
  ./setup.sh
  ```

### Runtime Issues
- **"Unable to connect"** (Client): Check network connection and server URL
- **"No media found"** (Desktop): Ensure media is in E:\ or F:\ drives
- **Black screen**: Check video output settings (480p/720p/1080i)

## Technical Details

### Architecture
- **Binary Format**: XBE (Xbox Executable)
- **Compiler**: GCC i686-xbox target
- **Graphics API**: DirectX 8.1
- **Audio API**: DirectSound
- **Input**: Xbox controller via XInput

### Desktop Mode Implementation
- **HTTP Server**: Mongoose (lightweight, ~60KB)
- **Port**: 3000 (localhost)
- **Web UI**: Embedded in XBE resources
- **API**: RESTful endpoints for media browsing/playback
- **Storage**: FATX filesystem (E:\ and F:\ partitions)

### Client Mode Implementation
- **Protocol**: HTTP/HTTPS
- **Streaming**: Progressive download + buffering
- **Authentication**: OAuth 2.0 or local credentials
- **Network**: 10/100 Ethernet

## Development Notes

This is a **novelty build** for the original Xbox platform. While the build scripts are provided and simulate the build process, a full implementation would require:

1. Complete C/C++ codebase targeting Xbox platform
2. DirectX 8.1 renderer for UI
3. DirectShow filter integration for media playback
4. Custom memory management (64MB constraint)
5. FATX filesystem drivers for local storage
6. Network stack for streaming (Client version)

The original Xbox homebrew scene has limited modern development activity, but projects like XBMC (now Kodi) demonstrated the platform's media capabilities.

## References

- nxdk: https://github.com/XboxDev/nxdk
- Original Xbox specs: https://en.wikipedia.org/wiki/Xbox_(console)
- XboxDev Wiki: https://xboxdevwiki.net/
- XBMC4Xbox: https://github.com/Rocky5/XBMC4Xbox

## License

Same as parent Nedflix project.
