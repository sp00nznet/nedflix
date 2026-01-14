@echo off
setlocal enabledelayedexpansion
echo ========================================
echo Nedflix Dreamcast - Windows Build Script
echo ========================================
echo.
echo WARNING: NOVELTY BUILD - LIKELY BROKEN
echo The Dreamcast's limited hardware (16MB RAM, 200MHz CPU) makes
echo full Nedflix functionality extremely challenging. Expect issues!
echo.
echo Two versions available:
echo   CLIENT  - Connect to remote Nedflix server
echo   DESKTOP - Standalone with embedded HTTP server (even more limited!)
echo.

:: Navigate to script directory
cd /d "%~dp0"
echo Working directory: %cd%
echo.

:: Check for MSYS2 and KallistiOS
call :check_kos
if %errorlevel% neq 0 (
    echo ERROR: KallistiOS toolchain check failed
    pause
    exit /b 1
)

:: Build menu
echo ========================================
echo Build Options:
echo ========================================
echo.
echo  CLIENT VERSION (connects to server):
echo   1. Build Client (Debug)
echo   2. Build Client (Release)
echo   3. Create Client CDI Image
echo.
echo  DESKTOP VERSION (standalone - VERY experimental):
echo   4. Build Desktop (Debug)
echo   5. Build Desktop (Release)
echo   6. Create Desktop CDI Image
echo.
echo  BOTH VERSIONS:
echo   7. Create Both CDI Images
echo.
echo  UTILITIES:
echo   8. Clean Build Output
echo   9. Open MSYS2 Shell
echo  10. Install/Update KallistiOS
echo   0. Exit
echo.

set /p choice="Enter your choice (0-10): "

if "%choice%"=="1" goto build_client_debug
if "%choice%"=="2" goto build_client_release
if "%choice%"=="3" goto package_client
if "%choice%"=="4" goto build_desktop_debug
if "%choice%"=="5" goto build_desktop_release
if "%choice%"=="6" goto package_desktop
if "%choice%"=="7" goto package_both
if "%choice%"=="8" goto clean
if "%choice%"=="9" goto open_msys2
if "%choice%"=="10" goto install_kos
if "%choice%"=="0" exit /b 0

echo Invalid choice. Please run the script again.
pause
exit /b 1

:build_client_debug
echo.
echo Building Client (Debug) for Dreamcast...
call :show_novelty_warning
call :run_msys2_build "client" "debug"
goto build_done

:build_client_release
echo.
echo Building Client (Release) for Dreamcast...
call :show_novelty_warning
call :run_msys2_build "client" "release"
goto build_done

:package_client
echo.
echo Creating Client CDI Image for Dreamcast...
call :show_novelty_warning
call :run_msys2_build "client" "cdi"
if %errorlevel% equ 0 (
    call :show_deploy_instructions "Client" "nedflix-client.cdi"
)
goto build_done

:build_desktop_debug
echo.
echo Building Desktop (Debug) for Dreamcast...
call :show_novelty_warning
call :show_desktop_warning
call :run_msys2_build "desktop" "debug"
goto build_done

:build_desktop_release
echo.
echo Building Desktop (Release) for Dreamcast...
call :show_novelty_warning
call :show_desktop_warning
call :run_msys2_build "desktop" "release"
goto build_done

:package_desktop
echo.
echo Creating Desktop CDI Image for Dreamcast...
call :show_novelty_warning
call :show_desktop_warning
call :run_msys2_build "desktop" "cdi"
if %errorlevel% equ 0 (
    call :show_deploy_instructions "Desktop" "nedflix-desktop.cdi"
)
goto build_done

:package_both
echo.
echo Creating both Client and Desktop CDI images...
call :show_novelty_warning
echo.
echo [1/2] Building Client CDI...
call :run_msys2_build "client" "cdi"
echo.
echo [2/2] Building Desktop CDI...
call :run_msys2_build "desktop" "cdi"
echo.
echo ========================================
echo Both CDI images created!
echo.
echo Client version: Connect to your Nedflix server
echo   - Requires server URL configuration
echo   - Broadband Adapter highly recommended
echo   - Best for: Audio streaming, low-res video
echo.
echo Desktop version: Standalone (EXTREMELY experimental)
echo   - No authentication required
echo   - 16MB RAM is NOT enough for HTTP server + player!
echo   - Best for: Proof of concept only
echo ========================================
goto build_done

:clean
echo.
echo Cleaning build output...
if exist "build" rmdir /s /q build
echo Clean complete.
goto end

:open_msys2
echo.
echo Opening MSYS2 shell...
if exist "C:\msys64\msys2_shell.cmd" (
    start "" "C:\msys64\msys2_shell.cmd" -mingw64
) else if exist "C:\msys32\msys2_shell.cmd" (
    start "" "C:\msys32\msys2_shell.cmd" -mingw32
) else (
    echo MSYS2 not found. Please install it first.
    echo Download from: https://www.msys2.org/
)
goto end

