# Nedflix for PlayStation 3

**TECHNICAL DEMO / NOVELTY PORT**

This port demonstrates homebrew development on the PlayStation 3 using the PSL1GHT open-source SDK. It is not intended for production use.

## Requirements

**Note: This port requires Custom Firmware (CFW) or Homebrew Enabler (HEN) on your PS3.**

## Hardware Specifications

| Component | Specification |
|-----------|---------------|
| CPU | 3.2 GHz Cell BE (1 PPE + 6 SPEs) |
| RAM | 256 MB XDR + 256 MB GDDR3 |
| GPU | 550 MHz RSX (NVIDIA-based) |
| Network | Gigabit Ethernet + WiFi |
| Storage | HDD 20-500 GB |

## Design Rationale

### Why PS3?

The PlayStation 3 (2006) is the most capable retro console we target:

1. **Powerful CPU**: The Cell Broadband Engine with 6 available SPEs provides massive parallel processing capability.

2. **Full Network Stack**: Gigabit Ethernet and WiFi enable true streaming.

3. **HD Output**: 720p/1080p output makes this suitable for modern displays.

4. **Large Storage**: Internal HDD means no external media needed.

5. **Active Scene**: PSL1GHT and ps3toolchain are actively maintained.

### Implementation Choices

**RSX Framebuffer Rendering**

We use RSX in framebuffer mode because:
- PSL1GHT provides good framebuffer support
- Avoids complex RSX shader programming
- Sufficient for 2D UI at HD resolutions
- More portable than GPU-specific code

**Cell SPE Audio Processing**

The SPEs are perfect for media processing:
- Can offload audio decoding to SPEs
- Main PPE handles UI and network
- Similar to how retail PS3 media apps work

**Full Network Client**

Unlike limited consoles, PS3 can be a full network client:
- BSD sockets via PSL1GHT
- HTTP client for API communication
- Audio/video streaming from server
- True Nedflix client experience

**HDD Configuration Storage**

With a real filesystem:
- Settings stored on HDD
- No need for memory cards
- Can cache media metadata

### What This Port Can Do

- **Full HD UI** at 720p/1080p
- **Network streaming** via Ethernet/WiFi
- **Audio playback** with SPE-assisted decode
- **Video playback** (with limitations)
- **DualShock 3** controller support
- **Persistent settings** on HDD

### Limitations

1. **CFW/HEN Required**: No official way to run unsigned code.

2. **RSX Complexity**: Full GPU utilization requires more work.

3. **No Blu-ray Access**: Can't use BD drive for media in homebrew.

4. **Memory Split**: 256+256 MB architecture requires careful management.

## Building

### Prerequisites

1. **ps3toolchain**:
   ```bash
   git clone https://github.com/ps3dev/ps3toolchain
   cd ps3toolchain
   sudo ./toolchain.sh
   ```

2. **PSL1GHT**:
   ```bash
   git clone https://github.com/ps3dev/PSL1GHT
   cd PSL1GHT
   make install
   ```

3. **Environment**:
   ```bash
   export PS3DEV=/usr/local/ps3dev
   export PSL1GHT=$PS3DEV/psl1ght
   export PATH=$PS3DEV/bin:$PS3DEV/ppu/bin:$PATH
   ```

### Build Commands

```bash
# Build
./build.sh

# Clean
./build.sh clean
```

### Output

- `nedflix.self` - Signed executable for CFW
- `nedflix.elf` - Unsigned ELF (for debugging)

## Deployment

1. Copy `nedflix.self` to your PS3 via:
   - FTP (if you have FTP server running)
   - USB drive

2. Place in `/dev_hdd0/game/NEDF00001/USRDIR/EBOOT.BIN`
   Or use a homebrew installer

3. Launch from XMB under Games

### Directory Structure

```
/dev_hdd0/game/NEDF00001/
├── PARAM.SFO
├── ICON0.PNG
└── USRDIR/
    └── EBOOT.BIN
```

## Controls

| Button | Action |
|--------|--------|
| X (Cross) | Select / Play-Pause |
| O (Circle) | Back / Stop |
| D-Pad | Navigate |
| L1/R1 | Switch library |
| L2/R2 | Seek / Page |
| Left Stick | Scroll |
| Start | Settings |
| Select | Toggle info |

## Configuration

Settings are stored at:
```
/dev_hdd0/game/NEDF00001/USRDIR/config.dat
```

## Why PS3 is Special

Among our retro targets, PS3 stands out:

| Feature | DC | GC | Xbox | Xbox 360 | PS3 |
|---------|----|----|------|----------|-----|
| Network | Rare | No | Yes | Yes | Yes |
| HD Output | No | No | No | Yes | Yes |
| CPU Power | Low | Low | Med | High | Very High |
| RAM | 16MB | 24MB | 64MB | 512MB | 512MB |
| Storage | VMU | MemCard | HDD | HDD | HDD |
| Video | No | No | Limited | Limited | Yes |

The PS3 can run a nearly full-featured Nedflix client, limited mainly by the homebrew SDK rather than hardware.

## Performance Notes

- UI renders at 60 FPS
- Network latency depends on WiFi vs Ethernet
- Audio streaming is smooth
- Video depends on format and resolution

## Resources

- [PSL1GHT GitHub](https://github.com/ps3dev/PSL1GHT)
- [ps3dev Community](https://github.com/ps3dev)
- [PS3 Homebrew Wiki](https://www.yourwikiname.com)

## Legal Notice

This software is provided for educational purposes. Running homebrew on PS3 requires Custom Firmware which modifies your console. We do not condone piracy. Always own the games you play.
