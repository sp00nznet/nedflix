@echo off
REM Nedflix Nintendo Switch - TRUE One-Click Windows Build
setlocal enabledelayedexpansion

echo ============================================
echo   Nedflix for Nintendo Switch
echo   One-Click Build
echo ============================================
echo.

REM Find devkitPro
set "DEVKITPRO="
if exist "C:\devkitPro\devkitA64" set "DEVKITPRO=C:\devkitPro"
if exist "D:\devkitPro\devkitA64" set "DEVKITPRO=D:\devkitPro"
if exist "%USERPROFILE%\devkitPro\devkitA64" set "DEVKITPRO=%USERPROFILE%\devkitPro"
if exist "C:\msys64\opt\devkitpro\devkitA64" set "DEVKITPRO=C:\msys64\opt\devkitpro"

if not defined DEVKITPRO (
    echo [INFO] devkitPro not found. Installing automatically...
    echo.

    REM Download installer
    echo [1/3] Downloading devkitPro installer...
    powershell -Command "& {[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri 'https://github.com/devkitPro/installer/releases/download/v3.0.3/devkitProUpdater-3.0.3.exe' -OutFile '%TEMP%\devkitpro.exe'}" 2>nul
    if not exist "%TEMP%\devkitpro.exe" (
        echo [ERROR] Download failed. Please check your internet connection.
        pause
        exit /b 1
    )

    REM Run installer silently
    echo [2/3] Installing devkitPro (this takes 5-10 minutes)...
    "%TEMP%\devkitpro.exe" /S /D=C:\devkitPro

    REM Wait for installation
    echo [INFO] Waiting for installation to complete...
    :wait_install
    if not exist "C:\devkitPro\devkitA64\bin\aarch64-none-elf-gcc.exe" (
        timeout /t 5 /nobreak >nul
        goto wait_install
    )

    set "DEVKITPRO=C:\devkitPro"
    echo [3/3] Installation complete!
    echo.
)

echo [OK] devkitPro: %DEVKITPRO%

REM Set environment
set "DEVKITA64=%DEVKITPRO%\devkitA64"
set "PATH=%DEVKITA64%\bin;%DEVKITPRO%\tools\bin;%DEVKITPRO%\msys2\usr\bin;%PATH%"

REM Create directories
if not exist "build" mkdir build
if not exist "romfs" mkdir romfs

REM Build
echo.
echo [INFO] Building...
cd /d "%~dp0"
make -j%NUMBER_OF_PROCESSORS%

if exist "%~dp0nedflix.nro" (
    echo.
    echo ============================================
    echo   BUILD SUCCESSFUL
    echo ============================================
    echo Output: %~dp0nedflix.nro
    for %%A in (nedflix.nro) do echo Size: %%~zA bytes
    echo.
    echo To deploy: Copy to SD card /switch/nedflix/
    echo To test: Use Yuzu or Ryujinx emulator
) else (
    echo.
    echo [ERROR] Build failed. Check errors above.
)

echo.
pause