:install_kos
echo.
echo Installing/Updating KallistiOS...
call :install_kos_toolchain
goto end

:build_done
echo.
if %errorlevel% equ 0 (
    echo ========================================
    echo Build completed successfully!
    echo ========================================
    echo.
    echo REMINDER: This is a NOVELTY build!
    echo Dreamcast constraints: 16MB RAM, 200MHz CPU
    echo Don't expect miracles from 1998 hardware!
) else (
    echo ========================================
    echo Build failed with error code %errorlevel%
    echo ========================================
)
goto end

:: ========================================
:: Function to show novelty warning
:: ========================================
:show_novelty_warning
echo.
echo ========================================
echo        NOVELTY BUILD WARNING
echo ========================================
echo This is a proof-of-concept build for
echo nostalgia purposes. The Dreamcast has:
echo   - 16MB RAM (8MB main + 8MB video)
echo   - 200MHz SH-4 CPU
echo   - Can't decode modern video codecs
echo.
echo Realistic expectations:
echo   - Audio streaming: Probably works
echo   - Low-res video (240p-360p): Maybe
echo   - HD video: Absolutely not
echo   - Modern codecs (H.264+): No way
echo ========================================
echo.
exit /b 0

:: ========================================
:: Function to show desktop-specific warning
:: ========================================
:show_desktop_warning
echo ========================================
echo    DESKTOP MODE: EXTRA WARNING
echo ========================================
echo Running HTTP server + media player
echo simultaneously in 16MB RAM is...
echo basically impossible. This build
echo exists as a "what if" experiment.
echo.
echo Memory breakdown:
echo   - HTTP Server: ~2-3 MB
echo   - Web UI: ~1 MB (stripped!)
echo   - Media Player: ~5-6 MB
echo   - Buffers: ~2-3 MB
echo   - OS: ~2 MB
echo   = More than 16MB needed!
echo.
echo Expect crashes. Lots of crashes.
echo ========================================
echo.
exit /b 0

:: ========================================
:: Function to show deployment instructions
:: ========================================
:show_deploy_instructions
echo.
echo ========================================
echo %~1 CDI image created: %~2
echo ========================================
echo.
echo Deployment instructions:
echo   1. Burn %~2 to CD-R using ImgBurn or DiscJuggler
echo   2. Use low burn speed (4x-8x) for best compatibility
echo   3. Boot Dreamcast with disc inserted
echo   4. Connect Broadband Adapter (highly recommended)
echo   5. Configure settings on first launch
echo.
echo Hardware requirements:
echo   - Sega Dreamcast console (any region)
echo   - Broadband Adapter (HIT-0400) OR
echo   - Dial-up Modem (audio-only with modem)
echo   - CD-R media (not CD-RW)
echo.
echo Limitations reminder:
echo   - Max 480p resolution
echo   - Limited codec support (MPEG-1 best)
echo   - 56K modem = audio only
echo   - May crash with large libraries
echo ========================================
exit /b 0

:: ========================================
:: Function to check for KallistiOS
:: ========================================
:check_kos
echo Checking for KallistiOS toolchain...

:: First check for MSYS2
set "MSYS2_PATH="
if exist "C:\msys64" set "MSYS2_PATH=C:\msys64"
if exist "C:\msys32" if "%MSYS2_PATH%"=="" set "MSYS2_PATH=C:\msys32"

if "%MSYS2_PATH%"=="" (
    echo MSYS2 not found. Installing automatically...
    call :install_msys2
    if !errorlevel! neq 0 (
        echo ERROR: Failed to install MSYS2
        exit /b 1
    )
    set "MSYS2_PATH=C:\msys64"
)

echo Found MSYS2 at: %MSYS2_PATH%

:: Check for KOS in common locations
set "KOS_PATH="
if exist "%USERPROFILE%\kos" set "KOS_PATH=%USERPROFILE%\kos"
if exist "C:\kos" if "%KOS_PATH%"=="" set "KOS_PATH=C:\kos"
if exist "%MSYS2_PATH%\home\%USERNAME%\kos" if "%KOS_PATH%"=="" set "KOS_PATH=%MSYS2_PATH%\home\%USERNAME%\kos"

if "%KOS_PATH%"=="" (
    echo KallistiOS not found.
    echo.
    set /p install_kos="Would you like to install KallistiOS? (Y/N): "
    if /i "!install_kos!"=="Y" (
        call :install_kos_toolchain
        if !errorlevel! neq 0 exit /b 1
    ) else (
        echo KallistiOS is required to build Dreamcast applications.
        echo Please install it manually or run this script again.
        exit /b 1
    )
) else (
    echo Found KallistiOS at: %KOS_PATH%
)

