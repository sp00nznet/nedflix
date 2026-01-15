@echo off
REM Nedflix GameCube Build Script for Windows
REM
REM TECHNICAL DEMO / NOVELTY PORT
REM One-click build with automatic toolchain installation.
REM

setlocal enabledelayedexpansion

echo ======================================
echo   Nedflix for Nintendo GameCube
echo   TECHNICAL DEMO / NOVELTY PORT
echo ======================================
echo.

REM Find MSYS2
set "MSYS_PATH="
if exist "C:\msys64\msys2_shell.cmd" set "MSYS_PATH=C:\msys64"
if exist "C:\msys32\msys2_shell.cmd" set "MSYS_PATH=C:\msys32"
if exist "%USERPROFILE%\msys64\msys2_shell.cmd" set "MSYS_PATH=%USERPROFILE%\msys64"

if "%MSYS_PATH%"=="" (
    echo ========================================
    echo  MSYS2 Not Found - Installing...
    echo ========================================
    echo.
    echo MSYS2 is required for GameCube development.
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

echo Found MSYS2 at: %MSYS_PATH%

REM Check if devkitPro is installed
set "DEVKIT_OK=0"
if exist "%MSYS_PATH%\opt\devkitpro\devkitPPC\bin\powerpc-eabi-gcc.exe" set "DEVKIT_OK=1"

if "!DEVKIT_OK!"=="0" (
    echo.
    echo ========================================
    echo  devkitPro Not Found
    echo ========================================
    echo.
    echo The devkitPro GameCube toolchain needs to be installed.
    echo This will download and install the toolchain (takes 5-10 minutes^).
    echo.
    choice /C YN /M "Install devkitPro GameCube toolchain automatically"
    if !errorlevel! equ 2 (
        echo.
        echo Please install devkitPro manually.
        pause
        exit /b 1
    )
    echo.
    echo Installing devkitPro GameCube toolchain...
    echo.

    REM Install devkitPro using pacman
    "%MSYS_PATH%\usr\bin\bash.exe" -lc "pacman-key --recv BC26F752D25B92CE272E0F44F7FD5492264BB9D0 --keyserver keyserver.ubuntu.com 2>/dev/null || true && pacman-key --lsign BC26F752D25B92CE272E0F44F7FD5492264BB9D0 2>/dev/null || true && pacman -U --noconfirm https://pkg.devkitpro.org/devkitpro-keyring.pkg.tar.xz 2>/dev/null || true && echo '[dkp-libs]' >> /etc/pacman.conf && echo 'Server = https://pkg.devkitpro.org/packages' >> /etc/pacman.conf && echo '[dkp-windows]' >> /etc/pacman.conf && echo 'Server = https://pkg.devkitpro.org/packages/windows/\$arch' >> /etc/pacman.conf && pacman -Syu --noconfirm && pacman -S --noconfirm gamecube-dev"

    if !errorlevel! neq 0 (
        echo.
        echo ERROR: Toolchain installation failed.
        echo Check the output above for errors.
        pause
        exit /b 1
    )
    echo.
    echo devkitPro installed successfully!
    echo.
)

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
REM Convert path and set up devkitPro environment properly
set "BUILD_DIR=%~dp0"
set "BUILD_DIR=!BUILD_DIR:\=/!"
set "BUILD_DIR=/!BUILD_DIR::=!"
"%MSYS_PATH%\usr\bin\bash.exe" -lc "export DEVKITPRO=/opt/devkitpro && export DEVKITPPC=/opt/devkitpro/devkitPPC && export PATH=$DEVKITPPC/bin:$PATH && cd '!BUILD_DIR!' && make"
if exist "%~dp0nedflix.dol" (
    echo.
    echo Build successful!
    echo Output: nedflix.dol
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
set "BUILD_DIR=%~dp0"
set "BUILD_DIR=!BUILD_DIR:\=/!"
set "BUILD_DIR=/!BUILD_DIR::=!"
"%MSYS_PATH%\usr\bin\bash.exe" -lc "export DEVKITPRO=/opt/devkitpro && export DEVKITPPC=/opt/devkitpro/devkitPPC && cd '!BUILD_DIR!' && make clean"
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
where dolphin-emu >nul 2>&1
if %errorlevel%==0 (
    start dolphin-emu -e "%~dp0nedflix.dol"
) else (
    echo Dolphin not found in PATH.
    echo Please open nedflix.dol manually in Dolphin.
)
echo.
pause
goto menu

:help
echo.
echo Nedflix GameCube Build Script
echo.
echo Prerequisites:
echo   - MSYS2 with devkitPro
echo   - gamecube-dev package
echo.
echo Features (TECHNICAL DEMO):
echo   - Audio playback (WAV files only)
echo   - File browser for SD card media
echo   - GX-based UI rendering
echo   - GameCube controller support
echo.
echo Limitations:
echo   - No video playback
echo   - No network support
echo   - WAV format only
echo.
pause
goto menu
