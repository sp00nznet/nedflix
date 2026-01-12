@echo off
echo ========================================
echo Nedflix Desktop - Install Dependencies
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

cd /d "%~dp0"
echo Installing dependencies...
call npm install

if %errorlevel% equ 0 (
    echo.
    echo Dependencies installed successfully!
    echo.
    echo You can now run:
    echo   - 'npm start' to run the app
    echo   - 'npm run dev' for development mode
    echo   - 'build.bat' to build installers
) else (
    echo.
    echo ERROR: Failed to install dependencies
)

echo.
pause