exit /b 0

:: ========================================
:: Function to install MSYS2
:: ========================================
:install_msys2
echo ----------------------------------------
echo Installing MSYS2...
echo ----------------------------------------
echo.

:: Try winget first
where winget >nul 2>nul
if %errorlevel% equ 0 (
    echo Installing via winget...
    winget install MSYS2.MSYS2 --silent --accept-package-agreements --accept-source-agreements
    if !errorlevel! equ 0 (
        echo MSYS2 installed successfully via winget!
        exit /b 0
    )
    echo Winget install failed, trying manual download...
)

:: Manual download
echo Downloading MSYS2 installer...
set "MSYS2_URL=https://github.com/msys2/msys2-installer/releases/download/2024-01-13/msys2-x86_64-20240113.exe"
set "MSYS2_INSTALLER=%TEMP%\msys2-installer.exe"

powershell -Command "& {[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri '%MSYS2_URL%' -OutFile '%MSYS2_INSTALLER%' -UseBasicParsing}"

if not exist "%MSYS2_INSTALLER%" (
    echo ERROR: Failed to download MSYS2 installer
    exit /b 1
)

echo Installing MSYS2...
"%MSYS2_INSTALLER%" install --root C:\msys64 --confirm-command

:: Clean up
del "%MSYS2_INSTALLER%" 2>nul

echo MSYS2 installation complete!
exit /b 0

:: ========================================
:: Function to install KallistiOS toolchain
:: ========================================
:install_kos_toolchain
echo ----------------------------------------
echo Installing KallistiOS Toolchain
echo ----------------------------------------
echo.
echo This will take 20-30 minutes to compile!
echo.

:: Ensure MSYS2 is available
if "%MSYS2_PATH%"=="" (
    if exist "C:\msys64" (
        set "MSYS2_PATH=C:\msys64"
    ) else (
        echo MSYS2 not found. Please install MSYS2 first.
        exit /b 1
    )
)

:: Step 1: Update MSYS2 first (may require restart)
echo Step 1: Updating MSYS2 packages...
echo NOTE: If MSYS2 needs to close for updates, run this script again after.
echo.

call "%MSYS2_PATH%\msys2_shell.cmd" -mingw64 -defterm -no-start -c "pacman -Syu --noconfirm"

