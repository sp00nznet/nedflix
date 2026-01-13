# Nedflix Xbox Series S/X Client

A UWP (Universal Windows Platform) application for streaming your Nedflix media library on Xbox Series S/X consoles.

## Features

- **Full Gamepad Support**: Navigate with D-pad, analog sticks, and standard Xbox controller buttons
- **WebView2 Integration**: Renders the Nedflix web UI natively
- **Xbox Optimized**: Runs in fullscreen with TV-safe zones
- **Server Configuration**: Connect to your Nedflix server on your local network
- **Controller Hints**: On-screen button prompts for easy navigation

## Controller Mapping

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
| **Menu** | Settings |

## Requirements

### Development Machine (Windows)
- Windows 10/11 (64-bit)
- [.NET 6.0 SDK](https://dotnet.microsoft.com/download/dotnet/6.0) or later
- [Windows 10 SDK](https://developer.microsoft.com/windows/downloads/windows-sdk/) (10.0.19041.0 or later)
- Visual Studio 2022 (optional, for IDE support)

### Xbox Console
- Xbox Series S or Xbox Series X
- [Xbox Dev Mode](https://docs.microsoft.com/gaming/xbox-live/get-started/setup-ide/managed-partners/vstudio-xbox/live-where-to-get-xdk) enabled
- Network connection to your Nedflix server

## Building

### Using build.bat (Interactive)
```cmd
cd xbox
build.bat
```
Then select an option from the menu:
1. Build Debug (x64) - For PC testing
2. Build Release (x64) - For Xbox sideload
3. Build Xbox Configuration (x64)
4. Create MSIX Package (for sideload)
5. Build All Configurations
6. Clean Build Output
7. Open in Visual Studio

### Using PowerShell (Command Line)
```powershell
cd xbox

# Build release
.\build.ps1 build -Configuration Release

# Create MSIX package
.\build.ps1 publish

# Deploy to Xbox
.\build.ps1 deploy -XboxIP 192.168.1.100

# Show help
.\build.ps1 help
```

### Using dotnet CLI
```cmd
cd xbox
dotnet restore
dotnet build -c Release -p:Platform=x64
dotnet publish -c Release -p:Platform=x64 -p:AppxPackageSigningEnabled=false
```

## Xbox Dev Mode Setup

### Enabling Dev Mode

1. On your Xbox, go to **Settings** > **System** > **Console info**
2. Note your console's **Xbox Live device ID** (needed for registration)
3. Open the **Microsoft Store** and search for "**Xbox Dev Mode Activation**"
4. Install and launch the app
5. Follow the on-screen instructions to link your console to a Microsoft Partner Center account
6. Restart your Xbox when prompted

### Switching to Dev Mode

1. After restart, your Xbox will show a choice between **Retail** and **Developer** mode
2. Select **Developer Mode**
3. The **Dev Home** app will launch automatically
4. Note your Xbox's **IP address** displayed in Dev Home

### Enabling Device Portal

1. In Dev Home, go to **Remote Access Settings**
2. Enable **Xbox Device Portal**
3. Set a **Username** and **Password** for portal access
4. Note the portal URL (typically `https://<xbox-ip>:11443`)

## Installing on Xbox

### Method 1: Device Portal (Recommended)

1. Build the MSIX package:
   ```cmd
   build.bat
   :: Select option 4
   ```

2. Open a web browser and navigate to:
   ```
   https://<your-xbox-ip>:11443
   ```

3. Accept the certificate warning (self-signed cert is normal)

4. Log in with your Device Portal credentials

5. Navigate to **Apps** in the left sidebar

6. Under **Install app**, click **Add**

7. Browse to the MSIX file:
   ```
   xbox\bin\Release\net6.0-windows10.0.19041.0\win10-x64\publish\Nedflix.Xbox_1.0.0.0_x64.msix
   ```

8. Click **Next**, then **Install**

9. The app will appear in your Xbox's app list

### Method 2: USB Drive

1. Build the MSIX package

2. Copy the `.msix` file to a USB drive

3. Insert the USB drive into your Xbox

4. In Dev Home, go to **My games & apps**

5. Navigate to the USB drive and select the package to install

## Running the App

1. Switch your Xbox to **Developer Mode** (or stay in Dev Mode if already there)

2. Launch **Nedflix** from your apps list

3. On first launch, enter your Nedflix server URL:
   ```
   http://192.168.1.100:3000
   ```
   (Replace with your actual server IP and port)

4. Check "**Remember this server**" to save the setting

5. Press **Connect** to start streaming

## Troubleshooting

### "Connection Error" on launch
- Ensure your Nedflix server is running
- Verify the server URL is correct
- Check that Xbox can reach your server (same network)
- Try `http://` instead of `https://` for local servers

### App won't install
- Ensure Dev Mode is properly activated
- Check that the package is not signed (use `AppxPackageSigningEnabled=false`)
- Verify Windows SDK version compatibility

### Controller not responding
- Press any button to wake the controller
- Check controller is connected to Xbox (not PC)
- Restart the app if issues persist

### Video won't play
- Check your server supports the video format
- Ensure network bandwidth is sufficient
- Try a lower quality setting in Nedflix

### Build errors
- Run `dotnet restore` to fix package issues
- Ensure Windows 10 SDK 10.0.19041.0+ is installed
- Install the "Universal Windows Platform development" workload in Visual Studio

## Project Structure

```
xbox/
├── App.xaml              # Application resources and theme
├── App.xaml.cs           # Application entry point
├── MainWindow.xaml       # Main UI layout
├── MainWindow.xaml.cs    # WebView2 host and gamepad logic
├── Nedflix.Xbox.csproj   # Project file
├── Package.appxmanifest  # UWP manifest (capabilities, icons)
├── app.manifest          # Windows app manifest
├── build.bat             # Interactive build script
├── build.ps1             # PowerShell build/deploy script
├── package.json          # npm scripts wrapper
├── Assets/               # App icons and tiles
│   ├── StoreLogo.png
│   ├── Square44x44Logo.png
│   ├── Square150x150Logo.png
│   └── ...
└── README.md             # This file
```

## Network Requirements

The Xbox app connects to your Nedflix server over HTTP/HTTPS. Ensure:

- Xbox and server are on the same network (or server is accessible)
- Firewall allows connections on port 3000 (or your configured port)
- For HTTPS, the certificate must be valid or Xbox must trust it

## Limitations

- **No Offline Mode**: Requires network connection to Nedflix server
- **Dev Mode Only**: App runs only in Xbox Developer Mode (not Retail)
- **No Store Distribution**: Sideload only (not available on Xbox Store)
- **x64 Only**: ARM builds not supported on Xbox

## License

MIT License - See project root for details.
