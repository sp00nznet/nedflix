@echo off
echo Nedflix N64 Build
echo =================
echo.

:: Check for libdragon
if not exist "C:\libdragon" (
    echo ERROR: libdragon not found at C:\libdragon
    echo Install from: https://github.com/DragonMinded/libdragon
    pause
    exit /b 1
)

set N64_INST=C:\libdragon
set PATH=%N64_INST%\bin;%PATH%

echo Building...
make

if exist "nedflix.z64" (
    echo.
    echo SUCCESS: nedflix.z64 created
    echo Use with N64 emulator or flashcart
) else (
    echo.
    echo BUILD FAILED
)

pause
