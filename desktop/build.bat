@echo off
setlocal enabledelayedexpansion
echo ========================================
echo Nedflix Desktop - Windows Build Script
echo ========================================
echo.

:: Navigate to script directory
cd /d "%~dp0"
echo Working directory: %cd%
echo.

:: Check if Node.js is installed
where node >nul 2>nul
if %errorlevel% neq 0 (
    echo Node.js is not installed. Installing automatically...
    echo.
    call :install_nodejs
    if !errorlevel! neq 0 (
        echo ERROR: Failed to install Node.js
        pause
        exit /b 1
    )
    :: Refresh PATH
    call refreshenv >nul 2>nul
    :: If refreshenv not available, try to find node in common locations
    where node >nul 2>nul
    if !errorlevel! neq 0 (
        if exist "%ProgramFiles%\nodejs\node.exe" (
            set "PATH=%ProgramFiles%\nodejs;%PATH%"
        ) else if exist "%ProgramFiles(x86)%\nodejs\node.exe" (
            set "PATH=%ProgramFiles(x86)%\nodejs;%PATH%"
        ) else if exist "%LOCALAPPDATA%\Programs\nodejs\node.exe" (
            set "PATH=%LOCALAPPDATA%\Programs\nodejs;%PATH%"
        ) else (
            echo ERROR: Node.js was installed but not found in PATH
            echo Please restart your terminal and run this script again.
            pause
            exit /b 1
        )
    )
)

:: Check Node.js version
for /f "tokens=1" %%a in ('node -v') do set NODE_VERSION=%%a
echo Found Node.js %NODE_VERSION%
echo.

:: Install dependencies if needed
if not exist "node_modules" (
    echo Installing dependencies...
    call npm install
    if %errorlevel% neq 0 (
        echo ERROR: Failed to install dependencies
        pause
        exit /b 1
    )
    echo Dependencies installed successfully.
    echo.
)

:: Build menu
echo Build Options:
echo   1. Build Windows Installer (x64)
echo   2. Build Windows Installer (x86)
echo   3. Build Portable Version
echo   4. Build All
echo   5. Run Development Mode
echo   6. Exit
echo.

set /p choice="Enter your choice (1-6): "

if "%choice%"=="1" goto build_x64
if "%choice%"=="2" goto build_x86
if "%choice%"=="3" goto build_portable
if "%choice%"=="4" goto build_all
if "%choice%"=="5" goto run_dev
if "%choice%"=="6" exit /b 0

echo Invalid choice. Please run the script again.
pause
exit /b 1

:build_x64
echo.
echo Building Windows x64 Installer...
call npm run build:win
goto build_done

:build_x86
echo.
echo Building Windows x86 Installer...
call npm run build:win32
goto build_done

:build_portable
echo.
echo Building Portable Version...
call npm run build:portable
goto build_done

:build_all
echo.
echo Building All Versions...
call npm run build
goto build_done

:run_dev
echo.
echo Starting Development Mode...
call npm run dev
goto end

:build_done
echo.
if %errorlevel% equ 0 (
    echo ========================================
    echo Build completed successfully!
    echo Output files are in the 'dist' folder.
    echo ========================================
) else (
    echo ========================================
    echo Build failed with error code %errorlevel%
    echo ========================================
)

:end
echo.
pause
exit /b 0

:: ========================================
:: Function to install Node.js
:: ========================================
:install_nodejs
echo ----------------------------------------
echo Downloading Node.js LTS installer...
echo ----------------------------------------

:: Determine architecture
if "%PROCESSOR_ARCHITECTURE%"=="AMD64" (
    set "NODE_ARCH=x64"
) else (
    set "NODE_ARCH=x86"
)

:: Set download URL for Node.js LTS
set "NODE_VERSION_DL=v20.11.0"
set "NODE_MSI=node-%NODE_VERSION_DL%-%NODE_ARCH%.msi"
set "NODE_URL=https://nodejs.org/dist/%NODE_VERSION_DL%/%NODE_MSI%"
set "DOWNLOAD_PATH=%TEMP%\%NODE_MSI%"

echo Downloading from: %NODE_URL%
echo.

:: Download using PowerShell
powershell -Command "& {[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri '%NODE_URL%' -OutFile '%DOWNLOAD_PATH%' -UseBasicParsing}"

if not exist "%DOWNLOAD_PATH%" (
    echo ERROR: Failed to download Node.js installer
    exit /b 1
)

echo Download complete. Installing Node.js...
echo This may require administrator privileges.
echo.

:: Install Node.js silently
msiexec /i "%DOWNLOAD_PATH%" /qn /norestart

if %errorlevel% neq 0 (
    echo Silent install failed. Trying interactive install...
    msiexec /i "%DOWNLOAD_PATH%"
)

:: Clean up
del "%DOWNLOAD_PATH%" 2>nul

echo.
echo Node.js installation complete.
echo ----------------------------------------
exit /b 0
