# Build Script Automation Documentation

This document explains the automatic dependency installation system implemented across all Nedflix build scripts.

## Overview

All Nedflix build scripts now automatically install required dependencies without user intervention. Users can simply run the build script and it will handle everything from toolchain installation to building the final package.

## Build Scripts Summary

| Platform | Script | Auto-Installs | OS Support |
|----------|--------|---------------|------------|
| **Desktop (Electron)** | `desktop/build.sh` | Node.js 20 LTS | Linux (apt, dnf, pacman, zypper) |
| **Desktop (Electron)** | `desktop/build.bat` | Node.js 20 LTS | Windows |
| **Xbox Series X/S** | `xbox/build.ps1` | .NET SDK 8.0 | Windows (winget, choco, direct download) |
| **Xbox Series X/S** | `xbox/build.bat` | .NET SDK 8.0 | Windows (winget, choco, direct download) |
| **Original Xbox** | `xbox-original/build-client.sh` | nxdk + system deps | Linux (apt, dnf, pacman), macOS (Homebrew) |
| **Original Xbox** | `xbox-original/build-desktop.sh` | nxdk + system deps | Linux (apt, dnf, pacman), macOS (Homebrew) |
| **Apple TV** | `appletv/build-client.sh` | Xcode CLI, Homebrew, CocoaPods | macOS only |
| **Apple TV** | `appletv/build-desktop.sh` | Xcode CLI, Homebrew, CocoaPods | macOS only |
| **Android TV** | `androidtv/build-client.sh` | JDK 17, Android SDK, Gradle | Linux (apt, dnf, pacman), macOS (Homebrew) |
| **Android TV** | `androidtv/build-desktop.sh` | JDK 17, Android SDK, Gradle | Linux (apt, dnf, pacman), macOS (Homebrew) |

---

## Detailed Platform Documentation

### 1. Desktop (Electron) - Linux

**Script**: `desktop/build.sh`

**What it installs**:
- Node.js 20 LTS via NodeSource repository (Debian/Ubuntu)
- Node.js + npm via system package manager (Fedora, Arch, openSUSE)

**Installation method**:
```bash
# Detects package manager and uses appropriate method:
# - apt-get (Debian/Ubuntu): NodeSource repository
# - dnf (Fedora/RHEL): Official repos
# - pacman (Arch Linux): Official repos
# - zypper (openSUSE): Official repos
```

**How it works**:
1. Checks if `node` command exists
2. If not found, calls `install_nodejs()` function
3. Function detects OS and package manager
4. Installs Node.js using the appropriate method
5. Automatically runs `npm install` for dependencies

**Testing**: To test, uninstall Node.js and run:
```bash
cd desktop && ./build.sh
```

---

### 2. Desktop (Electron) - Windows

**Script**: `desktop/build.bat`

**What it installs**:
- Node.js 20 LTS (x64 or x86 depending on architecture)

**Installation method**:
- Downloads official MSI installer from nodejs.org
- Runs silent installation via msiexec

**How it works**:
1. Checks if `node` command exists using `where node`
2. If not found, downloads MSI installer to %TEMP%
3. Detects CPU architecture (AMD64 vs x86)
4. Downloads appropriate installer (x64 or x86)
5. Runs: `msiexec /i installer.msi /qn /norestart`
6. Falls back to interactive install if silent fails
7. Refreshes PATH and checks for Node.js in common locations

**Testing**: To test, uninstall Node.js and run:
```batch
cd desktop && build.bat
```

---

### 3. Xbox Series X/S - PowerShell

**Script**: `xbox/build.ps1`

**What it installs**:
- .NET SDK 8.0

**Installation methods** (tries in order):
1. **winget** (Windows 11 / Windows 10 with App Installer)
2. **Chocolatey** (if installed)
3. **Direct download** (MSI from Microsoft)

**How it works**:
1. Checks if `dotnet` command exists
2. If not found, tries winget first:
   ```powershell
   winget install Microsoft.DotNet.SDK.8 --silent
   ```
