@echo off
setlocal enabledelayedexpansion
echo ========================================
echo Nedflix Desktop - Install Dependencies
echo ========================================
echo.

:: Navigate to script directory
cd /d "%~dp0"

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

:: Show Node.js version
for /f "tokens=1" %%a in ('node -v') do set NODE_VERSION=%%a
echo Found Node.js %NODE_VERSION%
echo.

echo Installing dependencies...
call npm install

if %errorlevel% equ 0 (
    echo.
    echo ========================================
    echo Dependencies installed successfully!
    echo ========================================
    echo.
    echo You can now run:
    echo   - 'npm start' to run the app
    echo   - 'npm run dev' for development mode
    echo   - 'build.bat' to build installers
) else (
    echo.
    echo ========================================
    echo ERROR: Failed to install dependencies
    echo ========================================
)

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
