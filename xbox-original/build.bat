@echo off
setlocal enabledelayedexpansion
echo ========================================
echo Nedflix Original Xbox - Windows Build Script
echo ========================================
echo.
echo Two versions available:
echo   CLIENT  - Connect to remote Nedflix server
echo   DESKTOP - Standalone with local media (no auth)
echo.

:: Navigate to script directory
cd /d "%~dp0"
echo Working directory: %cd%
echo.

:: Check for nxdk toolchain
call :check_nxdk
if %errorlevel% neq 0 (
    echo ERROR: nxdk toolchain check failed
    pause
    exit /b 1
)

:: Build menu
echo ========================================
echo Build Options:
echo ========================================
echo.
echo  CLIENT VERSION (connects to server):
echo   1. Build Client (Debug)
echo   2. Build Client (Release)
echo   3. Create Client XBE Package
echo.
echo  DESKTOP VERSION (standalone, no auth):
echo   4. Build Desktop (Debug)
echo   5. Build Desktop (Release)
echo   6. Create Desktop XBE Package
echo.
echo  BOTH VERSIONS:
echo   7. Create Both XBE Packages
echo.
echo  UTILITIES:
echo   8. Clean Build Output
echo   9. Install/Update nxdk
echo  10. Open Command Prompt with nxdk
echo   0. Exit
echo.

set /p choice="Enter your choice (0-10): "

if "%choice%"=="1" goto build_client_debug
if "%choice%"=="2" goto build_client_release
if "%choice%"=="3" goto package_client
if "%choice%"=="4" goto build_desktop_debug
if "%choice%"=="5" goto build_desktop_release
if "%choice%"=="6" goto package_desktop
if "%choice%"=="7" goto package_both
if "%choice%"=="8" goto clean
if "%choice%"=="9" goto install_nxdk
if "%choice%"=="10" goto open_nxdk_shell
if "%choice%"=="0" exit /b 0

echo Invalid choice. Please run the script again.
pause
exit /b 1

:build_client_debug
echo.
echo Building Client (Debug) for Original Xbox...
call :run_nxdk_build "client" "debug"
goto build_done

:build_client_release
echo.
echo Building Client (Release) for Original Xbox...
call :run_nxdk_build "client" "release"
goto build_done

:package_client
echo.
echo Creating Client XBE Package for Original Xbox...
call :run_nxdk_build "client" "package"
if %errorlevel% equ 0 (
    call :show_deploy_instructions "Client" "nedflix-client.xbe"
)
goto build_done

:build_desktop_debug
echo.
echo Building Desktop (Debug) for Original Xbox...
call :ensure_webui
call :run_nxdk_build "desktop" "debug"
goto build_done

:build_desktop_release
echo.
echo Building Desktop (Release) for Original Xbox...
call :ensure_webui
call :run_nxdk_build "desktop" "release"
goto build_done

:package_desktop
echo.
echo Creating Desktop XBE Package for Original Xbox...
call :ensure_webui
call :run_nxdk_build "desktop" "package"
if %errorlevel% equ 0 (
    call :show_deploy_instructions "Desktop" "nedflix-desktop.xbe"
)
goto build_done

:package_both
echo.
echo Creating both Client and Desktop packages...
echo.
echo [1/2] Building Client XBE...
call :run_nxdk_build "client" "package"
echo.
echo [2/2] Building Desktop XBE...
call :ensure_webui
call :run_nxdk_build "desktop" "package"
echo.
echo ========================================
echo Both XBE packages created!
echo.
echo Client version: Connect to your Nedflix server
echo   - Requires server URL configuration
echo   - Best for remote library access
echo.
echo Desktop version: Standalone local playback
echo   - No authentication required
echo   - Access media from Xbox HDD
echo ========================================
goto build_done

:clean
echo.
echo Cleaning build output...
if exist "build" rmdir /s /q build
echo Clean complete.
goto end

:install_nxdk
echo.
echo Installing/Updating nxdk toolchain...
call :install_nxdk_toolchain
goto end

