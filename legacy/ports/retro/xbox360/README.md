# Nedflix for Xbox 360

**TECHNICAL DEMO / NOVELTY PORT**

This port demonstrates homebrew development on the Xbox 360 using the libxenon SDK from the Free60 project. It produces a XEX file that can run on the **Xenia emulator** or on JTAG/RGH modified consoles.

## Quick Start (Xenia Emulator)

```bash
# Install toolchain (first time only)
./build.sh install

# Build XEX file
./build.sh

# Output: nedflix.xex - Open with Xenia Canary
```

## Requirements

### For Xenia Emulator (Recommended for Testing)
- [Xenia Canary](https://xenia.jp/) - Xbox 360 emulator
- No hardware modification required
- Works on Windows, Linux (via Wine/Proton)

### For Real Hardware
**WARNING: This requires a JTAG/RGH modified Xbox 360 console.**

Running unsigned code on an Xbox 360 requires hardware modification. This is for educational and hobbyist purposes only.

## Hardware Specifications

| Component | Specification |
|-----------|---------------|
| CPU | 3.2 GHz IBM PowerPC tri-core (Xenon) |
| RAM | 512 MB GDDR3 (unified) |
| GPU | ATI Xenos 500 MHz |
| Network | 100 Mbps Ethernet, WiFi (later models) |
| Storage | HDD 20-250 GB, USB |

## Building

### Automatic Installation (Recommended)

```bash
# Install libxenon toolchain automatically
./build.sh install

# Build XEX for Xenia
./build.sh
```

### Manual Installation

1. **Install libxenon toolchain**:
   ```bash
   git clone https://github.com/Free60Project/libxenon
   cd libxenon/toolchain
   ./build-toolchain.sh
   ```

2. **Environment setup**:
   ```bash
   export DEVKITXENON=/path/to/libxenon
   export PATH=$DEVKITXENON/bin:$PATH
   ```

3. **Build**:
   ```bash
   ./build.sh
   ```

### Build Commands

```bash
./build.sh              # Build XEX file for Xenia/Xbox 360
./build.sh install      # Install libxenon toolchain
./build.sh xex          # Rebuild XEX only (skip compile if ELF exists)
./build.sh clean        # Clean build artifacts
./build.sh help         # Show help
```

### Output Files

| File | Description |
|------|-------------|
| `nedflix.elf` | PowerPC ELF executable |
| `nedflix.elf32` | 32-bit ELF for XeLL loader |
| `nedflix.xex` | **Xbox 360 executable for Xenia** |

## Running on Xenia

1. Download [Xenia Canary](https://xenia.jp/) (recommended over master branch)
2. Extract and run `xenia_canary.exe`
3. File → Open → Select `nedflix.xex`
4. The app should boot and display the UI

### Xenia Configuration Tips

For best experience in Xenia:
- Enable "Vsync" for smooth rendering
- Set resolution to 720p or 1080p
- Use an Xbox controller or configure keyboard mapping

## Deployment to Real Hardware

### JTAG/RGH Console

1. Copy `nedflix.xex` to USB drive
2. Create directory structure:
   ```
   USB:/
   └── Apps/
       └── Nedflix/
           └── default.xex    (rename nedflix.xex)
   ```
3. Boot your JTAG/RGH Xbox 360
4. Launch from FreeStyle, Aurora, or XeLL

### XeLL Boot

If using XeLL directly:
1. Place `nedflix.elf32` on USB root
2. XeLL will auto-detect and offer to boot

## Controls

| Button | Action |
|--------|--------|
| A | Select / Play-Pause |
| B | Back / Stop |
| X | Play/Pause (alternate) |
| D-Pad Up/Down | Navigate |
| D-Pad Left/Right | Adjust volume |
| LB/RB | Switch library |
| LT/RT | Page up/down, Seek |
| Back | Settings |
| Guide | Settings |

## Architecture

### Design Rationale

**Graphics: Framebuffer Mode**
- Uses Xenos in framebuffer mode for simplicity
- 720p rendering at 60fps
- Compatible with Xenia's GPU emulation

**Audio: PCM Streaming**
- Software PCM playback (no XMA hardware access)
- Streams from server pre-decoded
- Works in both Xenia and real hardware

**Network: lwIP Stack**
- Standard BSD socket API
- DHCP client support
- HTTP client for API communication

### What Works

- UI rendering at 720p
- Xbox 360 controller input
- Network initialization via DHCP
- HTTP client for API communication
- Audio streaming (PCM)
- Configuration persistence

### Limitations

1. **XMA Decode**: Hardware audio decoder not accessible
2. **GPU Shaders**: Limited to framebuffer mode
3. **Video Playback**: Would require significant CPU resources

## Troubleshooting

### Xenia Issues

| Problem | Solution |
|---------|----------|
| Black screen | Try Xenia Canary instead of master |
| Controller not working | Check Xenia input settings |
| Crash on boot | Verify XEX was built correctly |
| Network timeout | Check firewall settings |

### Build Issues

| Problem | Solution |
|---------|----------|
| `DEVKITXENON not set` | Run `./build.sh install` |
| `xenon-gcc not found` | Ensure PATH includes $DEVKITXENON/bin |
| Python errors | Install Python 3: `apt install python3` |

## Development

### Source Structure

```
xbox360/
├── build.sh         # Build script with XEX creation
├── Makefile         # libxenon makefile
├── README.md        # This file
└── src/
    ├── main.c       # Application entry point
    ├── ui.c         # Xenos framebuffer UI
    ├── input.c      # Controller handling
    ├── audio.c      # PCM audio playback
    ├── network.c    # lwIP networking
    ├── api.c        # Server API client
    ├── config.c     # Settings persistence
    ├── json.c       # JSON parser
    └── nedflix.h    # Shared header
```

### Building for Development

```bash
# Quick rebuild after code changes
make -j$(nproc)
./build.sh xex

# Full clean rebuild
./build.sh clean
./build.sh
```

## Resources

- [Xenia Emulator](https://xenia.jp/) - Xbox 360 emulator
- [Free60 Project](https://free60.org/) - Xbox 360 homebrew wiki
- [libxenon GitHub](https://github.com/Free60Project/libxenon) - Homebrew SDK
- [Xenia Compatibility List](https://github.com/xenia-project/game-compatibility)

## Legal Notice

This software is provided for educational purposes.

- **Xenia**: Legal to use with homebrew and games you own
- **Real Hardware**: Requires JTAG/RGH modification (may void warranty)

We do not condone piracy or circumvention of copy protection for commercial games.
