# Nedflix Xbox Assets

This folder contains the visual assets for the Xbox app.

## Required Assets

The following PNG images are required for the UWP package:

| File | Size | Description |
|------|------|-------------|
| `StoreLogo.png` | 50x50 | Store listing icon |
| `Square44x44Logo.png` | 44x44 | Small tile / taskbar icon |
| `Square44x44Logo.targetsize-24_altform-unplated.png` | 24x24 | Small icon variant |
| `Square71x71Logo.png` | 71x71 | Small tile |
| `Square150x150Logo.png` | 150x150 | Medium tile |
| `Wide310x150Logo.png` | 310x150 | Wide tile |
| `Square310x310Logo.png` | 310x310 | Large tile |
| `SplashScreen.png` | 620x300 | App splash screen |
| `nedflix.ico` | Multi-size | Windows icon file |

## Generating Assets

### Option 1: Use the generator script
Run `generate-assets.bat` on Windows with ImageMagick installed.

### Option 2: Use Visual Studio
1. Open the project in Visual Studio 2022
2. Double-click `Package.appxmanifest`
3. Go to "Visual Assets" tab
4. Use "Asset Generator" to create all sizes from a source image

### Option 3: Manual creation
Create PNG files at the sizes above using your preferred image editor.
Use the Nedflix brand colors:
- Primary: #e94560 (Nedflix red)
- Background: #1a1a2e (Dark blue)
- Text: #ffffff (White)

## Brand Guidelines

- Use the "NEDFLIX" text logo or "N" monogram
- Maintain adequate padding (10-15% of image size)
- Use transparent backgrounds where appropriate
- Ensure sufficient contrast for visibility