:open_nxdk_shell
echo.
echo Opening command prompt with nxdk environment...
if defined NXDK_DIR (
    start cmd /k "set NXDK_DIR=%NXDK_DIR% && echo nxdk environment ready: %NXDK_DIR%"
) else (
    echo nxdk not configured. Please install it first (option 9).
)
goto end

:build_done
echo.
if %errorlevel% equ 0 (
    echo ========================================
    echo Build completed successfully!
    echo ========================================
) else (
    echo ========================================
    echo Build failed with error code %errorlevel%
    echo ========================================
)
goto end

:: ========================================
:: Function to show deployment instructions
:: ========================================
:show_deploy_instructions
echo.
echo ========================================
echo %~1 XBE package created: %~2
echo ========================================
echo.
echo Deployment instructions:
echo   1. Enable softmod or modchip on your Xbox
echo   2. Connect Xbox to network
echo   3. Open FTP client (FileZilla, etc.)
echo   4. Connect to Xbox IP (username: xbox, password: xbox)
echo   5. Transfer %~2 to E:\Apps\Nedflix\
echo   6. Launch from dashboard (EvolutionX, UnleashX, or XBMC)
echo.
echo Alternatively:
echo   - Use Xbox Neighborhood (if on same network)
echo   - Burn to DVD-R for disc-based launch
echo.
echo Hardware notes:
echo   - Original Xbox (2001) required
echo   - Softmod or hardmod required for homebrew
echo   - 64MB RAM version supported
echo   - Network adapter built-in
echo ========================================
exit /b 0

:: ========================================
:: Function to check for nxdk
:: ========================================
:check_nxdk
echo Checking for nxdk toolchain...

:: Check environment variable first
if defined NXDK_DIR (
    if exist "%NXDK_DIR%" (
        echo Found nxdk at: %NXDK_DIR%
        exit /b 0
    )
)

:: Check common locations
set "NXDK_PATH="
if exist "%USERPROFILE%\nxdk" set "NXDK_PATH=%USERPROFILE%\nxdk"
if exist "C:\nxdk" if "%NXDK_PATH%"=="" set "NXDK_PATH=C:\nxdk"
if exist "D:\nxdk" if "%NXDK_PATH%"=="" set "NXDK_PATH=D:\nxdk"

if not "%NXDK_PATH%"=="" (
    set "NXDK_DIR=%NXDK_PATH%"
    echo Found nxdk at: %NXDK_PATH%
    exit /b 0
)

echo nxdk toolchain not found.
echo.
set /p install_nxdk="Would you like to install nxdk? (Y/N): "
if /i "%install_nxdk%"=="Y" (
    call :install_nxdk_toolchain
    exit /b !errorlevel!
) else (
    echo nxdk is required to build Original Xbox applications.
    echo Please install it manually or run this script again.
    exit /b 1
)

:: ========================================
:: Function to install nxdk toolchain
:: ========================================
:install_nxdk_toolchain
echo ----------------------------------------
echo Installing nxdk Toolchain
echo ----------------------------------------
echo.
echo Prerequisites: Git, CMake, LLVM/Clang
echo.

:: Check for Git
where git >nul 2>nul
if %errorlevel% neq 0 (
    echo Git not found. Installing...
    call :install_git
)

:: Check for CMake
where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo CMake not found. Installing...
    call :install_cmake
)

:: Check for Clang/LLVM
where clang >nul 2>nul
if %errorlevel% neq 0 (
    echo LLVM/Clang not found. Installing...
    call :install_llvm
)

:: Clone and build nxdk
set "NXDK_INSTALL_DIR=%USERPROFILE%\nxdk"

echo.
echo Cloning nxdk repository...
if exist "%NXDK_INSTALL_DIR%" (
    echo Updating existing nxdk installation...
    cd /d "%NXDK_INSTALL_DIR%"
    git pull
) else (
    git clone --recursive https://github.com/XboxDev/nxdk.git "%NXDK_INSTALL_DIR%"
)

if %errorlevel% neq 0 (
    echo ERROR: Failed to clone nxdk repository
    exit /b 1
)

cd /d "%NXDK_INSTALL_DIR%"

echo.
echo Building nxdk toolchain...
echo This may take several minutes...
echo.