3. Falls back to Chocolatey:
   ```powershell
   choco install dotnet-sdk -y
   ```
4. Final fallback: Downloads .NET SDK installer and runs it
5. Refreshes environment variables after installation
6. Provides guidance if PowerShell restart is needed

**Testing**: To test, uninstall .NET SDK and run:
```powershell
cd xbox
.\build.ps1 build -Version Client
```

---

### 4. Xbox Series X/S - Batch

**Script**: `xbox/build.bat`

**What it installs**:
- .NET SDK 8.0

**Installation methods** (tries in order):
1. **winget** (Windows 11 / Windows 10 with App Installer)
2. **Chocolatey** (if installed)
3. **Direct download** (EXE from Microsoft)

**How it works**:
1. Checks if `dotnet` command exists using `where dotnet`
2. Calls `:install_dotnet_sdk` subroutine if not found
3. Tries installation methods in priority order
4. Calls `:refresh_path` to update PATH from registry
5. Verifies installation was successful

**Refresh PATH function**:
```batch
:refresh_path
for /f "tokens=2*" %%a in ('reg query "HKLM\...\Environment" /v Path') do set "SYS_PATH=%%b"
for /f "tokens=2*" %%a in ('reg query "HKCU\Environment" /v Path') do set "USER_PATH=%%b"
set "PATH=%SYS_PATH%;%USER_PATH%"
```

**Testing**: To test, uninstall .NET SDK and run:
```batch
cd xbox && build.bat
```

---

### 5. Original Xbox (2001)

**Scripts**: `xbox-original/build-client.sh`, `xbox-original/build-desktop.sh`

**What it installs**:
- **System dependencies**: git, cmake, flex, bison, clang, lld, llvm
- **nxdk toolchain**: Xbox homebrew development kit

**Installation methods**:
- **Linux**: apt (Debian/Ubuntu), dnf (Fedora), pacman (Arch)
- **macOS**: Homebrew (auto-installs Homebrew if missing)
- **Windows**: Manual installation instructions

**How it works**:
1. Checks if `$NXDK_DIR` environment variable is set
2. If not, checks default location: `$HOME/nxdk`
3. If not found, installs system dependencies first
4. Clones nxdk from GitHub:
   ```bash
   git clone --recursive https://github.com/XboxDev/nxdk.git $HOME/nxdk
   ```
5. Builds nxdk toolchain: `./build.sh`
6. Exports `NXDK_DIR` for current session
7. Provides shell config command for permanent use

**Desktop version differences**:
- Desktop build also copies Web UI files from `desktop/public` to `xbox-original/build/desktop/webui`
- Includes embedded HTTP server (mongoose) for standalone operation

**Testing**: To test:
```bash
unset NXDK_DIR
cd xbox-original && ./build-client.sh
```

---

### 6. Apple TV (tvOS)

**Scripts**: `appletv/build-client.sh`, `appletv/build-desktop.sh`

**What it installs**:
1. **Xcode Command Line Tools**
2. **Homebrew** (if not installed)
3. **CocoaPods**
4. **Xcode** (provides installation instructions)
5. **tvOS SDK** (prompts to install via Xcode)

**Installation methods**:
- Xcode CLI Tools: `xcode-select --install` (launches installer)
- Homebrew: Official install script
- CocoaPods: `sudo gem install cocoapods`

**How it works**:
1. Checks if `xcodebuild` command exists
2. Verifies macOS platform (tvOS requires macOS)
3. Installs Xcode CLI Tools if missing (interactive prompt)
4. Checks for Homebrew, installs if missing
5. Handles Apple Silicon (M1/M2) PATH configuration
6. Installs CocoaPods via RubyGems
7. Accepts Xcode license automatically: `sudo xcodebuild -license accept`
8. Checks for tvOS SDK in Xcode
9. Prompts user to install tvOS via Xcode > Settings > Platforms if missing

