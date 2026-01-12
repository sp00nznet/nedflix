@echo off
echo ========================================
echo Nedflix Desktop - Windows Build Script
echo ========================================
echo.

:: Check if Node.js is installed
where node >nul 2>nul
if %errorlevel% neq 0 (
    echo ERROR: Node.js is not installed or not in PATH
    echo Please install Node.js from https://nodejs.org/
    pause
    exit /b 1
)

:: Check Node.js version
for /f "tokens=1" %%a in ('node -v') do set NODE_VERSION=%%a
echo Found Node.js %NODE_VERSION%

:: Navigate to script directory
cd /d "%~dp0"
echo Working directory: %cd%
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