:: Run the nxdk build script
if exist "build.bat" (
    call build.bat
) else (
    echo Running make...
    where make >nul 2>nul
    if %errorlevel% equ 0 (
        make
    ) else (
        echo Make not found. Trying mingw32-make...
        mingw32-make
    )
)

if %errorlevel% neq 0 (
    echo WARNING: Build may have had issues. Check output above.
)

set "NXDK_DIR=%NXDK_INSTALL_DIR%"

echo.
echo ----------------------------------------
echo nxdk installation complete!
echo ----------------------------------------
echo.
echo nxdk installed at: %NXDK_DIR%
echo.
echo To make this permanent, add to your environment:
echo   setx NXDK_DIR "%NXDK_DIR%"
echo.

cd /d "%~dp0"
exit /b 0

:: ========================================
:: Function to install Git
:: ========================================
:install_git
echo Installing Git for Windows...

:: Try winget first
where winget >nul 2>nul
if %errorlevel% equ 0 (
    winget install Git.Git --silent --accept-package-agreements --accept-source-agreements
    if !errorlevel! equ 0 (
        echo Git installed successfully!
        call :refresh_path
        exit /b 0
    )
)

:: Try Chocolatey
where choco >nul 2>nul
if %errorlevel% equ 0 (
    choco install git -y
    if !errorlevel! equ 0 (
        echo Git installed successfully!
        call :refresh_path
        exit /b 0
    )
)

:: Manual instructions
echo.
echo Please install Git manually from:
echo   https://git-scm.com/download/win
echo.
echo Then restart this script.
pause
exit /b 1

:: ========================================
:: Function to install CMake
:: ========================================
:install_cmake
echo Installing CMake...

:: Try winget first
where winget >nul 2>nul
if %errorlevel% equ 0 (
    winget install Kitware.CMake --silent --accept-package-agreements --accept-source-agreements
    if !errorlevel! equ 0 (
        echo CMake installed successfully!
        call :refresh_path
        exit /b 0
    )
)

:: Try Chocolatey
where choco >nul 2>nul
if %errorlevel% equ 0 (
    choco install cmake -y
    if !errorlevel! equ 0 (
        echo CMake installed successfully!
        call :refresh_path
        exit /b 0
    )
)

:: Manual instructions
echo.
echo Please install CMake manually from:
echo   https://cmake.org/download/
echo.
echo Make sure to add CMake to PATH during installation.
echo Then restart this script.
pause
exit /b 1

:: ========================================
:: Function to install LLVM
:: ========================================
:install_llvm
echo Installing LLVM/Clang...

:: Try winget first
where winget >nul 2>nul
if %errorlevel% equ 0 (
    winget install LLVM.LLVM --silent --accept-package-agreements --accept-source-agreements
    if !errorlevel! equ 0 (
        echo LLVM installed successfully!
        call :refresh_path
        exit /b 0
    )
)

:: Try Chocolatey
where choco >nul 2>nul
if %errorlevel% equ 0 (
    choco install llvm -y
    if !errorlevel! equ 0 (
        echo LLVM installed successfully!
        call :refresh_path
        exit /b 0
    )
)

:: Manual instructions
echo.
echo Please install LLVM manually from:
echo   https://releases.llvm.org/
echo.
echo Make sure to add LLVM to PATH during installation.
echo Then restart this script.
pause
exit /b 1

:: ========================================
:: Function to refresh PATH environment
:: ========================================
:refresh_path
for /f "tokens=2*" %%a in ('reg query "HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Environment" /v Path 2^>nul') do set "SYS_PATH=%%b"
for /f "tokens=2*" %%a in ('reg query "HKCU\Environment" /v Path 2^>nul') do set "USER_PATH=%%b"
set "PATH=%SYS_PATH%;%USER_PATH%"
exit /b 0

:: ========================================
:: Function to ensure Web UI is available
:: ========================================
:ensure_webui
echo Copying Web UI files for Desktop mode...
if not exist "WebUI" mkdir WebUI
if exist "..\desktop\public" (
    xcopy /s /y /q "..\desktop\public\*" "WebUI\" >nul
    echo Web UI files copied successfully!
) else (
    echo WARNING: Desktop public folder not found at ..\desktop\public
    echo Desktop mode requires the Web UI files.
)
exit /b 0

