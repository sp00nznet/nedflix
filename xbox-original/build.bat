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
echo  XISO FOR EMULATOR (xemu):
echo  11. Create Client XISO (for xemu)
echo  12. Create Desktop XISO (for xemu)
echo  13. Create Both XISOs
echo.
echo  UTILITIES:
echo   8. Clean Build Output
echo   9. Install/Update nxdk
echo  10. Open Command Prompt with nxdk
echo   0. Exit
echo.

set /p choice="Enter your choice (0-13): "

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
if "%choice%"=="11" goto xiso_client
if "%choice%"=="12" goto xiso_desktop
if "%choice%"=="13" goto xiso_both
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

:xiso_client
echo.
echo Creating Client XISO for xemu...
call :run_nxdk_build "client" "release"
if !errorlevel! neq 0 goto build_done
call :create_xiso "client" "nedflix-client.iso"
goto build_done

:xiso_desktop
echo.
echo Creating Desktop XISO for xemu...
call :ensure_webui
call :run_nxdk_build "desktop" "release"
if !errorlevel! neq 0 goto build_done
call :create_xiso "desktop" "nedflix-desktop.iso"
goto build_done

:xiso_both
echo.
echo Creating both Client and Desktop XISOs...
echo.
echo [1/2] Building Client XISO...
call :run_nxdk_build "client" "release"
call :create_xiso "client" "nedflix-client.iso"
echo.
echo [2/2] Building Desktop XISO...
call :ensure_webui
call :run_nxdk_build "desktop" "release"
call :create_xiso "desktop" "nedflix-desktop.iso"
echo.
echo ========================================
echo Both XISO images created!
echo   - nedflix-client.iso
echo   - nedflix-desktop.iso
echo.
echo Load in xemu: Machine ^> Load Disc...
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
    :: Try robocopy first (available on all modern Windows)
    robocopy "..\desktop\public" "WebUI" /E /NFL /NDL /NJH /NJS /NC /NS >nul 2>nul
    if !errorlevel! leq 7 (
        echo Web UI files copied successfully!
    ) else (
        :: Fallback to copy
        copy /Y "..\desktop\public\*.*" "WebUI\" >nul 2>nul
        echo Web UI files copied.
    )
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

