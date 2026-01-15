# Nedflix for Xbox 360

**TECHNICAL DEMO / NOVELTY PORT**

This port demonstrates homebrew development on the Xbox 360 using the libxenon SDK from the Free60 project. It is not intended for production use.

## Requirements

**WARNING: This port requires a JTAG/RGH modified Xbox 360 console.**

Running unsigned code on an Xbox 360 requires hardware modification. This is for educational and hobbyist purposes only.

## Hardware Specifications

| Component | Specification |
|-----------|---------------|
| CPU | 3.2 GHz IBM PowerPC tri-core (Xenon) |
| RAM | 512 MB GDDR3 (unified) |
| GPU | ATI Xenos 500 MHz |
| Network | 100 Mbps Ethernet, WiFi (later models) |
| Storage | HDD 20-250 GB, USB |

## Design Rationale

### Why Xbox 360?

The Xbox 360 (2005) represents an interesting target for homebrew:

1. **Powerful Hardware**: The tri-core Xenon CPU with VMX128 extensions and unified memory architecture makes it capable of media streaming.

2. **Network Connectivity**: Built-in Ethernet (and WiFi on later models) enables true client/server operation.

3. **Modern Architecture**: PowerPC architecture is well-documented and toolchains exist.

4. **Active Homebrew Scene**: The Free60 project provides libxenon, a functional homebrew SDK.

### Implementation Choices

**Graphics: Framebuffer Mode**

We use Xenos in framebuffer mode rather than full 3D rendering because:
- libxenon's GPU support is limited compared to the official XDK
- Xenos shader documentation is incomplete
- Framebuffer rendering is simpler and more reliable for UI

**Audio: Software Streaming**

The XMA hardware decoder isn't accessible via libxenon, so we:
- Use software PCM playback
- Stream audio from the server pre-decoded
- Focus on audio-first experience

**Network: lwIP Stack**

libxenon includes lwIP which provides:
- Standard BSD socket API
- DHCP client
- HTTP client capability

### Limitations

1. **Hardware Modification Required**: No way around this - the Xbox 360 has robust security.

2. **No XMA Decode**: We can't use the hardware audio decoder, limiting compressed audio support.

3. **Limited GPU Access**: Complex rendering requires reverse-engineered shader knowledge.

4. **No Video Playback**: Without XMA/hardware decode, video would require significant CPU resources.

### What Works

- UI rendering at 720p
- Xbox 360 controller input (wired USB)
- Network initialization via DHCP
- HTTP client for API communication
- Audio streaming (PCM)
- Configuration save to USB

## Building

### Prerequisites

1. **libxenon toolchain**:
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

### Build Commands

```bash
# Build
./build.sh

# Clean
./build.sh clean
```

### Output

- `nedflix.elf` - Main executable
- `nedflix.elf32` - 32-bit variant for some loaders

## Deployment

1. Copy `nedflix.elf32` to a USB drive
2. Optionally rename to `default.xex`
3. Boot your JTAG/RGH Xbox 360
4. Launch via XeLL or a dashboard that supports ELF loading

### USB Directory Structure

```
USB:/
└── nedflix/
    └── config.dat    (created automatically)
```

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

## Known Issues

1. Controller support requires wired USB controller
2. Network may take several seconds to initialize
3. No persistent storage without USB drive

## Resources

- [Free60 Project](https://free60.org/)
- [libxenon GitHub](https://github.com/Free60Project/libxenon)
- [Xbox 360 Homebrew Wiki](https://www.yourwikiname.com)

## Legal Notice

This software is provided for educational purposes. Using homebrew on Xbox 360 requires hardware modification which may void your warranty. We do not condone piracy or circumvention of copy protection for commercial games.
