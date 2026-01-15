@echo off
REM Nedflix Xbox 360 Build Script for Windows
REM
REM TECHNICAL DEMO / NOVELTY PORT
REM Requires JTAG/RGH modified console for deployment.
REM
REM Prerequisites:
REM   - libxenon toolchain (via WSL or MSYS2)
REM   - Free60 SDK
REM
REM Note: Xbox 360 homebrew development is primarily done on Linux.
REM       This script uses WSL if available, or MSYS2.
REM

setlocal enabledelayedexpansion

echo ======================================
echo   Nedflix for Xbox 360
echo   TECHNICAL DEMO / NOVELTY PORT
echo ======================================
echo.
echo WARNING: Requires JTAG/RGH modified console!
echo.

REM Check for WSL first (preferred)
where wsl >nul 2>&1
if %errorlevel%==0 (
    set "BUILD_ENV=WSL"
    echo Found WSL - using Linux build environment
    goto menu
)

REM Check for MSYS2
set "MSYS_PATH="
if exist "C:\msys64\msys2_shell.cmd" set "MSYS_PATH=C:\msys64"
if exist "C:\msys32\msys2_shell.cmd" set "MSYS_PATH=C:\msys32"

if not "%MSYS_PATH%"=="" (
    set "BUILD_ENV=MSYS2"
    echo Found MSYS2 at: %MSYS_PATH%
    goto menu
)

echo Error: No suitable build environment found!
echo.
echo Please install one of:
echo   1. WSL (Windows Subsystem for Linux) - Recommended
echo      wsl --install
echo.
echo   2. MSYS2
echo      https://www.msys2.org/
echo.
echo Then install libxenon:
echo   git clone https://github.com/Free60Project/libxenon
echo   cd libxenon/toolchain ^&^& ./build-toolchain.sh
echo.
pause
exit /b 1

:menu
echo.
echo Select build option:
echo   1. Build ELF
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
echo Building Nedflix for Xbox 360...
if "%BUILD_ENV%"=="WSL" (
    REM Use wslpath to convert Windows path to WSL format
    wsl bash -c "cd \"$(wslpath '%~dp0')\" && ./build.sh"
) else (
    "%MSYS_PATH%\usr\bin\bash.exe" -lc "cd '%~dp0' && ./build.sh"
)
if exist "%~dp0nedflix.elf32" (
    echo.
    echo Build successful!
    echo Output: nedflix.elf, nedflix.elf32
) else if exist "%~dp0nedflix.elf" (
    echo.
    echo Build successful!
    echo Output: nedflix.elf
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
if "%BUILD_ENV%"=="WSL" (
    REM Use wslpath to convert Windows path to WSL format
    wsl bash -c "cd \"$(wslpath '%~dp0')\" && ./build.sh clean"
) else (
    "%MSYS_PATH%\usr\bin\bash.exe" -lc "cd '%~dp0' && ./build.sh clean"
)
echo Clean complete.
echo.
pause
goto menu

:help
echo.
echo Nedflix Xbox 360 Build Script
echo.
echo WARNING: Running homebrew on Xbox 360 requires
echo hardware modification (JTAG or RGH).
echo.
echo Prerequisites:
echo   - WSL or MSYS2
echo   - libxenon from Free60 project
echo.
echo Installation (in WSL/Linux):
echo   git clone https://github.com/Free60Project/libxenon
echo   cd libxenon/toolchain
echo   ./build-toolchain.sh
echo.
echo Environment:
echo   export DEVKITXENON=/path/to/libxenon
echo   export PATH=$DEVKITXENON/bin:$PATH
echo.
echo Deployment:
echo   1. Copy nedflix.elf32 to USB drive
echo   2. Boot JTAG/RGH Xbox 360
echo   3. Launch via XeLL or compatible dashboard
echo.
pause
goto menu