**Apple Silicon support**:
```bash
if [[ $(uname -m) == "arm64" ]]; then
    echo 'eval "$(/opt/homebrew/bin/brew shellenv)"' >> ~/.zprofile
    eval "$(/opt/homebrew/bin/brew shellenv)"
fi
```

**Desktop version differences**:
- Copies Web UI from `desktop/public` to `appletv/build/desktop/WebUI`
- Bundles Web UI in app resources for standalone operation
- Uses GCDWebServer for embedded HTTP server

**Testing**: To test:
```bash
cd appletv && ./build-client.sh
```

---

### 7. Android TV

**Scripts**: `androidtv/build-client.sh`, `androidtv/build-desktop.sh`

**What it installs**:
1. **JDK 17+** (OpenJDK)
2. **Android SDK command-line tools**
3. **Android SDK packages** (platform-tools, build-tools 33.0.0, platforms;android-33)
4. **Gradle** (optional, falls back to wrapper)

**Installation methods**:
- **Linux**: apt (Debian/Ubuntu), dnf (Fedora), pacman (Arch)
- **macOS**: Homebrew
- **Android SDK**: Direct download from Google

**How it works**:

**JDK Installation**:
1. Checks if `java` command exists
2. Validates Java version is 17 or higher
3. If too old or missing, installs via package manager:
   - Linux (apt): `openjdk-17-jdk`
   - Linux (dnf): `java-17-openjdk-devel`
   - macOS: `brew install openjdk@17`

**Android SDK Installation**:
1. Checks `$ANDROID_HOME` environment variable
2. Checks default locations:
   - `$HOME/Android/Sdk` (Linux)
   - `$HOME/Library/Android/sdk` (macOS)
   - `/usr/local/android-sdk`
3. If not found, downloads command-line tools:
   ```bash
   # Linux
   https://dl.google.com/android/repository/commandlinetools-linux-11076708_latest.zip
   # macOS
   https://dl.google.com/android/repository/commandlinetools-mac-11076708_latest.zip
   ```
4. Extracts to `$HOME/Android/Sdk/cmdline-tools/latest`
5. Accepts all SDK licenses: `yes | sdkmanager --licenses`
6. Installs required packages:
   ```bash
   sdkmanager "platform-tools" "platforms;android-33" "build-tools;33.0.0"
   ```

**Gradle Installation**:
- Tries system package manager first
- Falls back to Gradle wrapper if system install fails

**Desktop version differences**:
- Copies Web UI from `desktop/public` to `androidtv/app/src/main/assets/webui`
- Uses NanoHTTPD (lightweight Java HTTP server) for embedded operation
- Supports SMB/NFS network shares for media access

**Testing**: To test:
```bash
unset ANDROID_HOME
cd androidtv && ./build-client.sh
```

---

## Common Patterns

### OS Detection

All bash scripts use this pattern:
```bash
detect_os() {
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        echo "linux"
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        echo "macos"
    elif [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]]; then
        echo "windows"
    else
        echo "unknown"
    fi
}
```

### Package Manager Detection (Linux)

```bash
if command -v apt-get &> /dev/null; then
    # Debian/Ubuntu
elif command -v dnf &> /dev/null; then
    # Fedora/RHEL
elif command -v pacman &> /dev/null; then
    # Arch Linux
fi
```

### Colored Output

All scripts use ANSI color codes:
```bash
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

echo -e "${GREEN}Success!${NC}"
```

### Environment Variable Export

Scripts export variables for current session and provide instructions for permanent configuration:
```bash
export ANDROID_HOME="$install_dir"
echo -e "${YELLOW}Add this to your ~/.bashrc or ~/.zshrc:${NC}"
echo -e "${CYAN}export ANDROID_HOME=$install_dir${NC}"
```

---

## Installation Priority Order

### Windows Scripts
1. **winget** (Windows 11 / Windows 10 2022+)
2. **Chocolatey** (if user has it installed)
3. **Direct download** (MSI/EXE from official source)

### Linux Scripts
1. **System package manager** (apt, dnf, pacman)
2. **Official repository** (if available, e.g., NodeSource for Node.js)
3. **Direct download** (as fallback)

