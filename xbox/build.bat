@echo off
setlocal enabledelayedexpansion
echo ========================================
echo Nedflix Xbox - Build Script
echo ========================================
echo.
echo This script builds the Nedflix Xbox app
echo for Xbox Series S/X (via Dev Mode sideload)
echo.

:: Navigate to script directory
cd /d "%~dp0"
echo Working directory: %cd%
echo.

:: Check for .NET SDK
where dotnet >nul 2>nul
if %errorlevel% neq 0 (
    echo .NET SDK is not installed.
    echo.
    echo Please install .NET 6.0 SDK or later from:
    echo   https://dotnet.microsoft.com/download/dotnet/6.0
    echo.
    echo After installation, restart this script.
    pause
    exit /b 1
)

:: Check .NET SDK version
for /f "tokens=1" %%a in ('dotnet --version') do set DOTNET_VERSION=%%a
echo Found .NET SDK %DOTNET_VERSION%
echo.

:: Check for Windows SDK
if not exist "%ProgramFiles(x86)%\Windows Kits\10\bin" (
    echo WARNING: Windows 10 SDK not detected.
    echo Some features may not work correctly.
    echo.
    echo Install Windows 10 SDK from Visual Studio Installer
    echo or download from: https://developer.microsoft.com/windows/downloads/windows-sdk/
    echo.
)

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
echo   1. Build Debug (x64) - For PC testing
echo   2. Build Release (x64) - For Xbox sideload
echo   3. Build Xbox Configuration (x64)
echo   4. Create MSIX Package (for sideload)
echo   5. Build All Configurations
echo   6. Clean Build Output
echo   7. Open in Visual Studio
echo   8. Exit
echo.

set /p choice="Enter your choice (1-8): "

if "%choice%"=="1" goto build_debug
if "%choice%"=="2" goto build_release
if "%choice%"=="3" goto build_xbox
if "%choice%"=="4" goto create_msix
if "%choice%"=="5" goto build_all
if "%choice%"=="6" goto clean
if "%choice%"=="7" goto open_vs
if "%choice%"=="8" exit /b 0

echo Invalid choice. Please run the script again.
pause
exit /b 1

:build_debug
echo.
echo Building Debug (x64)...
dotnet build -c Debug -p:Platform=x64
goto build_done

:build_release
echo.
echo Building Release (x64)...
dotnet build -c Release -p:Platform=x64
goto build_done

:build_xbox
echo.
echo Building Xbox Configuration (x64)...
dotnet build -c Xbox -p:Platform=x64
goto build_done

:create_msix
echo.
echo Creating MSIX Package for Xbox sideload...
echo.
echo NOTE: For Xbox sideloading, the package must NOT be signed
echo       or signed with a developer certificate.
echo.
dotnet publish -c Release -p:Platform=x64 -p:AppxPackageSigningEnabled=false
if %errorlevel% equ 0 (
    echo.
    echo ========================================
    echo MSIX package created successfully!
    echo.
    echo To install on Xbox:
    echo   1. Enable Dev Mode on your Xbox
    echo   2. Note your Xbox's IP address from Dev Home
    echo   3. Open Device Portal in browser: https://[xbox-ip]:11443
    echo   4. Go to "Apps" section
    echo   5. Click "Add" under "Install app"
    echo   6. Browse to the .msix file in:
    echo      bin\Release\net6.0-windows10.0.19041.0\win10-x64\publish\
    echo   7. Click "Next" then "Install"
    echo ========================================
)
goto build_done

:build_all
echo.
echo Building all configurations...
echo.
echo [1/3] Building Debug...
dotnet build -c Debug -p:Platform=x64
echo.
echo [2/3] Building Release...
dotnet build -c Release -p:Platform=x64
echo.
echo [3/3] Creating MSIX Package...
dotnet publish -c Release -p:Platform=x64 -p:AppxPackageSigningEnabled=false
goto build_done

:clean
echo.
echo Cleaning build output...
dotnet clean
if exist "bin" rmdir /s /q bin
if exist "obj" rmdir /s /q obj
echo Clean complete.
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
    echo.
    echo Output location:
    echo   bin\[Configuration]\net6.0-windows10.0.19041.0\win10-x64\
    echo.
) else (
    echo ========================================
    echo Build failed with error code %errorlevel%
    echo ========================================
    echo.
    echo Common issues:
    echo   - Missing Windows SDK: Install via Visual Studio
    echo   - Missing workloads: Run 'dotnet workload install maui-windows'
    echo   - NuGet errors: Check internet connection
)

:end
echo.
pause
exit /b 0
