@echo off
setlocal enabledelayedexpansion

:: Nedflix Dreamcast - One-Click Windows Build
:: Automatically configures KOS and builds

cd /d "%~dp0"

:: Find MSYS2
set "MSYS2_PATH="
if exist "C:\msys64" set "MSYS2_PATH=C:\msys64"
if exist "C:\msys32" if "%MSYS2_PATH%"=="" set "MSYS2_PATH=C:\msys32"

if "%MSYS2_PATH%"=="" (
    echo ========================================
    echo  MSYS2 Not Found - Installing...
    echo ========================================
    echo.
    echo MSYS2 is required for Dreamcast development.
    echo.
    choice /C YN /M "Download and install MSYS2 automatically"
    if !errorlevel! equ 2 (
        echo.
        echo Please install MSYS2 manually from https://www.msys2.org/
        pause
        exit /b 1
    )
    echo.
    echo Downloading MSYS2 installer...
    powershell -Command "Invoke-WebRequest -Uri 'https://github.com/msys2/msys2-installer/releases/download/2024-01-13/msys2-x86_64-20240113.exe' -OutFile '%TEMP%\msys2-installer.exe'"
    if !errorlevel! neq 0 (
        echo ERROR: Failed to download MSYS2. Please install manually.
        pause
        exit /b 1
    )
    echo Running MSYS2 installer...
    "%TEMP%\msys2-installer.exe"
    echo.
    echo After MSYS2 installation completes, please run this script again.
    pause
    exit /b 0
)

:: Check if toolchain exists
set "TOOLCHAIN_OK=0"
if exist "%MSYS2_PATH%\opt\toolchains\dc\sh-elf\bin\sh-elf-gcc.exe" set "TOOLCHAIN_OK=1"

if "%TOOLCHAIN_OK%"=="0" (
    echo ========================================
    echo  Dreamcast Toolchain Not Found
    echo ========================================
    echo.
    echo The KallistiOS toolchain needs to be installed.
    echo This will download and compile the toolchain (takes 30-60 minutes^).
    echo.
    choice /C YN /M "Install Dreamcast toolchain automatically"
    if !errorlevel! equ 2 (
        echo.
        echo Please run install_toolchain.bat manually.
        pause
        exit /b 1
    )
    echo.
    echo Installing Dreamcast toolchain...
    echo This will take a while. Please be patient.
    echo.

    :: Create toolchain install script
    set "INSTALL_SCRIPT=%MSYS2_PATH%\tmp\dc_toolchain_install.sh"
    (
        echo #!/bin/bash
        echo set -e
        echo echo "=== Installing Dreamcast Toolchain ==="
        echo.
        echo # Install dependencies
        echo echo "Installing dependencies..."
        echo pacman -Syu --noconfirm
        echo pacman -S --noconfirm --needed base-devel mingw-w64-x86_64-toolchain git wget texinfo bison flex gmp-devel mpc-devel mpfr-devel libpng-devel libjpeg-turbo-devel
        echo.
        echo # Create directories
        echo mkdir -p /opt/toolchains/dc
        echo mkdir -p ~/kos
        echo.
        echo # Clone KallistiOS
        echo if [ ! -d ~/kos/.git ]; then
        echo     echo "Cloning KallistiOS..."
        echo     git clone https://github.com/KallistiOS/KallistiOS.git ~/kos
        echo fi
        echo.
        echo # Build toolchain
        echo cd ~/kos/utils/dc-chain
        echo if [ ! -f config.mk ]; then
        echo     cp config.mk.stable.sample config.mk
        echo fi
        echo.
        echo echo "Downloading toolchain sources..."
        echo ./download.sh
        echo.
        echo echo "Unpacking sources..."
        echo ./unpack.sh
        echo.
        echo echo "Building toolchain (this takes 30-60 minutes^)..."
        echo make
        echo.
        echo # Setup KOS environ.sh
        echo cd ~/kos
        echo if [ ! -f environ.sh ]; then
        echo     cp doc/environ.sh.sample environ.sh
        echo     sed -i 's^|/opt/toolchains/dc"^|/opt/toolchains/dc/sh-elf"^|' environ.sh
        echo fi
        echo.
        echo source environ.sh
        echo echo "Building KOS..."
        echo make
        echo.
        echo echo "=== Toolchain Installation Complete ==="
    ) > "!INSTALL_SCRIPT!"

    "%MSYS2_PATH%\usr\bin\bash.exe" -l "/tmp/dc_toolchain_install.sh"
    set "INSTALL_RESULT=!errorlevel!"
    del "!INSTALL_SCRIPT!" 2>nul

    if !INSTALL_RESULT! neq 0 (
        echo.
        echo ERROR: Toolchain installation failed.
        echo Check the output above for errors.
        pause
        exit /b 1
    )
    echo.
    echo Toolchain installed successfully!
    echo.
)

