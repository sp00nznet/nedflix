@echo off
REM Nedflix PS3 Build Script for Windows
REM
REM TECHNICAL DEMO / NOVELTY PORT
REM
REM Prerequisites:
REM   - PSL1GHT SDK (via WSL or MSYS2)
REM   - ps3toolchain
REM
REM Note: PS3 homebrew development is primarily done on Linux.
REM       This script uses WSL if available, or MSYS2.
REM

setlocal enabledelayedexpansion

echo ======================================
echo   Nedflix for PlayStation 3
echo   TECHNICAL DEMO / NOVELTY PORT
echo ======================================
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
echo Then install PSL1GHT:
echo   git clone https://github.com/ps3dev/ps3toolchain
echo   cd ps3toolchain ^&^& ./toolchain.sh
echo.
pause
exit /b 1

:menu
echo.
echo Select build option:
echo   1. Build SELF/ELF
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
echo Building Nedflix for PS3...
if "%BUILD_ENV%"=="WSL" (
    wsl bash -c "cd '%~dp0' && ./build.sh"
) else (
    "%MSYS_PATH%\usr\bin\bash.exe" -lc "cd '%~dp0' && ./build.sh"
)
if exist "%~dp0nedflix.self" (
    echo.
    echo Build successful!
    echo Output: nedflix.self, nedflix.elf
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
    wsl bash -c "cd '%~dp0' && ./build.sh clean"
) else (
    "%MSYS_PATH%\usr\bin\bash.exe" -lc "cd '%~dp0' && ./build.sh clean"
)
echo Clean complete.
echo.
pause
goto menu

:help
echo.
echo Nedflix PS3 Build Script
echo.
echo Prerequisites:
echo   - WSL or MSYS2
echo   - ps3toolchain
echo   - PSL1GHT SDK
echo.
echo Installation (in WSL/Linux):
echo   git clone https://github.com/ps3dev/ps3toolchain
echo   cd ps3toolchain
echo   sudo ./toolchain.sh
echo.
echo   git clone https://github.com/ps3dev/PSL1GHT
echo   cd PSL1GHT
echo   make install
echo.
echo Environment:
echo   export PS3DEV=/usr/local/ps3dev
echo   export PSL1GHT=$PS3DEV/psl1ght
echo   export PATH=$PS3DEV/bin:$PS3DEV/ppu/bin:$PATH
echo.
echo Deployment:
echo   1. Copy nedflix.self to PS3 via FTP or USB
echo   2. Place in /dev_hdd0/game/NEDF00001/USRDIR/EBOOT.BIN
echo   3. Requires CFW or HEN
echo.
pause
goto menu
