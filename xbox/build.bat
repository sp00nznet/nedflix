@echo off
setlocal enabledelayedexpansion
echo ========================================
echo Nedflix Xbox - Build Script
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

:: Check for .NET SDK
where dotnet >nul 2>nul
if %errorlevel% neq 0 (
    echo .NET SDK is not installed. Installing automatically...
    echo.
    call :install_dotnet_sdk
    if !errorlevel! neq 0 (
        echo ERROR: Failed to install .NET SDK
        echo Please install manually from: https://dotnet.microsoft.com/download/dotnet/8.0
        pause
        exit /b 1
    )

    :: Refresh environment
    call :refresh_path

    :: Check again
    where dotnet >nul 2>nul
    if !errorlevel! neq 0 (
        echo ERROR: .NET SDK was installed but not found in PATH
        echo Please restart your terminal and run this script again.
        pause
        exit /b 1
    )
)

:: Check .NET SDK version
for /f "tokens=1" %%a in ('dotnet --version') do set DOTNET_VERSION=%%a
echo Found .NET SDK %DOTNET_VERSION%
echo.

:: Restore dependencies
echo Restoring NuGet packages...
dotnet restore
if %errorlevel% neq 0 (
    echo ERROR: Failed to restore packages
    pause
    exit /b 1
)
echo.

:: Build menu
echo ========================================
echo Build Options:
echo ========================================
echo.
echo  CLIENT VERSION (connects to server):
echo   1. Build Client (Debug)
echo   2. Build Client (Release)
echo   3. Package Client for Xbox
echo.
echo  DESKTOP VERSION (standalone, no auth):
echo   4. Build Desktop (Debug)
echo   5. Build Desktop (Release)
echo   6. Package Desktop for Xbox
echo.
echo  BOTH VERSIONS:
echo   7. Package Both for Xbox
echo.
echo  UTILITIES:
echo   8. Clean Build Output
echo   9. Copy Web UI (for Desktop mode)
echo  10. Open in Visual Studio
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
if "%choice%"=="9" goto copy_webui
if "%choice%"=="10" goto open_vs
if "%choice%"=="0" exit /b 0

echo Invalid choice. Please run the script again.
pause
exit /b 1

:build_client_debug
echo.
echo Building Client (Debug)...
dotnet build -c Debug -p:Platform=x64
goto build_done

:build_client_release
echo.
echo Building Client (Release)...
dotnet build -c Client -p:Platform=x64
goto build_done

:package_client
echo.
echo Creating Client MSIX Package for Xbox...
dotnet publish -c ClientXbox -p:Platform=x64 -p:AppxPackageSigningEnabled=false
if %errorlevel% equ 0 (
    call :show_install_instructions "Client"
)
goto build_done

:build_desktop_debug
echo.
echo Building Desktop (Debug)...
call :ensure_webui
dotnet build -c Desktop -p:Platform=x64
goto build_done

:build_desktop_release
echo.
echo Building Desktop (Release)...
call :ensure_webui
dotnet build -c Desktop -p:Platform=x64
goto build_done

:package_desktop
echo.
echo Creating Desktop MSIX Package for Xbox...
call :ensure_webui
dotnet publish -c DesktopXbox -p:Platform=x64 -p:AppxPackageSigningEnabled=false
if %errorlevel% equ 0 (
    call :show_install_instructions "Desktop"
)
goto build_done

:package_both
echo.
echo Creating both Client and Desktop packages...
echo.
echo [1/2] Building Client package...
dotnet publish -c ClientXbox -p:Platform=x64 -p:AppxPackageSigningEnabled=false
echo.
echo [2/2] Building Desktop package...
call :ensure_webui
dotnet publish -c DesktopXbox -p:Platform=x64 -p:AppxPackageSigningEnabled=false
echo.
echo ========================================
echo Both packages created!
echo.
echo Client version: Connect to your Nedflix server
echo   - Requires server URL configuration
echo   - Best for remote access
echo.
echo Desktop version: Standalone local playback
echo   - No authentication required
echo   - Access media from Xbox storage
echo ========================================
goto build_done

:clean
echo.
echo Cleaning build output...
dotnet clean
if exist "bin" rmdir /s /q bin
if exist "obj" rmdir /s /q obj
echo Clean complete.
goto end

:copy_webui
echo.
echo Copying Web UI files for Desktop mode...
call :ensure_webui
echo Web UI files copied.
goto end

