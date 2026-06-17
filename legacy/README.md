# Legacy ports

These platforms are **no longer actively targeted**. Nedflix now focuses on Web, Desktop
(PC), Mobile (Android/iOS), and TV (Android TV / Apple TV). The projects here are kept for
reference and history — they are experimental/novelty homebrew builds with hardware
limitations and may require modified console firmware.

## Xbox Series X/S — `xbox/`
UWP app for Xbox Series X/S (Dev Mode required). Build: `build.bat` / `build.ps1`.

## Retro console ports — `ports/retro/`

| Platform | Directory | Build | SDK | Notes |
|----------|-----------|-------|-----|-------|
| **Dreamcast** | `ports/retro/dreamcast` | `build.sh` | KallistiOS | Audio streaming focus |
| **GameCube** | `ports/retro/gamecube` | `build.sh` | devkitPPC | Audio playback only |
| **Xbox Original** | `ports/retro/xbox-original` | `build.sh` / `build.bat` | nxdk | Full client, softmod required |
| **PlayStation 3** | `ports/retro/ps3` | `build.sh` | PSL1GHT | Full HD client, CFW required |
| **Xbox 360** | `ports/retro/xbox360` | `build.sh` | libxenon | Full HD client, JTAG/RGH required |
