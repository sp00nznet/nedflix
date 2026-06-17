# Nedflix for Nintendo GameCube

**TECHNICAL DEMO / NOVELTY PORT**

This port demonstrates homebrew development on the Nintendo GameCube using devkitPPC and libogc. It is not intended for production use.

## Hardware Specifications

| Component | Specification |
|-----------|---------------|
| CPU | 485 MHz IBM Gekko (PowerPC 750CXe) |
| RAM | 24 MB (16 MB main + 8 MB ARAM) |
| GPU | ATI Flipper 162 MHz |
| Network | None (Broadband Adapter rare) |
| Storage | Memory Card 8MB, SD via adapter |

## Design Rationale

### Why GameCube?

The GameCube (2001) presents unique challenges:

1. **No Built-in Network**: Unlike Xbox, the GameCube has no standard network connectivity. The Broadband Adapter (BBA) is a rare accessory.

2. **Limited RAM**: 24 MB total means careful memory management is critical.

3. **Excellent Audio**: The ARAM (Audio RAM) and DSP provide quality audio playback capabilities.

4. **Active Homebrew Scene**: devkitPPC and libogc are mature, well-documented tools.

### Implementation Choices

**Audio-Only Focus**

We focus exclusively on audio playback because:
- No hardware video decoder exists
- RAM is too limited for video frame buffering
- The DSP and ARAM excel at audio streaming
- Audio files are much smaller than video

**GX Graphics (2D Mode)**

We use GX in 2D orthographic mode:
- Simple quad rendering for UI elements
- Bitmap font rendering (no texture fonts)
- Efficient for menu-driven interfaces
- No complex 3D needed for a media player

**SD Card Storage**

The GameCube lacks built-in storage, so we:
- Support SD cards via SD Gecko or SD Media Launcher adapters
- Store media files on FAT-formatted SD cards
- Save configuration to SD card
- Create default directories on first run

**WAV Format Only**

We only support WAV (PCM) files because:
- No CPU headroom for MP3 decoding
- GameCube DSP doesn't decode MP3
- WAV provides direct PCM playback
- Users can pre-convert files

### Limitations

1. **No Network**: Without the rare BBA, network streaming is impossible.

2. **WAV Files Only**: No compressed audio support (MP3, AAC, OGG).

3. **Memory Constraints**: Large files must stream from SD; can't buffer entire albums.

4. **No Video**: Hardware limitations make video impractical.

### What Works

- UI rendering at 480i/480p
- GameCube controller input (analog + digital)
- SD card filesystem browsing
- WAV audio playback via ASND
- Configuration persistence
- Auto-play next track

## Building

### Prerequisites

1. **devkitPro**:
   - Install from https://devkitpro.org/wiki/Getting_Started
   - Or use the package manager:
     ```bash
     # Debian/Ubuntu
     wget https://apt.devkitpro.org/install-devkitpro-pacman
     chmod +x install-devkitpro-pacman
     sudo ./install-devkitpro-pacman
     sudo dkp-pacman -S gamecube-dev

     # macOS
     brew install devkitpro-pacman
     sudo dkp-pacman -S gamecube-dev
     ```

2. **Environment setup**:
   ```bash
   export DEVKITPRO=/opt/devkitpro
   export DEVKITPPC=$DEVKITPRO/devkitPPC
   export PATH=$DEVKITPPC/bin:$PATH
   ```

### Build Commands

```bash
# Build
./build.sh

# Clean
./build.sh clean

# Run in Dolphin emulator
./build.sh run
```

### Output

- `nedflix.dol` - GameCube executable
- `nedflix.elf` - ELF for debugging

## SD Card Setup

Create this directory structure on a FAT32-formatted SD card:

```
SD:/
└── nedflix/
    ├── music/          # Put .wav files here
    ├── audiobooks/     # Audiobooks go here
    └── config/         # Settings saved here
```

## Converting Audio Files

Convert your audio to WAV format:

```bash
# Using ffmpeg
ffmpeg -i input.mp3 -ar 44100 -ac 2 -f wav output.wav

# Batch convert a folder
for f in *.mp3; do
    ffmpeg -i "$f" -ar 44100 -ac 2 "${f%.mp3}.wav"
done
```

Recommended format:
- Sample rate: 44100 Hz
- Channels: Stereo (2)
- Bit depth: 16-bit
- Format: PCM WAV

## Running

### On Real Hardware

1. You need a way to run homebrew:
   - SD Media Launcher
   - Action Replay with SD adapter
   - Modchip (Xeno GC, etc.)

2. Copy `nedflix.dol` to your SD card
3. Launch using your preferred method

### In Dolphin Emulator

```bash
# Direct launch
./build.sh run

# Or manually
dolphin-emu -e nedflix.dol
```

## Controls

| Button | Action |
|--------|--------|
| A | Select / Play-Pause |
| B | Back / Stop |
| X | Play/Pause (alternate) |
| D-Pad Up/Down | Navigate menu |
| D-Pad Left/Right | Adjust volume |
| L/R Triggers | Switch library |
| Analog Stick | Fast scroll |
| Start | Open settings |

## Known Issues

1. Large WAV files may cause brief loading delays
2. No support for compressed audio formats
3. Memory card saving not implemented (SD only)
4. Some SD adapters may have compatibility issues

## Why This Matters

The GameCube port demonstrates:

1. **Constraint-Driven Design**: How hardware limitations shape software architecture
2. **Audio Processing**: Using ARAM and DSP for efficient audio
3. **Retro Development**: Working with older tools and techniques
4. **Cross-Platform Thinking**: Same app concept, completely different implementation

## Resources

- [devkitPro](https://devkitpro.org/)
- [libogc Documentation](https://libogc.devkitpro.org/)
- [GC-Forever Forums](https://www.gc-forever.com/)
- [Dolphin Emulator](https://dolphin-emu.org/)

## Legal Notice

This software is provided for educational purposes. Running homebrew on GameCube may require hardware modification or special accessories. We do not condone piracy.
