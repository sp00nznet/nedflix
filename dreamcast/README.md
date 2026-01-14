# Nedflix for Sega Dreamcast (1998)

Build scripts for compiling Nedflix for the Sega Dreamcast console.

## ⚠️ NOVELTY BUILD - EXPERIMENTAL ⚠️

**IMPORTANT:** This is a proof-of-concept "for fun" build. The Dreamcast's severe hardware limitations (16MB RAM, 200MHz CPU from 1998) make full Nedflix functionality essentially impossible. This build is provided for:
- **Nostalgia/retro gaming enthusiasts**
- **Educational purposes** (homebrew development)
- **"Because we can"** flex projects
- **Learning about embedded systems**

**Do NOT expect this to work properly!** Consider it a technical curiosity rather than a functional product.

---

## Overview

The Sega Dreamcast was released in 1998/1999 and was ahead of its time with built-in networking capabilities:
- **CPU**: 200 MHz Hitachi SH-4 (SuperH RISC)
- **GPU**: 100 MHz NEC PowerVR2 CLX2
- **RAM**: 16 MB (8MB main + 8MB video)
- **Storage**: GD-ROM (1GB), VMU (128KB save files)
- **Network**: Modem (56K) or Broadband Adapter (10/100 Mbps)
- **OS**: Proprietary Sega OS

---

## Versions

### Client Version
- **Purpose**: Connect to remote Nedflix server
- **Network**: Required (Broadband Adapter recommended)
- **Authentication**: Basic only (OAuth too heavy for RAM)
- **Use case**: Stream audio and low-quality video from server
- **Reality**: Might work for audio, video will struggle

### Desktop Version
- **Purpose**: Standalone media player with embedded server
- **Network**: Optional
- **Authentication**: None (no RAM for it!)
- **Use case**: Play media from SD card
- **Reality**: Running HTTP server + media player in 16MB RAM is... optimistic

---

## Build Requirements

### Development Environment
1. **KallistiOS (KOS)** - Dreamcast homebrew SDK
   - Repository: https://github.com/KallistiOS/KallistiOS
   - Includes SH-4 toolchain
   - Build time: 20-30 minutes

2. **System Dependencies**:
   - git, gcc, make, texinfo
   - libjpeg, libpng
   - python3
   - gmp, mpfr, libmpc, isl

### System Requirements
- Linux, macOS, or Windows (MSYS2)
- 2GB free disk space for toolchain
- 1-2 hours for first-time toolchain build

---

## Building

### Windows (MSYS2)

Run the interactive build script:
```cmd
cd dreamcast
build.bat
```

The menu provides options for:
- Client builds (Debug/Release)
- Desktop builds (Debug/Release)
- Creating CDI images
- Installing KallistiOS toolchain

Requirements: MSYS2 (automatically installed if missing)

### Linux/macOS

Use the Unix build script:
```bash
cd dreamcast
chmod +x build.sh
./build.sh release    # Build release version
./build.sh debug      # Build debug version
./build.sh cdi        # Build and create CDI image
./build.sh clean      # Clean build artifacts
```

Or build directly with make:
```bash
source ~/kos/environ.sh
cd dreamcast/src
make                  # Default build
make debug            # Debug build
make release          # Release build
make cdi              # Create CDI image
```

Output:
- `src/nedflix.elf` - ELF executable
- `src/nedflix.bin` - Raw binary
- `src/nedflix.cdi` - Bootable CD image (with `make cdi`)

---

## Deployment

### Prerequisites
1. **Sega Dreamcast console**
2. **Broadband Adapter** (for Client) or **Modem** (audio only)
3. **CD-R discs** (for burning images)
4. **SD Card Adapter** (for Desktop version) - optional

### Installation Steps

1. **Burn CDI to CD-R**:
   - Use ImgBurn (Windows), DiscJuggler, or cdrecord (Linux)
   - Burn at lowest speed (4x recommended) for reliability
   ```bash
   # Linux example
   cdrecord -v speed=4 dev=/dev/sr0 nedflix-client.cdi
   ```

2. **Boot Dreamcast**:
   - Insert burned CD-R
   - Power on Dreamcast
   - Disc should auto-boot (retail consoles)

3. **Configure (Client version)**:
   - Enter server URL
   - Use Dreamcast controller for navigation
   - Settings saved to VMU

4. **Connect Network** (Client):
   - Broadband Adapter: Ethernet cable
   - Modem: Phone line (audio only, realistically)

---

## What Actually Works (Realistically)

### Client Version: ⚠️ LIMITED
- ✅ **Audio streaming**: Low bitrate MP3/AAC might work
- ⚠️ **Video streaming**: 240p at best, expect stuttering
- ❌ **HD video**: Absolutely not
- ✅ **Navigation**: Should work fine
- ⚠️ **Authentication**: Basic only
- ❌ **OAuth**: Too memory-intensive