:open_vs
echo.
echo Opening project in Visual Studio...
if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Community\Common7\IDE\devenv.exe" (
    start "" "%ProgramFiles%\Microsoft Visual Studio\2022\Community\Common7\IDE\devenv.exe" "Nedflix.Xbox.csproj"
) else if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Professional\Common7\IDE\devenv.exe" (
    start "" "%ProgramFiles%\Microsoft Visual Studio\2022\Professional\Common7\IDE\devenv.exe" "Nedflix.Xbox.csproj"
) else if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\devenv.exe" (
    start "" "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\devenv.exe" "Nedflix.Xbox.csproj"
) else (
    echo Visual Studio 2022 not found.
    echo Please open Nedflix.Xbox.csproj manually.
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

:ensure_webui
:: Copy desktop public folder to WebUI if it exists
if not exist "WebUI" mkdir WebUI
if exist "..\desktop\public" (
    echo Copying desktop Web UI files...
    xcopy /s /y /q "..\desktop\public\*" "WebUI\" >nul
) else (
    echo WARNING: Desktop public folder not found at ..\desktop\public
    echo Desktop mode requires the Web UI files.
)
exit /b 0

:show_install_instructions
echo.
echo ========================================
echo %~1 MSIX package created successfully!
echo ========================================
echo.
echo To install on Xbox:
echo   1. Enable Dev Mode on your Xbox
echo   2. Note your Xbox's IP address from Dev Home
echo   3. Open Device Portal: https://[xbox-ip]:11443
echo   4. Go to "Apps" section
echo   5. Click "Add" under "Install app"
echo   6. Select the .msix file from:
echo      bin\%~1Xbox\net6.0-windows10.0.19041.0\win10-x64\publish\
echo   7. Click "Install"
echo.
exit /b 0

:end
echo.
pause
exit /b 0

:: ========================================
:: Function to refresh PATH environment
:: ========================================
:refresh_path
for /f "tokens=2*" %%a in ('reg query "HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Environment" /v Path 2^>nul') do set "SYS_PATH=%%b"
for /f "tokens=2*" %%a in ('reg query "HKCU\Environment" /v Path 2^>nul') do set "USER_PATH=%%b"
set "PATH=%SYS_PATH%;%USER_PATH%"
exit /b 0

:: ========================================
:: Function to install .NET SDK
:: ========================================
:install_dotnet_sdk
echo ----------------------------------------
echo Installing .NET SDK 8.0...
echo ----------------------------------------
echo.

:: Try winget first (Windows 11 / Windows 10 with App Installer)
where winget >nul 2>nul
if %errorlevel% equ 0 (
    echo Installing via winget...
    winget install Microsoft.DotNet.SDK.8 --silent --accept-package-agreements --accept-source-agreements
    if !errorlevel! equ 0 (
        echo .NET SDK installed successfully via winget!
        goto :install_dotnet_done
    )
    echo Winget install failed, trying alternative method...
    echo.
)

:: Try Chocolatey
where choco >nul 2>nul
if %errorlevel% equ 0 (
    echo Installing via Chocolatey...
    choco install dotnet-sdk -y
    if !errorlevel! equ 0 (
        echo .NET SDK installed successfully via Chocolatey!
        goto :install_dotnet_done
    )
    echo Chocolatey install failed, trying manual download...
    echo.
)

:: Manual download and install
echo Downloading .NET SDK installer...
set "DOTNET_URL=https://download.visualstudio.microsoft.com/download/pr/69f3e301-5243-4c5f-8b8e-e98e5d59ef3a/74e7b8d48b6a7bb60ec6eb0e5c6c8f0e/dotnet-sdk-8.0.101-win-x64.exe"
set "DOTNET_INSTALLER=%TEMP%\dotnet-sdk-installer.exe"

echo Downloading from Microsoft...
powershell -Command "& {[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri '%DOTNET_URL%' -OutFile '%DOTNET_INSTALLER%' -UseBasicParsing}"

if not exist "%DOTNET_INSTALLER%" (
    echo ERROR: Failed to download .NET SDK installer
    exit /b 1
)

echo.
echo Installing .NET SDK (this may take several minutes)...
echo You may see a UAC prompt - click Yes to continue.
echo.

:: Try silent install first
"%DOTNET_INSTALLER%" /install /quiet /norestart
if !errorlevel! neq 0 (
    echo Silent install failed. Trying interactive install...
    "%DOTNET_INSTALLER%"
)

:: Clean up
del "%DOTNET_INSTALLER%" 2>nul

:install_dotnet_done
echo.
echo ----------------------------------------
echo .NET SDK installation complete!
echo ----------------------------------------
exit /b 0
