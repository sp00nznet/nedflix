@echo off
REM Nedflix GameCube - TRUE One-Click Windows Build
setlocal enabledelayedexpansion

echo ======================================
echo   Nedflix for Nintendo GameCube
echo   One-Click Build
echo ======================================
echo.

REM Find devkitPro
set "DEVKITPRO="
if exist "C:\devkitPro\devkitPPC" set "DEVKITPRO=C:\devkitPro"
if exist "D:\devkitPro\devkitPPC" set "DEVKITPRO=D:\devkitPro"
if exist "%USERPROFILE%\devkitPro\devkitPPC" set "DEVKITPRO=%USERPROFILE%\devkitPro"
if exist "C:\msys64\opt\devkitpro\devkitPPC" set "DEVKITPRO=C:\msys64\opt\devkitpro"

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

    REM Run installer silently with GameCube/Wii selected
    echo [2/3] Installing devkitPro (this takes 5-10 minutes)...
    "%TEMP%\devkitpro.exe" /S /D=C:\devkitPro

    REM Wait for installation
    echo [INFO] Waiting for installation to complete...
    :wait_install
    if not exist "C:\devkitPro\devkitPPC\bin\powerpc-eabi-gcc.exe" (
        timeout /t 5 /nobreak >nul
        goto wait_install
    )

    set "DEVKITPRO=C:\devkitPro"
    echo [3/3] Installation complete!
    echo.
)

echo [OK] devkitPro: %DEVKITPRO%

REM Set environment
set "DEVKITPPC=%DEVKITPRO%\devkitPPC"
set "PATH=%DEVKITPPC%\bin;%DEVKITPRO%\tools\bin;%DEVKITPRO%\msys2\usr\bin;%PATH%"

REM Build
echo.
echo [INFO] Building...
cd /d "%~dp0"
make -j%NUMBER_OF_PROCESSORS%

if exist "%~dp0nedflix.dol" (
    echo.
    echo ======================================
    echo   BUILD SUCCESSFUL
    echo ======================================
    echo Output: %~dp0nedflix.dol
    for %%A in (nedflix.dol) do echo Size: %%~zA bytes
    echo.
    echo To test: Open in Dolphin emulator
    echo To deploy: Copy to SD card /apps/nedflix/
) else (
    echo.
    echo [ERROR] Build failed. Check errors above.
)

echo.
pause
