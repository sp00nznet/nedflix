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
    echo ERROR: MSYS2 not found. Install from https://www.msys2.org/
    pause
    exit /b 1
)

:: Check toolchain exists
if not exist "%MSYS2_PATH%\opt\toolchains\dc\sh-elf\bin\sh-elf-gcc.exe" (
    echo ERROR: Dreamcast toolchain not installed.
    echo Run install_toolchain.bat first.
    pause
    exit /b 1
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