:: Create build directory and set output file
if /i "%BUILD_TYPE%"=="client" (
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

:: Show build mode info (using goto for reliable branching)
if /i "%BUILD_MODE%"=="debug" goto :show_debug
if /i "%BUILD_MODE%"=="release" goto :show_release
if /i "%BUILD_MODE%"=="package" goto :show_package
goto :after_mode_info

:show_debug
echo   - Optimization: Disabled (-O0)
echo   - Debug symbols: Enabled
goto :after_mode_info

:show_release
echo   - Optimization: Full (-O2)
echo   - Debug symbols: Stripped
goto :after_mode_info

:show_package
echo   - Optimization: Full (-O2)
echo   - Creating XBE package...
goto :after_mode_info

:after_mode_info

:: Check if actual source code exists
if exist "src\main.c" (
    :: Real build with nxdk
    echo.
    echo Compiling with nxdk toolchain...

    :: Set CLIENT flag based on build type
    if /i "%BUILD_TYPE%"=="client" (
        set "CLIENT_FLAG=CLIENT=1"
    ) else (
        set "CLIENT_FLAG=CLIENT=0"
    )

    :: Set DEBUG flag if debug mode
    if /i "%BUILD_MODE%"=="debug" (
        set "DEBUG_FLAG=DEBUG=1"
    ) else (
        set "DEBUG_FLAG="
    )

    :: Check for NXDK_DIR
    if not defined NXDK_DIR (
        if exist "%USERPROFILE%\nxdk" (
            set "NXDK_DIR=%USERPROFILE%\nxdk"
        ) else if exist "C:\nxdk" (
            set "NXDK_DIR=C:\nxdk"
        ) else (
            echo ERROR: NXDK_DIR not set. Please install nxdk first ^(option 9^).
            exit /b 1
        )
    )

    echo   NXDK_DIR: !NXDK_DIR!
    echo   Mode: !CLIENT_FLAG!
    echo.

    :: Try to find make - check multiple options
    set "MAKE_CMD="

    :: Option 1: make in PATH
    where make >nul 2>nul
    if !errorlevel! equ 0 set "MAKE_CMD=make"

    :: Option 2: mingw32-make in PATH
    if "!MAKE_CMD!"=="" (
        where mingw32-make >nul 2>nul
        if !errorlevel! equ 0 set "MAKE_CMD=mingw32-make"
    )

    :: Option 3: make in nxdk bin directory
    if "!MAKE_CMD!"=="" (
        if exist "!NXDK_DIR!\bin\make.exe" set "MAKE_CMD=!NXDK_DIR!\bin\make.exe"
    )

    :: Option 4: Use MSYS2 if available
    if "!MAKE_CMD!"=="" (
        set "MSYS2_PATH="
        if exist "C:\msys64" set "MSYS2_PATH=C:\msys64"
        if exist "C:\msys32" if "!MSYS2_PATH!"=="" set "MSYS2_PATH=C:\msys32"

        if not "!MSYS2_PATH!"=="" (
            echo Using MSYS2 for build...

            :: Convert paths for MSYS2
            set "SCRIPT_DIR=%cd%"
            set "SCRIPT_DIR=!SCRIPT_DIR:\=/!"
            set "SCRIPT_DIR=/!SCRIPT_DIR::=!"

            set "NXDK_MSYS=!NXDK_DIR:\=/!"
            set "NXDK_MSYS=/!NXDK_MSYS::=!"

            :: Create build script in MSYS2 tmp to avoid path issues
            set "BUILD_SCRIPT=!MSYS2_PATH!\tmp\xbox_build.sh"
            if not exist "!MSYS2_PATH!\tmp" mkdir "!MSYS2_PATH!\tmp"

            :: Write build script line by line to avoid escaping issues
            echo #!/bin/bash> "!BUILD_SCRIPT!"
            echo.>> "!BUILD_SCRIPT!"
            echo # Always try to install clang - pacman --needed will skip if present>> "!BUILD_SCRIPT!"
            echo echo "Ensuring LLVM/Clang is installed...">> "!BUILD_SCRIPT!"
            echo pacman -S --noconfirm --needed mingw-w64-x86_64-clang mingw-w64-x86_64-lld mingw-w64-x86_64-llvm>> "!BUILD_SCRIPT!"
            echo.>> "!BUILD_SCRIPT!"
            echo export NXDK_DIR="!NXDK_MSYS!">> "!BUILD_SCRIPT!"
            echo export PATH="/mingw64/bin:$NXDK_DIR/bin:$PATH">> "!BUILD_SCRIPT!"
            echo.>> "!BUILD_SCRIPT!"
            echo echo "Verifying clang installation...">> "!BUILD_SCRIPT!"
            echo /mingw64/bin/clang --version>> "!BUILD_SCRIPT!"
            echo.>> "!BUILD_SCRIPT!"
            echo set -e>> "!BUILD_SCRIPT!"
            echo cd "!SCRIPT_DIR!/src">> "!BUILD_SCRIPT!"
            echo echo "Building in: $(pwd)">> "!BUILD_SCRIPT!"
            echo make !CLIENT_FLAG! !DEBUG_FLAG!>> "!BUILD_SCRIPT!"

            echo Running build script...
            call "!MSYS2_PATH!\msys2_shell.cmd" -mingw64 -defterm -no-start -c "bash /tmp/xbox_build.sh"
            set "MAKE_RESULT=!errorlevel!"
            del "!BUILD_SCRIPT!" 2>nul

            goto :check_build_result
        )
    )

    :: If still no make found, error out
    if "!MAKE_CMD!"=="" (
        echo ERROR: 'make' not found!
        echo.
        echo Please install one of the following:
        echo   1. MSYS2: https://www.msys2.org/
        echo      Then: pacman -S make
        echo.
        echo   2. MinGW-w64: Add to PATH
        echo.
        echo   3. Visual Studio Build Tools with C++ workload
        echo.
        exit /b 1
    )

    :: Run make directly
    cd /d "%~dp0\src"
    echo Running: !MAKE_CMD! !CLIENT_FLAG! !DEBUG_FLAG!
    "!MAKE_CMD!" !CLIENT_FLAG! !DEBUG_FLAG!
    set "MAKE_RESULT=!errorlevel!"
    cd /d "%~dp0"

    :check_build_result
    if !MAKE_RESULT! equ 0 (
        :: Check if XBE was created (nxdk outputs to bin/ subdirectory)
        if exist "src\bin\default.xbe" (
            echo.
            echo ========================================
            echo Build completed successfully!
            echo ========================================
            echo.
            echo Output: src\bin\default.xbe
            echo.
            echo To deploy:
            echo   1. FTP the .xbe file to your Xbox
            echo   2. Place in E:\Apps\Nedflix\
            echo   3. Launch from dashboard
            exit /b 0
        ) else if exist "src\default.xbe" (
            echo.
            echo ========================================
            echo Build completed successfully!
            echo ========================================
            echo.
            echo Output: src\default.xbe
            echo.
            echo To deploy:
            echo   1. FTP the .xbe file to your Xbox
            echo   2. Place in E:\Apps\Nedflix\
            echo   3. Launch from dashboard
            exit /b 0
        ) else (
            echo.
            echo Build finished but default.xbe not found.
            echo Check build output above for warnings.
            exit /b 1
        )
    ) else (
        echo.
        echo ========================================
        echo Build failed with error code !MAKE_RESULT!
        echo ========================================
        exit /b !MAKE_RESULT!
    )
) else (
    :: No source code - create placeholder to show what would be built
    echo.
    echo ========================================
    echo   PLACEHOLDER BUILD - No source code
    echo ========================================
    echo.
    echo Source code not found at src\main.c
    echo This build script is ready, but actual Xbox
    echo source code needs to be implemented.
    echo.
    echo Creating placeholder files to demonstrate
    echo the build output structure...
    echo.

    :: Create placeholder XBE info file
    set "LOG_FILE=!BUILD_DIR!\build.log"
    (
        echo Nedflix Original Xbox %BUILD_TYPE% Build
        echo ====================================
        echo Date: %date% %time%
        echo Status: PLACEHOLDER ^(no source code^)
        echo Toolchain: nxdk
        echo Target: Original Xbox ^(2001^)
        echo Architecture: i686 ^(Pentium III^)
        echo.
        if /i "%BUILD_TYPE%"=="client" (
            echo Mode: CLIENT
            echo Features ^(when implemented^):
            echo   - Network streaming from Nedflix server
            echo   - OAuth authentication support
            echo   - Server library browsing
            echo   - Xbox controller navigation
            echo   - 480i/480p/720p/1080i output
            echo   - Dolby Digital 5.1 audio
        ) else (
            echo Mode: DESKTOP
            echo Features ^(when implemented^):
            echo   - Embedded HTTP server ^(port 3000^)
            echo   - Local media playback ^(E:\ F:\ drives^)
            echo   - No authentication required
            echo   - Web UI bundled
            echo   - USB drive support
            echo   - Offline mode ^(no network required^)
        )
        echo.
        echo NOTE: This is a placeholder. To build a real XBE:
        echo   1. Create src\main.c with Xbox application code
        echo   2. Create Makefile.%BUILD_TYPE% for nxdk
        echo   3. Run this build script again
    ) > "!LOG_FILE!" 2>nul

    :: Create a placeholder readme instead of fake binary
    (
        echo This directory would contain: !OUTPUT_FILE!
        echo.
        echo To generate a real XBE binary:
        echo   1. Implement Xbox source code in src\
        echo   2. Create nxdk Makefile
        echo   3. Install nxdk toolchain
        echo   4. Run build.bat again
    ) > "!BUILD_DIR!\README.txt" 2>nul

    echo Placeholder files created in: !BUILD_DIR!\
    echo.
    echo To create a real build:
    echo   1. Add source code to src\main.c
    echo   2. Create Makefile.client and Makefile.desktop
    echo   3. Run this script again
    echo.
)
exit /b 0

:: ========================================
:: Function to create XISO image
:: ========================================
:create_xiso
set "XISO_TYPE=%~1"
set "XISO_OUTPUT=%~2"

echo.
echo ========================================
echo Creating XISO: %XISO_OUTPUT%
echo ========================================
echo.

:: Find the XBE file
set "XBE_PATH="
if exist "src\bin\default.xbe" set "XBE_PATH=src\bin\default.xbe"
if "!XBE_PATH!"=="" if exist "src\default.xbe" set "XBE_PATH=src\default.xbe"

if "!XBE_PATH!"=="" (
    echo ERROR: No XBE file found. Build the project first.
    exit /b 1
)

echo Found XBE: !XBE_PATH!

:: Create ISO staging directory
set "ISO_DIR=iso_staging"
if exist "!ISO_DIR!" rmdir /s /q "!ISO_DIR!"
mkdir "!ISO_DIR!"

:: Copy XBE as default.xbe (required name for Xbox boot)
copy "!XBE_PATH!" "!ISO_DIR!\default.xbe" >nul

:: Create a simple xbe.cfg for title info (optional but nice)
(
    echo [XBE Info]
    echo Title=Nedflix %XISO_TYPE%
    echo Version=1.0.0
) > "!ISO_DIR!\xbe.cfg" 2>nul

echo ISO staging directory created.

:: Check for extract-xiso in MSYS2
set "MSYS2_PATH="
if exist "C:\msys64" set "MSYS2_PATH=C:\msys64"
if exist "C:\msys32" if "!MSYS2_PATH!"=="" set "MSYS2_PATH=C:\msys32"

if "!MSYS2_PATH!"=="" (
    echo ERROR: MSYS2 not found. Cannot create XISO.
    echo Please install MSYS2 from https://www.msys2.org/
    rmdir /s /q "!ISO_DIR!"
    exit /b 1
)

:: Create output directory
if not exist "iso" mkdir "iso"

:: Convert paths for MSYS2
set "ISO_DIR_MSYS=!ISO_DIR:\=/!"
set "OUTPUT_MSYS=iso/!XISO_OUTPUT!"
set "OUTPUT_MSYS=!OUTPUT_MSYS:\=/!"

:: Create XISO build script
set "XISO_SCRIPT=!MSYS2_PATH!\tmp\create_xiso.sh"

echo #!/bin/bash> "!XISO_SCRIPT!"
echo.>> "!XISO_SCRIPT!"
echo # Ensure PATH includes mingw64 binaries>> "!XISO_SCRIPT!"
echo export PATH="/mingw64/bin:$PATH">> "!XISO_SCRIPT!"
echo.>> "!XISO_SCRIPT!"
echo # Always install cdrtools to ensure mkisofs is available>> "!XISO_SCRIPT!"
echo echo "Installing/updating cdrtools ^(provides mkisofs^)...">> "!XISO_SCRIPT!"
echo pacman -S --noconfirm --needed mingw-w64-x86_64-cdrtools>> "!XISO_SCRIPT!"
echo.>> "!XISO_SCRIPT!"
echo # Change to project directory>> "!XISO_SCRIPT!"
echo cd "%cd:\=/%">> "!XISO_SCRIPT!"
echo.>> "!XISO_SCRIPT!"
echo # Create the ISO using mkisofs>> "!XISO_SCRIPT!"
echo echo "Creating ISO image...">> "!XISO_SCRIPT!"
echo echo "  Source: !ISO_DIR_MSYS!">> "!XISO_SCRIPT!"
echo echo "  Output: !OUTPUT_MSYS!">> "!XISO_SCRIPT!"
echo.>> "!XISO_SCRIPT!"
echo # Use explicit path to mkisofs>> "!XISO_SCRIPT!"
echo /mingw64/bin/mkisofs -o "!OUTPUT_MSYS!" -iso-level 4 -R -J "!ISO_DIR_MSYS!">> "!XISO_SCRIPT!"
echo.>> "!XISO_SCRIPT!"
echo if [ -f "!OUTPUT_MSYS!" ]; then>> "!XISO_SCRIPT!"
echo     echo "ISO created successfully!">> "!XISO_SCRIPT!"
echo     ls -la "!OUTPUT_MSYS!">> "!XISO_SCRIPT!"
echo else>> "!XISO_SCRIPT!"
echo     echo "ERROR: ISO file was not created">> "!XISO_SCRIPT!"
echo     exit 1>> "!XISO_SCRIPT!"
echo fi>> "!XISO_SCRIPT!"

echo Creating XISO via MSYS2...
call "!MSYS2_PATH!\msys2_shell.cmd" -mingw64 -defterm -no-start -c "bash /tmp/create_xiso.sh"
set "XISO_RESULT=!errorlevel!"
del "!XISO_SCRIPT!" 2>nul

:: Clean up staging directory
rmdir /s /q "!ISO_DIR!"

:: Check result
if exist "iso\!XISO_OUTPUT!" (
    echo.
    echo ========================================
    echo XISO created successfully!
    echo ========================================
    echo.
    echo Output: iso\!XISO_OUTPUT!
    echo.
    echo To use with xemu:
    echo   1. Open xemu
    echo   2. Go to Machine ^> Load Disc...
    echo   3. Select iso\!XISO_OUTPUT!
    echo   4. The game will boot automatically
    echo.
    exit /b 0
) else (
    echo.
    echo ========================================
    echo XISO creation may have failed.
    echo ========================================
    echo.
    echo Alternative: Load XBE directly in xemu
    echo   1. In xemu, go to Machine ^> Load HDD...
    echo   2. Add !XBE_PATH! to the virtual HDD
    echo.
    exit /b 1
)

:end
echo.
pause
exit /b 0