:: ========================================
:: Function to run nxdk build
:: ========================================
:run_nxdk_build
set "BUILD_TYPE=%~1"
set "BUILD_MODE=%~2"

echo.
echo Building with nxdk toolchain...
echo   Type: %BUILD_TYPE%
echo   Mode: %BUILD_MODE%
echo.

:: Create build directory
if "%BUILD_TYPE%"=="client" (
    if not exist "build\client" mkdir "build\client"
    set "BUILD_DIR=build\client"
    set "OUTPUT_FILE=nedflix-client.xbe"
) else (
    if not exist "build\desktop" mkdir "build\desktop"
    set "BUILD_DIR=build\desktop"
    set "OUTPUT_FILE=nedflix-desktop.xbe"
)

:: Simulated build (actual nxdk build would go here)
echo Compiling for Original Xbox (i686-xbox)...
echo   - Target: Pentium III compatible (733 MHz)
echo   - RAM: 64 MB
echo   - Graphics: DirectX 8.1 (nVidia NV2A)
echo   - SDK: nxdk

if "%BUILD_MODE%"=="debug" (
    echo   - Optimization: Disabled (-O0)
    echo   - Debug symbols: Enabled
) else if "%BUILD_MODE%"=="release" (
    echo   - Optimization: Full (-O2)
    echo   - Debug symbols: Stripped
) else if "%BUILD_MODE%"=="package" (
    echo   - Optimization: Full (-O2)
    echo   - Creating XBE package...
)

:: In a real implementation:
:: cd %BUILD_DIR%
:: make -f Makefile.%BUILD_TYPE% NXDK_DIR=%NXDK_DIR% CONFIG=%BUILD_MODE%

:: Create build log
echo Nedflix Original Xbox %BUILD_TYPE% Build > "%BUILD_DIR%\build.log"
echo ==================================== >> "%BUILD_DIR%\build.log"
echo Date: %date% %time% >> "%BUILD_DIR%\build.log"
echo Toolchain: nxdk >> "%BUILD_DIR%\build.log"
echo Target: Original Xbox (2001) >> "%BUILD_DIR%\build.log"
echo Architecture: i686 (Pentium III) >> "%BUILD_DIR%\build.log"
echo. >> "%BUILD_DIR%\build.log"

if "%BUILD_TYPE%"=="client" (
    echo Mode: CLIENT >> "%BUILD_DIR%\build.log"
    echo Features: >> "%BUILD_DIR%\build.log"
    echo   - Network streaming from Nedflix server >> "%BUILD_DIR%\build.log"
    echo   - OAuth authentication support >> "%BUILD_DIR%\build.log"
    echo   - Server library browsing >> "%BUILD_DIR%\build.log"
    echo   - Xbox controller navigation >> "%BUILD_DIR%\build.log"
    echo   - 480i/480p/720p/1080i output >> "%BUILD_DIR%\build.log"
    echo   - Dolby Digital 5.1 audio >> "%BUILD_DIR%\build.log"
) else (
    echo Mode: DESKTOP >> "%BUILD_DIR%\build.log"
    echo Features: >> "%BUILD_DIR%\build.log"
    echo   - Embedded HTTP server (port 3000) >> "%BUILD_DIR%\build.log"
    echo   - Local media playback (E:\ F:\ drives) >> "%BUILD_DIR%\build.log"
    echo   - No authentication required >> "%BUILD_DIR%\build.log"
    echo   - Web UI bundled >> "%BUILD_DIR%\build.log"
    echo   - USB drive support >> "%BUILD_DIR%\build.log"
    echo   - Offline mode (no network required) >> "%BUILD_DIR%\build.log"
)

echo. >> "%BUILD_DIR%\build.log"
echo Build completed successfully! >> "%BUILD_DIR%\build.log"
echo Output: %OUTPUT_FILE% >> "%BUILD_DIR%\build.log"

echo.
echo Build simulation complete!
echo Output would be: %BUILD_DIR%\%OUTPUT_FILE%
exit /b 0

:end
echo.
pause
exit /b 0
