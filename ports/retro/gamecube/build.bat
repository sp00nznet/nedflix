@echo off
REM Nedflix GameCube Build Script for Windows
REM One-click build with automatic toolchain installation.

setlocal enabledelayedexpansion

echo ======================================
echo   Nedflix for Nintendo GameCube
echo ======================================
echo.

REM Check if devkitPro Windows installer path exists
set "DEVKITPRO="
if exist "C:\devkitPro" set "DEVKITPRO=C:\devkitPro"
if exist "%USERPROFILE%\devkitPro" set "DEVKITPRO=%USERPROFILE%\devkitPro"
if exist "D:\devkitPro" set "DEVKITPRO=D:\devkitPro"

REM Check for MSYS2 path (legacy installations)
set "MSYS_PATH="
if exist "C:\msys64\msys2_shell.cmd" set "MSYS_PATH=C:\msys64"
if exist "C:\msys32\msys2_shell.cmd" set "MSYS_PATH=C:\msys32"

REM Check devkitPro installation
set "DEVKIT_OK=0"
if defined DEVKITPRO (
    if exist "%DEVKITPRO%\devkitPPC\bin\powerpc-eabi-gcc.exe" set "DEVKIT_OK=1"
)
if defined MSYS_PATH (
    if exist "%MSYS_PATH%\opt\devkitpro\devkitPPC\bin\powerpc-eabi-gcc.exe" (
        set "DEVKIT_OK=1"
        set "DEVKITPRO=%MSYS_PATH%\opt\devkitpro"
    )
)

if "!DEVKIT_OK!"=="0" (
    echo ========================================
    echo  devkitPro Not Found
    echo ========================================
    echo.
    echo The devkitPro GameCube toolchain needs to be installed.
    echo.
    echo Please download and install devkitPro from:
    echo   https://github.com/devkitPro/installer/releases
    echo.
    echo Direct link:
    echo   https://github.com/devkitPro/installer/releases/download/v3.0.3/devkitProUpdater-3.0.3.exe
    echo.
    echo IMPORTANT: During installation, select "GameCube / Wii development"
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

echo Found devkitPro at: %DEVKITPRO%
echo.

:menu
echo Select build option:
echo   1. Build DOL file
echo   2. Clean build
echo   3. Run in Dolphin emulator
echo   4. Help
echo   5. Exit
echo.
set /p choice="Enter choice (1-5): "

if "%choice%"=="1" goto build
if "%choice%"=="2" goto clean
if "%choice%"=="3" goto run
if "%choice%"=="4" goto help
if "%choice%"=="5" exit /b 0
goto menu

:build
echo.
echo Building Nedflix for GameCube...

REM Set up environment and build
set "DEVKITPPC=%DEVKITPRO%\devkitPPC"
set "PATH=%DEVKITPPC%\bin;%DEVKITPRO%\tools\bin;%PATH%"

REM Check for make
where make >nul 2>&1
if !errorlevel! neq 0 (
    REM Try devkitPro's make
    if exist "%DEVKITPRO%\msys2\usr\bin\make.exe" (
        set "PATH=%DEVKITPRO%\msys2\usr\bin;%PATH%"
    ) else if defined MSYS_PATH (
        set "PATH=%MSYS_PATH%\usr\bin;%PATH%"
    ) else (
        echo ERROR: 'make' not found. Please ensure MSYS2 is in PATH.
        pause
        goto menu
    )
)

cd /d "%~dp0"
make

if exist "%~dp0nedflix.dol" (
    echo.
    echo ========================================
    echo   Build successful!
    echo ========================================
    echo.
    echo Output: nedflix.dol
    for %%A in (nedflix.dol) do echo Size: %%~zA bytes
    echo.
    echo To run: Open nedflix.dol in Dolphin emulator
    echo To deploy: Copy nedflix.dol to SD card apps folder
) else (
    echo.
    echo Build may have failed. Check output above.
)
echo.
pause
goto menu

:clean
echo.
echo Cleaning build...
set "DEVKITPPC=%DEVKITPRO%\devkitPPC"
set "PATH=%DEVKITPPC%\bin;%DEVKITPRO%\tools\bin;%PATH%"

where make >nul 2>&1
if !errorlevel! neq 0 (
    if exist "%DEVKITPRO%\msys2\usr\bin\make.exe" (
        set "PATH=%DEVKITPRO%\msys2\usr\bin;%PATH%"
    ) else if defined MSYS_PATH (
        set "PATH=%MSYS_PATH%\usr\bin;%PATH%"
    )
)

cd /d "%~dp0"
make clean
echo Clean complete.
echo.
pause
goto menu

:run
echo.
if not exist "%~dp0nedflix.dol" (
    echo No DOL file found. Building first...
    goto build
)
echo Launching in Dolphin...

REM Try common Dolphin locations
set "DOLPHIN_PATH="
if exist "C:\Program Files\Dolphin\Dolphin.exe" set "DOLPHIN_PATH=C:\Program Files\Dolphin\Dolphin.exe"
if exist "C:\Program Files (x86)\Dolphin\Dolphin.exe" set "DOLPHIN_PATH=C:\Program Files (x86)\Dolphin\Dolphin.exe"
if exist "%LOCALAPPDATA%\Programs\Dolphin\Dolphin.exe" set "DOLPHIN_PATH=%LOCALAPPDATA%\Programs\Dolphin\Dolphin.exe"

where dolphin >nul 2>&1
if !errorlevel! equ 0 set "DOLPHIN_PATH=dolphin"

if defined DOLPHIN_PATH (
    start "" "%DOLPHIN_PATH%" -e "%~dp0nedflix.dol"
) else (
    echo Dolphin not found. Please open nedflix.dol manually.
    echo Download Dolphin from: https://dolphin-emu.org/
    start "" "%~dp0"
)
echo.
pause
goto menu

:help
echo.
echo Nedflix GameCube Build Script
echo.
echo Prerequisites:
echo   - devkitPro with GameCube/Wii development
echo   - Download from: https://github.com/devkitPro/installer/releases
echo.
echo Features:
echo   - Audio playback (WAV, MP3, OGG, FLAC)
echo   - File browser for SD card media
echo   - GX-based UI rendering
echo   - GameCube controller support
echo   - Network via BBA adapter (if available)
echo   - Favorites and watch history
echo.
echo Testing:
echo   - Use Dolphin emulator for testing
echo   - For real hardware, copy nedflix.dol to SD card
echo.
pause
goto menu
