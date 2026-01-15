# Nedflix for Sega Dreamcast

**TECHNICAL DEMO / NOVELTY PORT**

This port demonstrates homebrew development on the Sega Dreamcast using KallistiOS. It is not intended for production use.

## Requirements

**Note: Broadband Adapter (BBA) required for network streaming. Most Dreamcasts only have 56K modem.**

## Hardware Specifications

| Component | Specification |
|-----------|---------------|
| CPU | 200 MHz Hitachi SH-4 |
| RAM | 16 MB main + 8 MB VRAM + 2 MB sound RAM |
| GPU | 100 MHz NEC PowerVR2 |
| Sound | Yamaha AICA (ARM7 + 64 channels) |
| Network | 56K modem or Broadband Adapter (rare) |
| Storage | GD-ROM, VMU (128KB), SD via adapter |

## Design Rationale

### Why Dreamcast?

The Dreamcast (1998) was ahead of its time with built-in networking, but has severe constraints:

1. **Extreme RAM Limitation**: Only 16 MB main RAM makes buffering media nearly impossible.

2. **Excellent Audio Hardware**: The Yamaha AICA with dedicated 2 MB sound RAM excels at audio streaming.

3. **Network Capable**: The optional Broadband Adapter provides 10 Mbps Ethernet.

4. **Active Homebrew Scene**: KallistiOS is mature and well-documented.

### Implementation Choices

**Audio Streaming Focus**

We focus primarily on audio because:
- 16 MB RAM can't buffer video frames
- The Yamaha AICA excels at audio streaming
- Audio requires ~256 KB buffers vs MB for video
- The 200 MHz SH-4 can't decode modern video codecs

**PowerVR2 2D Rendering**

We use PowerVR2 in simple 2D mode:
- Tile-based rendering for UI elements
- Text rendering via bitmap fonts
- No complex 3D needed for a media player

**VMU Configuration Storage**

Settings stored on VMU because:
- 128 KB is enough for configuration
- Portable between sessions
- Native save mechanism

**WAV/PCM Focus**

We prioritize uncompressed audio:
- The AICA can play PCM directly
- MP3 decoding taxes the SH-4
- Server can transcode if needed

### What This Port Can Do

- **Audio streaming** via Broadband Adapter
- **Local audio playback** from SD card
- **Basic UI** at 640x480 VGA
- **Controller input** with analog stick
- **VMU storage** for settings

### Limitations

1. **No Video**: 16 MB RAM cannot buffer video frames.

2. **Rare Network Hardware**: The Broadband Adapter is uncommon.

3. **Codec Limits**: MP3 decoding is CPU-intensive; prefer WAV.

4. **Memory Pressure**: Large playlists may cause issues.

5. **56K Modem**: Built-in modem too slow for streaming.

## Building

### Prerequisites

1. **KallistiOS toolchain**:
   ```bash
   git clone https://github.com/KallistiOS/KallistiOS
   cd KallistiOS/utils/dc-chain
   ./download.sh
   ./unpack.sh
   make
   ```

2. **Environment setup**:
   ```bash
   source /path/to/kos/environ.sh
   ```

### Build Commands

```bash
# Linux/macOS
./build.sh

# Windows (MSYS2)
build.bat

# Clean
./build.sh clean
```

### Output

- `nedflix.elf` - Main executable
- `nedflix.bin` - Raw binary
- `nedflix.cdi` - Bootable CD image (with `make cdi`)

## Deployment

### Burning to CD-R

1. Build the CDI image: `./build.sh cdi`
2. Burn using DiscJuggler, ImgBurn, or cdrecord
3. Burn at lowest speed (4x) for reliability

### SD Card (via SD adapter)

1. Copy `nedflix.bin` to SD card
2. Use SD Media Launcher or similar

### Network Boot

1. Use dc-tool-ip for development testing
2. Requires Broadband Adapter

## Controls

| Button | Action |
|--------|--------|
| A | Select / Play-Pause |
| B | Back / Stop |
| X | Play/Pause (alternate) |
| D-Pad Up/Down | Navigate |
| D-Pad Left/Right | Adjust volume |
| L/R Triggers | Switch library |
| Analog Stick | Fast scroll |
| Start | Settings |

## SD Card Setup

If using SD adapter for local playback:

```
SD:/
└── nedflix/
    ├── music/        # Put .wav files here
    └── audiobooks/   # Audiobooks here
```

## Known Issues

1. Large playlists may exhaust memory
2. Network timeouts with slow connections
3. Only WAV recommended for reliability
4. 56K modem unusable for streaming

## Comparison to Other Ports

| Feature | Dreamcast | GameCube | Xbox | PS3 |
|---------|-----------|----------|------|-----|
| RAM | 16 MB | 24 MB | 64 MB | 512 MB |
| Network | Rare | No | Yes | Yes |
| Video | No | No | Limited | Yes |
| Audio | Good | Good | Good | Good |

The Dreamcast port is the most constrained but demonstrates what's possible with creative optimization.

## Resources

- [KallistiOS](https://github.com/KallistiOS/KallistiOS)
- [Dreamcast Specs](https://segaretro.org/Sega_Dreamcast)
- [DCEmulation](https://dcemulation.org/)

## Legal Notice

This software is provided for educational purposes. Running homebrew on Dreamcast typically requires burning to CD-R. We do not condone piracy.
