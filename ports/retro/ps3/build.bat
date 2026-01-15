@echo off
REM Nedflix PS3 Build Script for Windows
REM
REM TECHNICAL DEMO / NOVELTY PORT
REM One-click build with automatic toolchain installation.
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
    goto check_toolchain
)

REM Check for MSYS2
set "MSYS_PATH="
if exist "C:\msys64\msys2_shell.cmd" set "MSYS_PATH=C:\msys64"
if exist "C:\msys32\msys2_shell.cmd" set "MSYS_PATH=C:\msys32"

if not "%MSYS_PATH%"=="" (
    set "BUILD_ENV=MSYS2"
    echo Found MSYS2 at: %MSYS_PATH%
    goto check_toolchain
)

REM No build environment - offer to install WSL
echo No build environment found.
echo.
choice /C YN /M "Install WSL (Windows Subsystem for Linux) automatically"
if !errorlevel! equ 2 (
    echo.
    echo Please install WSL manually: wsl --install
    pause
    exit /b 1
)
echo.
echo Installing WSL...
wsl --install
echo.
echo WSL installation started. Please restart your computer,
echo then run this script again.
pause
exit /b 0

:check_toolchain
REM Check if PS3 toolchain is installed
set "TOOLCHAIN_OK=0"
if "%BUILD_ENV%"=="WSL" (
    wsl bash -c "test -f /usr/local/ps3dev/ppu/bin/ppu-gcc" 2>nul && set "TOOLCHAIN_OK=1"
    if "!TOOLCHAIN_OK!"=="0" wsl bash -c "test -f $PS3DEV/ppu/bin/ppu-gcc" 2>nul && set "TOOLCHAIN_OK=1"
) else (
    if exist "%MSYS_PATH%\usr\local\ps3dev\ppu\bin\ppu-gcc.exe" set "TOOLCHAIN_OK=1"
)

if "!TOOLCHAIN_OK!"=="0" (
    echo.
    echo ========================================
    echo  PS3 Toolchain Not Found
    echo ========================================
    echo.
    echo The ps3toolchain needs to be installed.
    echo This will download and compile the toolchain (takes 60-90 minutes^).
    echo.
    choice /C YN /M "Install PS3 toolchain automatically"
    if !errorlevel! equ 2 (
        echo.
        echo Please install ps3toolchain manually.
        pause
        exit /b 1
    )
    echo.
    echo Installing PS3 toolchain...
    echo This will take a while. Please be patient.
    echo.

    if "%BUILD_ENV%"=="WSL" (
        wsl bash -c "sudo apt-get update && sudo apt-get install -y build-essential git autoconf automake bison flex libelf-dev libtool pkg-config texinfo libgmp-dev libmpfr-dev libmpc-dev zlib1g-dev libssl-dev python3 wget && export PS3DEV=/usr/local/ps3dev && export PSL1GHT=$PS3DEV && export PATH=$PS3DEV/bin:$PS3DEV/ppu/bin:$PS3DEV/spu/bin:$PATH && cd ~ && git clone https://github.com/ps3dev/ps3toolchain.git 2>/dev/null || true && cd ~/ps3toolchain && sudo -E ./toolchain.sh"
    ) else (
        "%MSYS_PATH%\usr\bin\bash.exe" -lc "pacman -Syu --noconfirm && pacman -S --noconfirm --needed base-devel git autoconf automake bison flex libelf libtool pkg-config texinfo gmp-devel mpfr-devel mpc-devel zlib-devel openssl-devel python3 wget && export PS3DEV=/usr/local/ps3dev && export PSL1GHT=$PS3DEV && export PATH=$PS3DEV/bin:$PS3DEV/ppu/bin:$PS3DEV/spu/bin:$PATH && cd ~ && git clone https://github.com/ps3dev/ps3toolchain.git 2>/dev/null || true && cd ~/ps3toolchain && ./toolchain.sh"
    )

    if !errorlevel! neq 0 (
        echo.
        echo ERROR: Toolchain installation failed.
        echo Check the output above for errors.
        pause
        exit /b 1
    )
    echo.
    echo Toolchain installed successfully!
    echo.
)

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
    REM Convert Windows path to WSL format and build
    set "SCRIPT_DIR=%~dp0"
    set "SCRIPT_DIR=!SCRIPT_DIR:~0,-1!"
    for /f "usebackq tokens=*" %%i in (`wsl wslpath -u "!SCRIPT_DIR!"`) do set "WSL_DIR=%%i"
    wsl bash -c "cd '!WSL_DIR!' && ./build.sh"
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
    REM Convert Windows path to WSL format and clean
    set "SCRIPT_DIR=%~dp0"
    set "SCRIPT_DIR=!SCRIPT_DIR:~0,-1!"
    for /f "usebackq tokens=*" %%i in (`wsl wslpath -u "!SCRIPT_DIR!"`) do set "WSL_DIR=%%i"
    wsl bash -c "cd '!WSL_DIR!' && ./build.sh clean"
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
