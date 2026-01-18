@echo off
echo ========================================
echo Nedflix Xbox - Asset Generator
echo ========================================
echo.
echo This script generates properly sized assets
echo from reel.png for the Xbox UWP app.
echo.

cd /d "%~dp0"

:: Check for source image
if not exist "reel.png" (
    echo ERROR: reel.png not found in Assets folder!
    echo Please ensure reel.png exists before running this script.
    goto end
)

:: Check for ImageMagick
where magick >nul 2>nul
if %errorlevel% neq 0 (
    echo ImageMagick not found.
    echo.
    echo To generate properly sized PNG assets, install ImageMagick:
    echo   https://imagemagick.org/script/download.php
    echo.
    echo Or use Visual Studio Asset Generator:
    echo   1. Open Nedflix.Xbox.csproj in Visual Studio
    echo   2. Double-click Package.appxmanifest
    echo   3. Go to Visual Assets tab
    echo   4. Use Asset Generator with reel.png as source
    echo.
    goto end
)

echo Generating PNG assets from reel.png with ImageMagick...
echo.

:: Define background color for padding
set BG_COLOR=#1a1a2e

:: Generate each required size from reel.png
echo Creating StoreLogo.png (50x50)...
magick reel.png -resize 50x50 -gravity center -background "%BG_COLOR%" -extent 50x50 StoreLogo.png

echo Creating Square44x44Logo.png...
magick reel.png -resize 44x44 -gravity center -background "%BG_COLOR%" -extent 44x44 Square44x44Logo.png

echo Creating Square44x44Logo.targetsize-24_altform-unplated.png...
magick reel.png -resize 24x24 -gravity center -background "%BG_COLOR%" -extent 24x24 Square44x44Logo.targetsize-24_altform-unplated.png

echo Creating SmallTile.png (71x71)...
magick reel.png -resize 71x71 -gravity center -background "%BG_COLOR%" -extent 71x71 SmallTile.png

echo Creating Square150x150Logo.png...
magick reel.png -resize 150x150 -gravity center -background "%BG_COLOR%" -extent 150x150 Square150x150Logo.png

echo Creating Wide310x150Logo.png...
magick reel.png -resize 150x150 -gravity center -background "%BG_COLOR%" -extent 310x150 Wide310x150Logo.png

echo Creating LargeTile.png (310x310)...
magick reel.png -resize 310x310 -gravity center -background "%BG_COLOR%" -extent 310x310 LargeTile.png

echo Creating SplashScreen.png (620x300)...
magick reel.png -resize 300x300 -gravity center -background "%BG_COLOR%" -extent 620x300 SplashScreen.png

echo.
echo ========================================
echo Assets generated successfully!
echo ========================================

:end
pause
