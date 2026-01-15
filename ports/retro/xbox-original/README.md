# Nedflix for Original Xbox

**TECHNICAL DEMO / NOVELTY PORT**

This port demonstrates homebrew development on the original Xbox (2001) using the nxdk open-source SDK. It is not intended for production use.

## Requirements

**Note: This port requires a softmodded or hardmodded Xbox to run unsigned code.**

## Hardware Specifications

| Component | Specification |
|-----------|---------------|
| CPU | 733 MHz Intel Pentium III |
| RAM | 64 MB DDR SDRAM |
| GPU | 233 MHz NVIDIA NV2A (DirectX 8.1) |
| Network | 10/100 Mbps Ethernet (built-in) |
| Storage | 8-10 GB internal HDD |

## Design Rationale

### Why Original Xbox?

The original Xbox (2001) is the most capable 6th-generation console for homebrew:

1. **Built-in Ethernet**: Every Xbox has 10/100 Mbps networking, enabling true client/server operation.

2. **Substantial RAM**: 64 MB is enough for reasonable video buffering.

3. **x86 Architecture**: Standard PC-like architecture is familiar to developers.

4. **Internal HDD**: Persistent storage without memory cards.

5. **Active Scene**: nxdk is maintained and XBMC proved the platform's media capabilities.

### Implementation Choices

**DirectX 8.1 Rendering**

We use the NV2A GPU via DirectX 8.1:
- Hardware-accelerated 2D/3D rendering
- Well-documented API
- Sufficient for UI at 480p/720p

**Full Network Client**

Unlike other 6th-gen consoles, Xbox can be a full streaming client:
- BSD-like socket API
- HTTP client for API communication
- Audio/video streaming over network
- No rare accessories needed

**HDD Storage**

With an internal hard drive:
- Settings saved to E:\ or F:\ partitions
- Can cache media metadata
- No memory card management

**On-Screen Keyboard**

For server URL entry:
- Virtual keyboard navigable with controller
- IP address and credential input
- Saved to HDD for next session

### What This Port Can Do

- **Network streaming** via built-in Ethernet
- **Video playback** at 480p (with limitations)
- **Audio playback** (MP3, WMA, AAC)
- **Full HD UI** at 480p/720p/1080i
- **Xbox controller** input
- **Persistent settings** on HDD

### Limitations

1. **Softmod Required**: No official way to run unsigned code.

2. **No H.264**: Hardware only supports older codecs (MPEG-4, XviD, DivX).

3. **64 MB RAM**: Limits HD video quality.

4. **DirectX 8.1**: No shader model 2.0 or later.

## Building

### Prerequisites

1. **nxdk toolchain**:
   ```bash
   git clone --recursive https://github.com/XboxDev/nxdk
   cd nxdk
   ./setup.sh
   ```

2. **Environment setup**:
   ```bash
   export NXDK_DIR=/path/to/nxdk
   ```

### Build Commands

```bash
# Linux/macOS
./build.sh

# Windows
build.bat

# Clean
./build.sh clean
```

### Output

- `default.xbe` - Xbox executable

## Deployment

### Via FTP

1. Enable FTP on your modded Xbox (UnleashX, XBMC, etc.)
2. Connect: `ftp <xbox-ip>` (default: xbox/xbox)
3. Upload to `E:\Apps\Nedflix\default.xbe`
4. Launch from dashboard

### Via USB

1. Copy `default.xbe` to USB drive
2. Use file manager on Xbox to copy to HDD
3. Launch from Apps menu

### Directory Structure

```
E:\Apps\Nedflix\
├── default.xbe
└── config.dat    (created on first run)
```

## Controls

| Button | Action |
|--------|--------|
| A | Select / Play-Pause |
| B | Back / Stop |
| X | Play/Pause (alternate) |
| D-Pad Up/Down | Navigate |
| D-Pad Left/Right | Adjust volume |
| L/R Triggers | Switch library / Seek |
| Left Stick | Navigate / Scroll |
| Right Stick | Seek (during playback) |
| Start | Settings |
| Back | Open keyboard |

## Configuration

On first run, you'll be prompted to enter:
- Server URL (e.g., `http://192.168.1.100:3000`)
- Username and password (if required)

Settings are stored in `config.dat` on the HDD.

## Known Issues

1. Some HTTPS servers may not work (limited SSL support)
2. High-bitrate video may stutter
3. Network configuration is DHCP only
4. Some video codecs require transcoding

## Why Xbox is Best 6th-Gen Port

| Feature | Dreamcast | GameCube | Xbox |
|---------|-----------|----------|------|
| Network | Rare | No | Yes |
| RAM | 16 MB | 24 MB | 64 MB |
| Storage | VMU | MemCard | HDD |
| Video | No | No | Yes |
| CPU | 200 MHz | 485 MHz | 733 MHz |

The original Xbox is the only 6th-generation console where a full media streaming client is practical.

## Resources

- [nxdk](https://github.com/XboxDev/nxdk)
- [XboxDev Wiki](https://xboxdevwiki.net/)
- [XBMC4Xbox](https://github.com/Rocky5/XBMC4Xbox)

## Legal Notice

This software is provided for educational purposes. Running homebrew on Xbox requires modification which may void warranty. We do not condone piracy.
