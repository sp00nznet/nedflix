# Nedflix Retro Console Ports

**TECHNICAL DEMOS / NOVELTY PORTS**

These ports demonstrate homebrew development on vintage gaming consoles. They are not intended for production use and serve primarily as educational examples of cross-platform media application development.

## Available Ports

| Console | Year | SDK | Primary Focus | Status |
|---------|------|-----|---------------|--------|
| [Dreamcast](./dreamcast/) | 1998 | KallistiOS | Audio Streaming | Functional |
| [GameCube](./gamecube/) | 2001 | devkitPPC/libogc | Audio Playback | Functional |
| [Xbox Original](./xbox-original/) | 2001 | nxdk | Full Client | Functional |
| [PlayStation 3](./ps3/) | 2006 | PSL1GHT | Full Client | Functional |
| [Xbox 360](./xbox360/) | 2005 | libxenon | Full Client | Functional |

## Port Details

### Sega Dreamcast (1998)
- **CPU:** 200 MHz Hitachi SH-4
- **RAM:** 16 MB main + 8 MB VRAM + 2 MB sound RAM
- **Network:** 56K modem or Broadband Adapter (rare)
- **Focus:** Audio streaming via Yamaha AICA
- **Limitations:** 16MB RAM severely limits video. Network adapters are rare.
- **Local Playback:** WAV files from SD card (via SD adapter)

### Nintendo GameCube (2001)
- **CPU:** 485 MHz IBM Gekko (PowerPC)
- **RAM:** 24 MB (16 MB main + 8 MB ARAM)
- **Network:** None built-in (rare BBA accessory)
- **Focus:** Local audio playback from SD card
- **Limitations:** No network support, no video decode hardware
- **Build:** Requires devkitPPC from devkitPro

### Xbox Original (2001)
- **CPU:** 733 MHz Intel Pentium III
- **RAM:** 64 MB
- **Network:** 10/100 Ethernet (built-in!)
- **Focus:** Full Nedflix client with network streaming
- **Features:** On-screen keyboard for IP configuration, file browser
- **Build:** Requires nxdk

### PlayStation 3 (2006)
- **CPU:** 3.2 GHz Cell BE (1 PPE + 6 SPEs)
- **RAM:** 256 MB XDR + 256 MB GDDR3
- **Network:** Gigabit Ethernet + WiFi
- **Focus:** Full HD client with video support
- **Features:** Most capable port - HD video, SPE-assisted decode
- **Build:** Requires PSL1GHT + ps3toolchain

### Xbox 360 (2005)
- **CPU:** 3.2 GHz PowerPC Xenon (3 cores, 6 threads)
- **RAM:** 512 MB GDDR3 (unified)
- **Network:** 100 Mbps Ethernet (built-in)
- **Focus:** Full HD client with network streaming
- **Features:** HD output, unified memory, hardware-accelerated graphics
- **Build:** Requires libxenon from Free60 project
- **Note:** Requires JTAG/RGH modified console

## Building

Each port has its own build script and Makefile. See the individual port directories for detailed instructions.

```bash
# Example for Dreamcast
cd dreamcast
./build.sh

# Example for GameCube
cd gamecube
./build.sh

# Example for Xbox Original
cd xbox-original
./build.sh     # Linux/macOS
./build.bat    # Windows

# Example for PS3
cd ps3
./build.sh

# Example for Xbox 360
cd xbox360
./build.sh
```

## Testing

- **Dreamcast:** Use Demul, Flycast, or Redream emulators
- **GameCube:** Use Dolphin emulator
- **Xbox Original:** Use xemu emulator or real hardware with softmod
- **PS3:** Requires CFW (Custom Firmware) PS3 or RPCS3 emulator
- **Xbox 360:** Requires JTAG/RGH console or Xenia emulator (limited support)

## Hardware Requirements Summary

| Port | Can Play Video? | Has Network? | Notes |
|------|-----------------|--------------|-------|
| Dreamcast | No | Limited | Audio focus, BBA rare |
| GameCube | No | No | SD card audio only |
| Xbox Original | Limited | Yes | Best 6th gen port |
| PS3 | Yes | Yes | HD capable |
| Xbox 360 | Limited | Yes | HD capable, JTAG/RGH required |

## License

These ports are provided for educational and demonstration purposes. Use of homebrew software may require modified console firmware.
