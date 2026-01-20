@echo off
REM ============================================
REM Nedflix Nintendo Switch - Windows Build Script
REM One-click build using devkitPro
REM ============================================

setlocal enabledelayedexpansion

echo.
echo ============================================
echo   Nedflix Nintendo Switch Build (Windows)
echo ============================================
echo.

REM Check for devkitPro in common locations
set "DEVKITPRO="
if exist "C:\devkitPro" set "DEVKITPRO=C:\devkitPro"
if exist "D:\devkitPro" set "DEVKITPRO=D:\devkitPro"
if exist "%USERPROFILE%\devkitPro" set "DEVKITPRO=%USERPROFILE%\devkitPro"

REM Check for MSYS2 installation (legacy)
set "MSYS_PATH="
if exist "C:\msys64\opt\devkitpro" (
    set "MSYS_PATH=C:\msys64"
    if not defined DEVKITPRO set "DEVKITPRO=C:\msys64\opt\devkitpro"
)

if not defined DEVKITPRO (
    echo ========================================
    echo  devkitPro Not Found
    echo ========================================
    echo.
    echo The devkitPro Nintendo Switch toolchain needs to be installed.
    echo.
    echo Please download and install devkitPro from:
    echo   https://github.com/devkitPro/installer/releases
    echo.
    echo Direct link:
    echo   https://github.com/devkitPro/installer/releases/download/v3.0.3/devkitProUpdater-3.0.3.exe
    echo.
    echo IMPORTANT: During installation, select "Switch development"
    echo.
    choice /C YN /M "Open download page in browser"
    if !errorlevel! equ 1 (
        start https://github.com/devkitPro/installer/releases
    )
    echo.
    echo After installing devkitPro, run this script again.
    pause
    exit /b 1
)

echo [OK] DevkitPro: %DEVKITPRO%

REM Set devkitA64
set "DEVKITA64=%DEVKITPRO%\devkitA64"

if not exist "%DEVKITA64%\bin\aarch64-none-elf-gcc.exe" (
    echo [ERROR] devkitA64 not found at %DEVKITA64%
    echo.
    echo Please run the devkitPro updater and select "Switch development"
    echo   https://github.com/devkitPro/installer/releases
    pause
    exit /b 1
)

echo [OK] DevkitA64: %DEVKITA64%

REM Check for libnx
set "LIBNX=%DEVKITPRO%\libnx"
if not exist "%LIBNX%\include\switch.h" (
    echo [ERROR] libnx not found at %LIBNX%
    echo.
    echo Please run the devkitPro updater and select "Switch development"
    pause
    exit /b 1
)

echo [OK] libnx: %LIBNX%
echo.

REM Set PATH
set "PATH=%DEVKITA64%\bin;%DEVKITPRO%\tools\bin;%PATH%"

REM Check for make
where make >nul 2>&1
if !errorlevel! neq 0 (
    if exist "%DEVKITPRO%\msys2\usr\bin\make.exe" (
        set "PATH=%DEVKITPRO%\msys2\usr\bin;%PATH%"
    ) else if defined MSYS_PATH (
        set "PATH=%MSYS_PATH%\usr\bin;%PATH%"
    ) else (
        echo [ERROR] 'make' not found.
        echo Please ensure MSYS2 is installed with devkitPro.
        pause
        exit /b 1
    )
)

:menu
echo Select build option:
echo   1. Build NRO file
echo   2. Clean build
echo   3. Help
echo   4. Exit
echo.
set /p choice="Enter choice (1-4): "

if "%choice%"=="1" goto build
if "%choice%"=="2" goto clean
if "%choice%"=="3" goto help
if "%choice%"=="4" exit /b 0
goto menu

:build
echo.
echo [INFO] Building Nedflix for Nintendo Switch...
echo.

REM Create directories
if not exist "build" mkdir build
if not exist "romfs" mkdir romfs

cd /d "%~dp0"
make

if exist "nedflix.nro" (
    echo.
    echo ============================================
    echo   Build successful!
    echo ============================================
    echo.
    echo Output: nedflix.nro
    for %%A in (nedflix.nro) do echo Size: %%~zA bytes
    echo.
    echo Installation:
    echo   1. Copy nedflix.nro to your Switch SD card
    echo   2. Place in: /switch/nedflix/nedflix.nro
    echo   3. Launch from Homebrew Menu
    echo.
    echo For CFW users (Atmosphere, etc.):
    echo   - Works with Homebrew Menu
    echo   - Requires title override or album applet
) else (
    echo.
    echo [ERROR] Build failed - check output above.
)
echo.
pause
goto menu

:clean
echo.
echo [INFO] Cleaning build directory...
cd /d "%~dp0"

if exist "build" rd /s /q "build"
if exist "nedflix.nro" del "nedflix.nro"
if exist "nedflix.nacp" del "nedflix.nacp"
if exist "nedflix.elf" del "nedflix.elf"

make clean 2>nul

echo Clean complete.
echo.
pause
goto menu

:help
echo.
echo Nedflix Nintendo Switch Build Script
echo.
echo Prerequisites:
echo   - devkitPro with Switch development tools
echo   - Download from: https://github.com/devkitPro/installer/releases
echo   - Select "Switch development" during installation
echo.
echo Features:
echo   - Audio playback (WAV, MP3, OGG, FLAC)
echo   - Video playback (MPEG1)
echo   - Network streaming support
echo   - Touch screen and controller input
echo   - Docked and handheld modes
echo   - Favorites and watch history
echo.
echo Testing:
echo   - Use Yuzu or Ryujinx emulator for testing
echo   - For real hardware, copy .nro to SD card
echo.
pause
goto menu