### Desktop Version: ⚠️⚠️ VERY LIMITED
- ⚠️ **HTTP server**: Basic, one connection max
- ⚠️ **Audio playback**: Might work from SD card
- ❌ **Video playback**: Probably crashes (not enough RAM)
- ✅ **Text UI**: Minimal interface should load
- ❌ **Full Web UI**: Won't fit in memory

---

## Hardware Limitations (The Reality)

### Memory Constraints
| Component | RAM Usage | Reality |
|-----------|-----------|---------|
| Operating System | ~2 MB | Fixed |
| HTTP Server | ~2-3 MB | If using Desktop |
| Media Player | ~5-6 MB | Buffers + decoder |
| Web UI | ~1-2 MB | Stripped down |
| Buffers | ~2-3 MB | Network/video |
| **TOTAL** | **12-16 MB** | **At the limit!** |

### CPU Constraints
- **200 MHz SH-4**: Can't decode modern codecs
- **MPEG-1**: Barely handles at 320x240
- **H.264**: Forget it
- **Audio**: MP3/AAC at low bitrates okay

### Network Constraints
- **Modem (56K)**: ~6 KB/s → Audio only
- **Broadband**: ~10 Mbps → Low-res video possible
- **Latency**: High compared to modern standards

---

## Supported Features (Theoretical)

### Formats
- **Video**: MPEG-1 (240p max), Motion JPEG (maybe)
- **Audio**: MP3, PCM, ADPCM
- **Resolution**: 320x240 to 640x480 max
- **Frame rate**: 15-24 fps realistic

### Controls
- **Dreamcast Controller**: D-pad and analog stick
- **VMU**: Settings storage (128KB)
- **Keyboard**: If plugged in (text entry)

---

## Known Issues (Expected)

### Client Version
1. **Crashes on large playlists** (RAM overflow)
2. **Video stuttering** (CPU can't keep up)
3. **Network timeouts** (slow decoder)
4. **Limited codec support** (no H.264)
5. **Modem unusable** (too slow for video)

### Desktop Version
1. **Crashes immediately** (server + player = too much RAM)
2. **Text-only UI** (no images to save memory)
3. **Single connection only** (not enough RAM for multiple)
4. **Slow response** (CPU bottleneck)
5. **Limited media formats** (no modern codecs)

---

## Troubleshooting

### "Disc won't boot"
- Burn at lower speed (4x or lower)
- Use high-quality CD-R media
- Check Dreamcast lens is clean
- Try different burning software

### "Crashes on startup"
- RAM overflow likely
- Try with smaller media library
- Reduce video quality settings
- Use audio-only mode

### "Network connection failed"
- Check Broadband Adapter connection
- Verify server is accessible
- Use static IP instead of DHCP
- Firewall may be blocking

### "Video won't play"
- Encode at 240p, 15fps, low bitrate
- Use MPEG-1 codec only
- Audio-only streaming more reliable
- RAM constraints likely causing issues

---

## Development Notes

This is a **NOVELTY BUILD** for educational/nostalgic purposes. A full implementation would require:

1. **Complete C codebase** targeting KallistiOS
2. **Custom PowerVR2 renderer** for UI
3. **Optimized MPEG-1 decoder** for SH-4
4. **Minimal HTTP client** (Client) or server (Desktop)
5. **Extreme memory optimization** (every byte counts!)
6. **Custom UI** (text-based to save RAM)
7. **VMU filesystem integration** for saves

The Dreamcast homebrew scene is small but active. Projects like DreamMP3 and DreamDVD show what's possible, but even those are limited to basic playback.

### Realistic Project Goals
Instead of full Nedflix, consider:
- **Audio streaming client** (realistic!)
- **Slideshow viewer** (works well)
- **Simple remote control** (totally doable)
- **VMU-based playlist manager** (creative use!)

---

## Comparison to Original Xbox

While both are "retro novelty builds", there's a huge difference:

| Feature | Dreamcast (1998) | Original Xbox (2001) |
|---------|------------------|---------------------|
| RAM | 16 MB | 64 MB |
| CPU | 200 MHz SH-4 | 733 MHz P3 |
| GPU | PowerVR2 | NV2A (GeForce 3) |
| Storage | GD-ROM (1GB) | HDD (8-10 GB) |
| **Viability** | **Audio only** | **Actually possible** |

The Original Xbox build is challenging but feasible. The Dreamcast build is... aspirational. 😅

---

## References

- **KallistiOS**: https://github.com/KallistiOS/KallistiOS
- **Dreamcast Specs**: https://segaretro.org/Sega_Dreamcast
- **DC-Dev Wiki**: http://dcemulation.org/
- **Dreamcast Programming**: https://dc.scummvm.org/
- **Homebrew Scene**: https://dcemulation.org/phpBB/

---

## Acknowledgments

The Dreamcast was an amazing console that was ahead of its time in 1999. While Nedflix is way too complex for it in 2026, this build celebrates the spirit of pushing hardware to its limits!

RIP Dreamcast (1998-2001) - It's thinking... 🌀

---

## License

Same as parent Nedflix project.

**Disclaimer**: This is a fan project. Sega and Dreamcast are trademarks of Sega Corporation.
