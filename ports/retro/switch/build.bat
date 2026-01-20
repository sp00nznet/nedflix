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

REM Check for devkitPro
if not defined DEVKITPRO (
    echo [INFO] DEVKITPRO not set, checking default locations...

    if exist "C:\devkitPro" (
        set "DEVKITPRO=C:\devkitPro"
    ) else if exist "D:\devkitPro" (
        set "DEVKITPRO=D:\devkitPro"
    ) else if exist "%USERPROFILE%\devkitPro" (
        set "DEVKITPRO=%USERPROFILE%\devkitPro"
    ) else (
        echo [ERROR] devkitPro not found!
        echo.
        echo Please install devkitPro from: https://devkitpro.org/wiki/Getting_Started
        echo.
        echo After installation, either:
        echo   1. Run this script from the devkitPro MSYS2 terminal, or
        echo   2. Set DEVKITPRO environment variable to your installation path
        echo.
        pause
        exit /b 1
    )
)

echo [OK] DevkitPro: %DEVKITPRO%

REM Set devkitA64
if not defined DEVKITA64 (
    set "DEVKITA64=%DEVKITPRO%\devkitA64"
)

if not exist "%DEVKITA64%" (
    echo [ERROR] devkitA64 not found at %DEVKITA64%
    echo Please install with: pacman -S switch-dev
    pause
    exit /b 1
)

echo [OK] DevkitA64: %DEVKITA64%

REM Check for libnx
set "LIBNX=%DEVKITPRO%\libnx"
if not exist "%LIBNX%" (
    echo [ERROR] libnx not found at %LIBNX%
    echo Please install with: pacman -S libnx
    pause
    exit /b 1
)

echo [OK] libnx: %LIBNX%
echo.

REM Set PATH
set "PATH=%DEVKITA64%\bin;%DEVKITPRO%\tools\bin;%PATH%"

REM Create directories
if not exist "build" mkdir build
if not exist "romfs" mkdir romfs

REM Parse arguments
set CLEAN=0
set VERBOSE=0

:parse_args
if "%~1"=="" goto done_args
if /i "%~1"=="clean" set CLEAN=1
if /i "%~1"=="-v" set VERBOSE=1
if /i "%~1"=="--verbose" set VERBOSE=1
if /i "%~1"=="-h" goto show_help
if /i "%~1"=="--help" goto show_help
shift
goto parse_args

:show_help
echo Usage: build.bat [clean] [-v] [-h]
echo.
echo Options:
echo   clean       Clean build directory before building
echo   -v          Verbose output
echo   -h          Show this help
exit /b 0

:done_args

REM Clean if requested
if %CLEAN%==1 (
    echo [INFO] Cleaning build directory...
    if exist "build" rd /s /q "build"
    if exist "nedflix.nro" del "nedflix.nro"
    if exist "nedflix.nacp" del "nedflix.nacp"
    if exist "nedflix.elf" del "nedflix.elf"
    mkdir build
)

REM Build
echo [INFO] Building...
echo.

if %VERBOSE%==1 (
    make V=1
) else (
    make
)

if errorlevel 1 (
    echo.
    echo [ERROR] Build failed!
    pause
    exit /b 1
)

REM Check output
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
) else (
    echo.
    echo [ERROR] Build failed - no output file!
    pause
    exit /b 1
)

pause
exit /b 0
