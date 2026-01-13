# Nedflix Xbox Series S/X

UWP (Universal Windows Platform) applications for streaming video on Xbox Series S/X consoles. Available in two versions to suit different use cases.

## Two Versions Available

### Client Version
Connect to your Nedflix server running on your network.

| Feature | Description |
|---------|-------------|
| **Server Connection** | Connects to remote Nedflix server |
| **Authentication** | Uses server's auth (OAuth, local) |
| **Media Source** | Server's configured media libraries |
| **Best For** | Accessing your home server remotely |

### Desktop Version
Standalone app with embedded server - no external server needed.

| Feature | Description |
|---------|-------------|
| **Server** | Embedded (runs locally on Xbox) |
| **Authentication** | None required |
| **Media Source** | Xbox storage, USB drives |
| **Best For** | Local playback, offline use |

## Controller Mapping

Both versions share the same gamepad controls:

| Button | Action |
|--------|--------|
| **A** | Select / Confirm |
| **B** | Back / Cancel |
| **X** | Play / Pause |
| **Y** | Toggle Fullscreen |
| **LB** | Skip Back 10s |
| **RB** | Skip Forward 10s |
| **LT** | Volume Down |
| **RT** | Volume Up |
| **D-Pad** | Navigate |
| **Left Stick** | Navigate |
| **Menu** | Settings / Refresh |

## Requirements

### Development Machine (Windows)
- Windows 10/11 (64-bit)
- [.NET 6.0 SDK](https://dotnet.microsoft.com/download/dotnet/6.0) or later
- [Windows 10 SDK](https://developer.microsoft.com/windows/downloads/windows-sdk/) (10.0.19041.0+)
- Visual Studio 2022 (optional)

### Xbox Console
- Xbox Series S or Xbox Series X
- [Xbox Dev Mode](https://docs.microsoft.com/gaming/xbox-live/get-started/setup-ide/managed-partners/vstudio-xbox/live-where-to-get-xdk) enabled
- Network connection (Client version) or local media (Desktop version)

## Quick Start

### Build Client Version (for remote server access)
```cmd
cd xbox
build.bat
:: Select option 3 - Package Client for Xbox
```

### Build Desktop Version (standalone, no server)
```cmd
cd xbox
build.bat
:: Select option 6 - Package Desktop for Xbox
```

### Build Both Versions
```cmd
cd xbox
build.bat
:: Select option 7 - Package Both for Xbox
```

## Building with PowerShell

```powershell
cd xbox

# Build Client for Xbox
.\build.ps1 publish -Version Client

# Build Desktop for Xbox
.\build.ps1 publish -Version Desktop

# Show all options
.\build.ps1 help
```

## Building with npm

```bash
cd xbox

# Build Client
npm run publish:client

# Build Desktop
npm run publish:desktop

# Build both
npm run publish:all
```

## Build Configurations

| Configuration | Version | Purpose |
|---------------|---------|---------|
| `Debug` | Client | Development/testing on PC |
| `Client` | Client | Release build for PC testing |
| `ClientXbox` | Client | Xbox deployment package |
| `Desktop` | Desktop | Release build for PC testing |
| `DesktopXbox` | Desktop | Xbox deployment package |

## Xbox Dev Mode Setup

### Enabling Dev Mode

1. Go to **Settings** > **System** > **Console info**
2. Note your console's **Xbox Live device ID**
3. Open **Microsoft Store** and install "**Xbox Dev Mode Activation**"
4. Launch the app and follow instructions
5. Restart Xbox when prompted

### Switching to Dev Mode

1. After restart, select **Developer Mode**
2. **Dev Home** app launches automatically
3. Note your Xbox's **IP address**

### Enabling Device Portal

1. In Dev Home, go to **Remote Access Settings**
2. Enable **Xbox Device Portal**
3. Set **Username** and **Password**
4. Note portal URL: `https://<xbox-ip>:11443`

## Installing on Xbox

### Via Device Portal (Recommended)

1. Build the MSIX package (Client or Desktop)
2. Open browser: `https://<xbox-ip>:11443`
3. Accept certificate warning
4. Log in with Device Portal credentials
5. Navigate to **Apps** > **Install app**
6. Click **Add** and select the `.msix` file:
   - Client: `bin\ClientXbox\...\publish\*.msix`
   - Desktop: `bin\DesktopXbox\...\publish\*.msix`
7. Click **Install**

### Via USB Drive

1. Copy `.msix` file to USB drive
2. Insert USB into Xbox
3. In Dev Home, browse to USB and install

## First Launch

### Client Version
1. Launch **Nedflix** from apps
2. Enter your Nedflix server URL (e.g., `http://192.168.1.100:3000`)
3. Check "**Remember this server**"
4. Press **Connect**

### Desktop Version
1. Launch **Nedflix** from apps
2. App starts immediately with embedded server
3. Configure media paths in settings (Menu button)
4. Browse media from Xbox storage or USB

## Troubleshooting

### Client Version Issues

**"Connection Error"**
- Verify server is running and accessible
- Check server URL format (include `http://`)
- Ensure Xbox and server are on same network

**Video won't stream**
- Check network bandwidth
- Verify server supports the video format
- Try lower quality settings

### Desktop Version Issues

**No media found**
- Configure media paths in settings
- Ensure media is on Xbox storage or USB
- Check file format compatibility

**Embedded server won't start**
- Restart the app
- Check Xbox storage space
- Verify app has storage permissions

### General Issues

**App won't install**
- Ensure Dev Mode is active
- Use unsigned packages (`AppxPackageSigningEnabled=false`)
- Check Windows SDK compatibility

**Controller not responding**
- Press any button to wake controller
- Ensure controller is paired with Xbox
- Restart app if needed

## Project Structure

```
xbox/
├── App.xaml(.cs)           # Application entry point
├── MainWindow.xaml(.cs)    # WebView2 host + gamepad logic
├── Services/
│   └── EmbeddedServer.cs   # Embedded HTTP server (Desktop mode)
├── Nedflix.Xbox.csproj     # Project file with configurations
├── Package.appxmanifest    # UWP manifest
├── build.bat               # Interactive Windows build script
├── build.ps1               # PowerShell build script
├── package.json            # npm scripts
├── WebUI/                  # Web UI files (Desktop mode, auto-copied)
├── Assets/                 # App icons and tiles
└── README.md
```

## Version Comparison

| Feature | Client | Desktop |
|---------|--------|---------|
| Requires server | Yes | No |
| Authentication | Server-based | None |
| Media location | Server libraries | Xbox/USB |
| Offline support | No | Yes |
| Multi-user | Yes (via server) | Single user |
| IPTV support | Yes (via server) | No |
| ErsatzTV | Yes (via server) | No |
| Network required | Yes | No |

## Choosing the Right Version

**Choose Client if you:**
- Have a Nedflix server running at home
- Want to access your full media library
- Need multi-user support
- Use IPTV or ErsatzTV features

**Choose Desktop if you:**
- Want to play local files on Xbox
- Don't have a server set up
- Need offline playback
- Have media on USB drives

## Limitations

- **Dev Mode Only**: Apps run only in Xbox Developer Mode
- **No Store Distribution**: Sideload only (not on Xbox Store)
- **x64 Only**: ARM builds not supported on Xbox
- **Desktop version**: No IPTV/ErsatzTV, limited to local media

## License

MIT License - See project root for details.