:: Convert Windows path to MSYS path
set "SRC_DIR=%cd%\src"
set "SRC_DIR=!SRC_DIR:\=/!"
set "SRC_DIR=/!SRC_DIR::=!"

:: Default to client debug build, or use args
set "BUILD_TARGET=debug"
set "CLIENT_FLAG=CLIENT=1"
if "%1"=="release" set "BUILD_TARGET=release"
if "%1"=="desktop" set "CLIENT_FLAG=CLIENT=0"
if "%2"=="release" set "BUILD_TARGET=release"
if "%2"=="desktop" set "CLIENT_FLAG=CLIENT=0"

echo ========================================
echo Nedflix Dreamcast Build
echo ========================================
echo Target: %BUILD_TARGET% %CLIENT_FLAG%
echo.

:: Create the all-in-one build script
set "SCRIPT=%MSYS2_PATH%\tmp\nedflix_build.sh"
(
    echo #!/bin/bash
    echo set -e
    echo.
    echo # KOS paths
    echo export KOS_BASE=~/kos
    echo export KOS_CC_BASE=/opt/toolchains/dc/sh-elf
    echo.
    echo # Fix environ.sh if needed
    echo if [ -f "$KOS_BASE/environ.sh" ]; then
    echo     # Fix wrong toolchain path if present
    echo     if grep -q 'KOS_CC_BASE=.*/opt/toolchains/dc"' "$KOS_BASE/environ.sh" 2^>/dev/null; then
    echo         echo "Fixing KOS_CC_BASE path..."
    echo         sed -i 's^|/opt/toolchains/dc"^|/opt/toolchains/dc/sh-elf"^|' "$KOS_BASE/environ.sh"
    echo     fi
    echo elif [ -f "$KOS_BASE/doc/environ.sh.sample" ]; then
    echo     echo "Creating environ.sh from sample..."
    echo     cp "$KOS_BASE/doc/environ.sh.sample" "$KOS_BASE/environ.sh"
    echo     sed -i 's^|/opt/toolchains/dc"^|/opt/toolchains/dc/sh-elf"^|' "$KOS_BASE/environ.sh"
    echo else
    echo     echo "ERROR: KOS not found at $KOS_BASE"
    echo     exit 1
    echo fi
    echo.
    echo # Source environment
    echo source "$KOS_BASE/environ.sh"
    echo.
    echo # Build KOS libraries if not built
    echo if [ ! -f "$KOS_BASE/kernel/arch/dreamcast/kernel/kernel.o" ]; then
    echo     echo "Building KOS kernel..."
    echo     make -C "$KOS_BASE/kernel" ^>^> /tmp/kos_build.log 2^>^&1 ^|^| { cat /tmp/kos_build.log; exit 1; }
    echo fi
    echo.
    echo if [ ! -f "$KOS_BASE/lib/libkallisti.a" ]; then
    echo     echo "Building KOS libraries..."
    echo     make -C "$KOS_BASE/lib" ^>^> /tmp/kos_build.log 2^>^&1 ^|^| { cat /tmp/kos_build.log; exit 1; }
    echo fi
    echo.
    echo # Build nedflix
    echo echo "Building Nedflix..."
    echo cd "!SRC_DIR!"
    echo make clean 2^>/dev/null ^|^| true
    echo make %BUILD_TARGET% %CLIENT_FLAG%
    echo.
    echo echo "BUILD SUCCESSFUL"
) > "!SCRIPT!"

:: Run it
"%MSYS2_PATH%\usr\bin\bash.exe" -l "/tmp/nedflix_build.sh"
set "BUILD_RESULT=!errorlevel!"

del "!SCRIPT!" 2>nul

if !BUILD_RESULT! equ 0 (
    if exist "src\nedflix.elf" (
        echo.
        echo SUCCESS: src\nedflix.elf
    )
)

pause
exit /b !BUILD_RESULT!
