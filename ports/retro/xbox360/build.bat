@echo off
REM Nedflix Xbox 360 Build Script for Windows
REM
REM TECHNICAL DEMO / NOVELTY PORT
REM Requires JTAG/RGH modified console for deployment.
REM One-click build with automatic toolchain installation.
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
REM Check if libxenon toolchain is installed
set "TOOLCHAIN_OK=0"
if "%BUILD_ENV%"=="WSL" (
    wsl bash -c "test -f /usr/local/xenon/bin/xenon-gcc" 2>nul && set "TOOLCHAIN_OK=1"
    if "!TOOLCHAIN_OK!"=="0" wsl bash -c "test -f ~/libxenon/toolchain/xenon/bin/xenon-gcc" 2>nul && set "TOOLCHAIN_OK=1"
) else (
    if exist "%MSYS_PATH%\usr\local\xenon\bin\xenon-gcc.exe" set "TOOLCHAIN_OK=1"
)

if "!TOOLCHAIN_OK!"=="0" (
    echo.
    echo ========================================
    echo  Xbox 360 Toolchain Not Found
    echo ========================================
    echo.
    echo The libxenon toolchain needs to be installed.
    echo This will download and compile the toolchain (takes 30-60 minutes^).
    echo.
    choice /C YN /M "Install Xbox 360 toolchain automatically"
    if !errorlevel! equ 2 (
        echo.
        echo Please install libxenon manually.
        pause
        exit /b 1
    )
    echo.
    echo Installing Xbox 360 toolchain...
    echo This will take a while. Please be patient.
    echo.

    if "%BUILD_ENV%"=="WSL" (
        wsl bash -c "sudo apt-get update && sudo apt-get install -y build-essential git libgmp-dev libmpfr-dev libmpc-dev flex bison texinfo libelf-dev wget && cd ~ && if [ ! -d libxenon ]; then git clone --recursive https://github.com/Free60Project/libxenon.git; fi && cd ~/libxenon && if [ -f toolchain/build-toolchain.sh ]; then cd toolchain && ./build-toolchain.sh; elif [ -f build-toolchain.sh ]; then ./build-toolchain.sh; else echo 'Toolchain script not found, building manually...' && sudo mkdir -p /usr/local/xenon && cd /tmp && wget -q https://ftp.gnu.org/gnu/binutils/binutils-2.41.tar.xz && tar xf binutils-2.41.tar.xz && cd binutils-2.41 && ./configure --target=xenon --prefix=/usr/local/xenon --disable-nls --disable-werror && make -j$(nproc) && sudo make install; fi"
    ) else (
        "%MSYS_PATH%\usr\bin\bash.exe" -lc "pacman -Syu --noconfirm && pacman -S --noconfirm --needed base-devel git gmp-devel mpfr-devel mpc-devel flex bison texinfo libelf wget && cd ~ && if [ ! -d libxenon ]; then git clone --recursive https://github.com/Free60Project/libxenon.git; fi && cd ~/libxenon && if [ -f toolchain/build-toolchain.sh ]; then cd toolchain && ./build-toolchain.sh; elif [ -f build-toolchain.sh ]; then ./build-toolchain.sh; else echo 'Toolchain script not found in repo'; fi"
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
    REM Convert Windows path to WSL format and build
    set "SCRIPT_DIR=%~dp0"
    set "SCRIPT_DIR=!SCRIPT_DIR:~0,-1!"
    for /f "usebackq tokens=*" %%i in (`wsl wslpath -u "!SCRIPT_DIR!"`) do set "WSL_DIR=%%i"
    wsl bash -c "cd '!WSL_DIR!' && ./build.sh"
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