### macOS Scripts
1. **Homebrew** (auto-installs if missing)
2. **Official installer** (if Homebrew unavailable)

---

## Error Handling

All scripts follow this error handling pattern:

1. **Check for tool**: Test if command exists
2. **Auto-install**: Call installation function
3. **Verify install**: Check if command now exists
4. **PATH refresh**: Update environment variables
5. **Final check**: Verify tool is accessible
6. **Fallback message**: Provide manual installation instructions if all automated methods fail

Example:
```bash
if ! command -v tool &> /dev/null; then
    install_tool
    if ! command -v tool &> /dev/null; then
        echo "ERROR: Installation failed"
        echo "Please install manually from: https://..."
        exit 1
    fi
fi
```

---

## Testing the Auto-Install System

### Test Procedure

For each script:

1. **Uninstall the dependency**:
   ```bash
   # Example for Node.js (Linux)
   sudo apt remove nodejs npm

   # Example for .NET SDK (Windows)
   winget uninstall Microsoft.DotNet.SDK.8
   ```

2. **Clear environment variables** (if applicable):
   ```bash
   unset ANDROID_HOME
   unset NXDK_DIR
   ```

3. **Run the build script**:
   ```bash
   cd <platform>
   ./<build-script>
   ```

4. **Verify**:
   - Script detects missing dependency
   - Auto-installation begins
   - Installation completes successfully
   - Build continues without errors

---

## Maintenance

### Updating Dependency Versions

When updating to newer versions, modify these variables:

**Node.js (desktop/build.bat)**:
```batch
set "NODE_VERSION_DL=v20.11.0"
set "NODE_MSI=node-%NODE_VERSION_DL%-%NODE_ARCH%.msi"
```

**.NET SDK (xbox/build.ps1)**:
```powershell
$dotnetVersion = "8.0.1"
$installerUrl = "https://download.visualstudio.microsoft.com/..."
```

**Android SDK (androidtv/build-client.sh)**:
```bash
cmdtools_url="https://dl.google.com/android/repository/commandlinetools-linux-11076708_latest.zip"
```

### Testing New Versions

1. Update the URL/version in the script
2. Test on a clean VM or container
3. Verify installation succeeds
4. Verify build completes successfully

---

## Troubleshooting

### Common Issues

**"Command not found after install"**
- Cause: PATH not refreshed
- Solution: Restart terminal or run script again

**"Permission denied"**
- Cause: Installer requires admin/sudo
- Solution: Run with elevated privileges or accept UAC prompt

**"Download failed"**
- Cause: Network issues or URL changed
- Solution: Check internet connection, update URL in script

**"Installation succeeded but build fails"**
- Cause: Additional dependencies required
- Solution: Check build output for specific missing dependencies

### Debug Mode

Add `-x` flag to bash scripts for debug output:
```bash
bash -x ./build-client.sh
```

Or add at top of script:
```bash
set -x  # Enable debug mode
```

---

## Future Improvements

Potential enhancements for the auto-install system:

1. **Version validation**: Check if installed version meets minimum requirements
2. **Caching**: Cache installers to avoid re-downloading
3. **Parallel installs**: Install multiple dependencies concurrently
4. **Proxy support**: Handle corporate proxy environments
5. **Offline mode**: Use pre-downloaded installers
6. **Progress bars**: Visual feedback during downloads
7. **Rollback**: Undo installation if build fails

---

## Summary

All Nedflix build scripts now provide:

✅ **Zero manual setup** - Just run the script
✅ **Cross-platform** - Works on Windows, Linux, macOS
✅ **Multiple package managers** - Tries best method for each OS
✅ **Fallback options** - Multiple installation methods
✅ **Clear feedback** - Colored output with progress indicators
✅ **Error recovery** - Helpful messages when auto-install fails
✅ **Environment setup** - Exports variables and provides config commands

Users can now build Nedflix for any platform without reading documentation or manually installing dependencies!
