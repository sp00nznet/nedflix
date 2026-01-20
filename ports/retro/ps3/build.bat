@echo off
REM ============================================
REM Nedflix PlayStation 3 - Windows Build Script
REM One-click build using PSL1GHT
REM ============================================

setlocal enabledelayedexpansion

echo.
echo ============================================
echo   Nedflix PlayStation 3 Build (Windows)
echo ============================================
echo.

REM Check for PS3DEV in common locations
set "PS3DEV="
if exist "C:\ps3dev" set "PS3DEV=C:\ps3dev"
if exist "D:\ps3dev" set "PS3DEV=D:\ps3dev"
if exist "%USERPROFILE%\ps3dev" set "PS3DEV=%USERPROFILE%\ps3dev"

REM Check MSYS2 locations
set "MSYS_PATH="
if exist "C:\msys64\usr\local\ps3dev" (
    set "MSYS_PATH=C:\msys64"
    if not defined PS3DEV set "PS3DEV=C:\msys64\usr\local\ps3dev"
)
if exist "C:\msys64\opt\ps3dev" (
    set "MSYS_PATH=C:\msys64"
    if not defined PS3DEV set "PS3DEV=C:\msys64\opt\ps3dev"
)

REM Check WSL
set "USE_WSL=0"
where wsl >nul 2>&1
if !errorlevel! equ 0 (
    wsl bash -c "test -d /usr/local/ps3dev" 2>nul && (
        set "USE_WSL=1"
        echo [INFO] Found PS3 toolchain in WSL
    )
)

if not defined PS3DEV (
    if "!USE_WSL!"=="0" (
        echo ========================================
        echo  PS3 Toolchain Not Found
        echo ========================================
        echo.
        echo The PSL1GHT PS3 toolchain needs to be installed.
        echo.
        echo Options:
        echo   1. Use WSL (recommended for Windows 10/11)
        echo   2. Use MSYS2
        echo.
        echo For WSL installation:
        echo   1. Open PowerShell as Admin and run: wsl --install
        echo   2. After restart, open Ubuntu and run:
        echo      sudo apt update
        echo      sudo apt install build-essential git autoconf automake
        echo      git clone https://github.com/ps3dev/ps3toolchain
        echo      cd ps3toolchain
        echo      sudo ./toolchain.sh
        echo.
        echo For MSYS2:
        echo   1. Install MSYS2 from https://www.msys2.org/
        echo   2. Open MSYS2 and follow similar steps
        echo.
        choice /C YN /M "Open PS3 toolchain GitHub page"
        if !errorlevel! equ 1 (
            start https://github.com/ps3dev/ps3toolchain
        )
        echo.
        pause
        exit /b 1
    )
)

if "!USE_WSL!"=="1" (
    echo [OK] Using WSL with PS3 toolchain
) else (
    echo [OK] PS3DEV: %PS3DEV%
)

REM Check for PSL1GHT
if defined PS3DEV (
    set "PSL1GHT=%PS3DEV%"
    if not exist "%PS3DEV%\ppu\bin\ppu-gcc.exe" (
        if not exist "%PS3DEV%\ppu\bin\powerpc64-ps3-elf-gcc.exe" (
            echo [ERROR] PPU compiler not found in %PS3DEV%
            echo Please ensure ps3toolchain completed successfully.
            pause
            exit /b 1
        )
    )
    echo [OK] PSL1GHT found
)

echo.

:menu
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
echo [INFO] Building Nedflix for PlayStation 3...
echo.

if "!USE_WSL!"=="1" (
    REM Build using WSL
    set "WIN_PATH=%~dp0"
    set "WIN_PATH=!WIN_PATH:\=/!"
    for /f "usebackq tokens=*" %%i in (`wsl wslpath -u "!WIN_PATH!"`) do set "WSL_PATH=%%i"
    wsl bash -c "cd '!WSL_PATH!' && export PS3DEV=/usr/local/ps3dev && export PSL1GHT=$PS3DEV && export PATH=$PS3DEV/bin:$PS3DEV/ppu/bin:$PS3DEV/spu/bin:$PATH && make"
) else (
    REM Build natively with MSYS2
    set "PATH=%PS3DEV%\bin;%PS3DEV%\ppu\bin;%PS3DEV%\spu\bin;%PATH%"

    REM Find make
    where make >nul 2>&1
    if !errorlevel! neq 0 (
        if defined MSYS_PATH (
            set "PATH=%MSYS_PATH%\usr\bin;%PATH%"
        ) else (
            echo [ERROR] 'make' not found. Please install MSYS2.
            pause
            goto menu
        )
    )

    cd /d "%~dp0"
    make
)

if exist "%~dp0nedflix.self" (
    echo.
    echo ============================================
    echo   Build successful!
    echo ============================================
    echo.
    echo Output: nedflix.self
    for %%A in (nedflix.self) do echo Size: %%~zA bytes
    echo.
    echo Installation:
    echo   1. Copy nedflix.self to your PS3 via FTP or USB
    echo   2. Rename to EBOOT.BIN and place in:
    echo      /dev_hdd0/game/NEDFLIX01/USRDIR/EBOOT.BIN
    echo   3. Requires CFW or HEN
    echo.
) else if exist "%~dp0EBOOT.BIN" (
    echo.
    echo ============================================
    echo   Build successful!
    echo ============================================
    echo.
    echo Output: EBOOT.BIN
    for %%A in (EBOOT.BIN) do echo Size: %%~zA bytes
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

if "!USE_WSL!"=="1" (
    set "WIN_PATH=%~dp0"
    set "WIN_PATH=!WIN_PATH:\=/!"
    for /f "usebackq tokens=*" %%i in (`wsl wslpath -u "!WIN_PATH!"`) do set "WSL_PATH=%%i"
    wsl bash -c "cd '!WSL_PATH!' && make clean"
) else (
    cd /d "%~dp0"
    if defined MSYS_PATH set "PATH=%MSYS_PATH%\usr\bin;%PATH%"
    make clean 2>nul
)

if exist "%~dp0nedflix.self" del "%~dp0nedflix.self"
if exist "%~dp0nedflix.elf" del "%~dp0nedflix.elf"
if exist "%~dp0EBOOT.BIN" del "%~dp0EBOOT.BIN"

echo Clean complete.
echo.
pause
goto menu

:help
echo.
echo Nedflix PS3 Build Script
echo.
echo Prerequisites:
echo   - ps3toolchain (PSL1GHT SDK)
echo   - GitHub: https://github.com/ps3dev/ps3toolchain
echo.
echo Recommended Setup (WSL):
echo   1. Install WSL: wsl --install
echo   2. Open Ubuntu terminal
echo   3. sudo apt update
echo   4. sudo apt install build-essential git autoconf
echo   5. git clone https://github.com/ps3dev/ps3toolchain
echo   6. cd ps3toolchain ^&^& sudo ./toolchain.sh
echo.
echo Features:
echo   - Audio playback with multi-format support
echo   - Video streaming
echo   - Network support
echo   - Full UI with controller navigation
echo   - Favorites and watch history
echo.
echo Deployment:
echo   - Requires PS3 with CFW or HEN
echo   - Use FTP or USB to copy files
echo.
pause
goto menu
