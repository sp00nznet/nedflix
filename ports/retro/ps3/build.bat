@echo off
REM Nedflix PlayStation 3 - TRUE One-Click Windows Build
setlocal enabledelayedexpansion

echo ============================================
echo   Nedflix for PlayStation 3
echo   One-Click Build
echo ============================================
echo.

REM Check for WSL
where wsl >nul 2>&1
if !errorlevel! neq 0 (
    echo [INFO] WSL not found. Installing...
    echo [INFO] This requires administrator privileges and a restart.
    echo.
    powershell -Command "Start-Process cmd -ArgumentList '/c wsl --install' -Verb RunAs -Wait"
    echo.
    echo [INFO] WSL installation started.
    echo [INFO] Please RESTART your computer, then run this script again.
    echo.
    pause
    exit /b 0
)

REM Check if ps3dev is installed in WSL
wsl bash -c "test -f /usr/local/ps3dev/ppu/bin/powerpc64-ps3-elf-gcc" 2>nul
if !errorlevel! neq 0 (
    echo [INFO] PS3 toolchain not found in WSL. Installing...
    echo [INFO] This will take 60-90 minutes. Please be patient.
    echo.

    REM Install dependencies and toolchain
    wsl bash -c "sudo apt-get update && sudo apt-get install -y build-essential git autoconf automake bison flex libelf-dev libtool pkg-config texinfo libgmp-dev libmpfr-dev libmpc-dev zlib1g-dev libssl-dev python3 wget libncurses-dev && cd /tmp && rm -rf ps3toolchain && git clone https://github.com/ps3dev/ps3toolchain.git && cd ps3toolchain && sudo -E ./toolchain.sh"

    if !errorlevel! neq 0 (
        echo [ERROR] Toolchain installation failed.
        pause
        exit /b 1
    )
    echo [INFO] Toolchain installed successfully!
    echo.
)

echo [OK] PS3 toolchain found in WSL

REM Convert Windows path to WSL path and build
set "WIN_PATH=%~dp0"
set "WIN_PATH=!WIN_PATH:\=/!"

echo.
echo [INFO] Building...
wsl bash -c "export PS3DEV=/usr/local/ps3dev && export PSL1GHT=\$PS3DEV && export PATH=\$PS3DEV/bin:\$PS3DEV/ppu/bin:\$PS3DEV/spu/bin:\$PATH && cd \"$(wslpath '%~dp0')\" && make -j$(nproc)"

if exist "%~dp0nedflix.self" (
    echo.
    echo ============================================
    echo   BUILD SUCCESSFUL
    echo ============================================
    echo Output: %~dp0nedflix.self
    for %%A in (nedflix.self) do echo Size: %%~zA bytes
    echo.
    echo To deploy:
    echo   1. Copy nedflix.self to PS3 via FTP
    echo   2. Rename to EBOOT.BIN
    echo   3. Place in /dev_hdd0/game/NEDFLIX01/USRDIR/
    echo   4. Requires CFW or HEN
) else if exist "%~dp0EBOOT.BIN" (
    echo.
    echo ============================================
    echo   BUILD SUCCESSFUL
    echo ============================================
    echo Output: %~dp0EBOOT.BIN
    for %%A in (EBOOT.BIN) do echo Size: %%~zA bytes
) else (
    echo.
    echo [ERROR] Build failed. Check errors above.
)

echo.
pause