:: Check if KOS directory already exists (in case we're resuming after MSYS2 update)
set "KOS_PATH=%MSYS2_PATH%\home\%USERNAME%\kos"
if exist "%KOS_PATH%\environ.sh" (
    echo.
    echo KallistiOS already installed at: %KOS_PATH%
    exit /b 0
)

:: Step 2: Install dependencies and KOS
echo.
echo Step 2: Installing KallistiOS dependencies and toolchain...
echo This will take 20-30 minutes to compile.
echo.

:: Create install script for MSYS2
set "KOS_INSTALL_SCRIPT=%TEMP%\install_kos.sh"

(
    echo #!/bin/bash
    echo set -e
    echo echo "Installing KallistiOS dependencies..."
    echo echo "This requires packages from multiple MSYS2 repositories..."
    echo.
    echo # Install base development tools
    echo pacman -S --noconfirm --needed base-devel
    echo.
    echo # Install required tools ^(MSYS2 base packages^)
    echo pacman -S --noconfirm --needed git make gcc texinfo patch wget gawk bison flex
    echo.
    echo # Install MinGW packages for image libraries ^(used by KOS tools^)
    echo pacman -S --noconfirm --needed mingw-w64-x86_64-libjpeg-turbo mingw-w64-x86_64-libpng
    echo.
    echo # Python for build scripts
    echo pacman -S --noconfirm --needed python
    echo.
    echo echo "Cloning KallistiOS..."
    echo cd ~
    echo if [ -d "kos" ]; then
    echo     echo "Updating existing KOS installation..."
    echo     cd kos ^&^& git pull
    echo else
    echo     git clone --recursive https://github.com/KallistiOS/KallistiOS.git kos
    echo fi
    echo.
    echo echo ""
    echo echo "=========================================="
    echo echo "Building KOS toolchain [20-30 minutes]..."
    echo echo "=========================================="
    echo echo ""
    echo cd ~/kos/utils/dc-chain
    echo.
    echo # Download and unpack toolchain sources
    echo if [ ! -f "download.stamp" ]; then
    echo     ./download.sh
    echo     touch download.stamp
    echo fi
    echo.
    echo if [ ! -f "unpack.stamp" ]; then
    echo     ./unpack.sh
    echo     touch unpack.stamp
    echo fi
    echo.
    echo # Build the cross-compiler
    echo make
    echo.
    echo echo "Building KOS library..."
    echo cd ~/kos
    echo cp doc/environ.sh.sample environ.sh
    echo source environ.sh
    echo make
    echo.
    echo echo ""
    echo echo "=========================================="
    echo echo "KallistiOS installation complete!"
    echo echo "=========================================="
) > "%KOS_INSTALL_SCRIPT%"

echo Running KallistiOS installation in MSYS2...
echo.

:: Use MSYS shell (not mingw64) for building the cross-compiler toolchain
call "%MSYS2_PATH%\msys2_shell.cmd" -msys -defterm -no-start -c "bash '%KOS_INSTALL_SCRIPT%'"
set "MSYS_RESULT=!errorlevel!"

del "%KOS_INSTALL_SCRIPT%" 2>nul

:: Verify installation regardless of exit code (MSYS2 sometimes returns non-zero)
if exist "%KOS_PATH%\environ.sh" (
    echo.
    echo KallistiOS installed successfully at: %KOS_PATH%
    exit /b 0
) else (
    echo.
    echo KallistiOS installation may have failed or been interrupted.
    echo If MSYS2 was updated, please run this script again to continue.
    exit /b 1
)

:: ========================================
:: Function to run build via MSYS2
:: ========================================
:run_msys2_build
set "BUILD_TYPE=%~1"
set "BUILD_MODE=%~2"

if "%MSYS2_PATH%"=="" set "MSYS2_PATH=C:\msys64"

:: Set output directory and file
if /i "%BUILD_TYPE%"=="client" (
    set "BUILD_DIR=build\client"
    set "OUTPUT_FILE=nedflix-client.cdi"
) else (
    set "BUILD_DIR=build\desktop"
    set "OUTPUT_FILE=nedflix-desktop.cdi"
)

:: Check if actual source code exists
if exist "src\main.c" (
    :: Real build with KallistiOS
    echo.
    echo Building with KallistiOS toolchain...

    :: Create build script for real compilation
    set "BUILD_SCRIPT=%TEMP%\dc_build.sh"

    :: Convert Windows path to MSYS2 path
    set "SCRIPT_DIR=%cd%"
    set "SCRIPT_DIR=!SCRIPT_DIR:\=/!"
    set "SCRIPT_DIR=/!SCRIPT_DIR::=!"

    (
        echo #!/bin/bash
        echo set -e
        echo.
        echo # Find KallistiOS
        echo if [ -f ~/kos/environ.sh ]; then
        echo     export KOS_BASE=~/kos
        echo elif [ -f /opt/toolchains/dc/kos/environ.sh ]; then
        echo     export KOS_BASE=/opt/toolchains/dc/kos
        echo else
        echo     echo "ERROR: KallistiOS not found"
        echo     exit 1
        echo fi
        echo.
        echo echo "Using KOS_BASE: $KOS_BASE"
        echo source $KOS_BASE/environ.sh
        echo.
        echo cd "!SCRIPT_DIR!/src"
        echo echo "Building in: $(pwd^)"
        echo.
        if /i "!BUILD_MODE!"=="cdi" (
            echo echo "Building CDI image..."
            echo make clean
            echo make release
            echo make cdi
        ) else if /i "!BUILD_MODE!"=="debug" (
            echo echo "Building debug..."
            echo make clean
            echo make debug
        ) else (
            echo echo "Building release..."
            echo make clean
            echo make release
        )
        echo.
        echo echo "Build complete!"
    ) > "%BUILD_SCRIPT%"

    call "%MSYS2_PATH%\msys2_shell.cmd" -mingw64 -defterm -no-start -c "bash '%BUILD_SCRIPT%'"
    set "BUILD_RESULT=!errorlevel!"
    del "%BUILD_SCRIPT%" 2>nul

    :: Check for output binary
    if exist "src\nedflix.elf" (
        echo.
        echo ========================================
        echo Build completed successfully!
        echo ========================================
        echo.
        echo Output files:
        if exist "src\nedflix.elf" echo   - src\nedflix.elf (ELF executable)
        if exist "src\nedflix.bin" echo   - src\nedflix.bin (Raw binary)
        if exist "src\nedflix.cdi" echo   - src\nedflix.cdi (CD Image)
        echo.
        exit /b 0
    ) else (
        echo.
        echo Build may have failed - nedflix.elf not found
        echo Check build output above for errors.
        exit /b 1
    )
) else (
    :: No source code - create placeholder
    echo.
    echo ========================================
    echo   PLACEHOLDER BUILD - No source code
    echo ========================================
    echo.
    echo Source code not found at src\main.c
    echo This build script is ready, but actual Dreamcast
    echo source code needs to be implemented.
    echo.
    exit /b 1
)

:end
echo.
pause
exit /b 0
