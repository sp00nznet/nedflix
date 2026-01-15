@echo off
REM Nedflix GameCube Build Script for Windows
REM
REM TECHNICAL DEMO / NOVELTY PORT
REM
REM Prerequisites:
REM   - devkitPro with devkitPPC (via MSYS2)
REM   - MSYS2 installed at C:\msys64 or custom path
REM
REM Installation:
REM   1. Install MSYS2: https://www.msys2.org/
REM   2. In MSYS2 terminal: pacman -S dkp-pacman
REM   3. Then: dkp-pacman -S gamecube-dev
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
    echo Error: MSYS2 not found!
    echo.
    echo Please install MSYS2 from https://www.msys2.org/
    echo Then install devkitPro:
    echo   1. Open MSYS2 terminal
    echo   2. Run: pacman -Syu
    echo   3. Run: pacman -S mingw-w64-x86_64-devkitPro-pacman
    echo   4. Run: dkp-pacman -S gamecube-dev
    echo.
    pause
    exit /b 1
)

echo Found MSYS2 at: %MSYS_PATH%
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
"%MSYS_PATH%\usr\bin\bash.exe" -lc "cd '%~dp0' && source /opt/devkitpro/devkitppc/base.sh 2>/dev/null; export DEVKITPRO=/opt/devkitpro; export DEVKITPPC=/opt/devkitpro/devkitPPC; make"
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
"%MSYS_PATH%\usr\bin\bash.exe" -lc "cd '%~dp0' && make clean"
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
