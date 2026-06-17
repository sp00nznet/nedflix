# Nedflix Xbox Assets

This folder contains the visual assets for the Xbox app, using the reel.png logo.

## Source Image

- `reel.png` - The Nedflix logo (film reel). This is the source for all generated assets.

## Required Assets

The following PNG images are required for the UWP package:

| File | Size | Description |
|------|------|-------------|
| `StoreLogo.png` | 50x50 | Store listing icon |
| `Square44x44Logo.png` | 44x44 | Small tile / taskbar icon |
| `Square44x44Logo.targetsize-24_altform-unplated.png` | 24x24 | Small icon variant |
| `SmallTile.png` | 71x71 | Small tile |
| `Square150x150Logo.png` | 150x150 | Medium tile |
| `Wide310x150Logo.png` | 310x150 | Wide tile |
| `LargeTile.png` | 310x310 | Large tile |
| `SplashScreen.png` | 620x300 | App splash screen |

## Generating Properly Sized Assets

The repository includes placeholder copies of reel.png for each asset. To generate properly sized versions:

### Option 1: Use the generator script (Recommended)
1. Install ImageMagick: https://imagemagick.org/script/download.php
2. Run `generate-assets.bat` on Windows

### Option 2: Use Visual Studio
1. Open the project in Visual Studio 2022
2. Double-click `Package.appxmanifest`
3. Go to "Visual Assets" tab
4. Use "Asset Generator" with `reel.png` as the source image

### Option 3: Manual creation
Create PNG files at the sizes above using your preferred image editor.

## Brand Colors

- Primary: #e94560 (Nedflix red)
- Background: #1a1a2e (Dark blue)
- Text: #ffffff (White)
