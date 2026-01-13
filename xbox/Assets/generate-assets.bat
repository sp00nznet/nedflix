@echo off
echo ========================================
echo Nedflix Xbox - Asset Generator
echo ========================================
echo.
echo This script generates placeholder assets
echo for the Xbox UWP app.
echo.

cd /d "%~dp0"

:: Check for ImageMagick
where magick >nul 2>nul
if %errorlevel% neq 0 (
    echo ImageMagick not found. Generating SVG placeholders instead...
    echo.
    echo To generate PNG assets, install ImageMagick:
    echo   https://imagemagick.org/script/download.php
    echo.
    echo Or use Visual Studio Asset Generator:
    echo   1. Open Nedflix.Xbox.csproj in Visual Studio
    echo   2. Double-click Package.appxmanifest
    echo   3. Go to Visual Assets tab
    echo   4. Use Asset Generator with a 400x400 source image
    echo.
    goto create_svg
)

echo Generating PNG assets with ImageMagick...
echo.

:: Define colors
set BG_COLOR=#1a1a2e
set PRIMARY=#e94560

:: Generate each required size
echo Creating StoreLogo.png (50x50)...
magick -size 50x50 xc:%BG_COLOR% -fill %PRIMARY% -gravity center -pointsize 24 -annotate 0 "N" StoreLogo.png

echo Creating Square44x44Logo.png...
magick -size 44x44 xc:%BG_COLOR% -fill %PRIMARY% -gravity center -pointsize 20 -annotate 0 "N" Square44x44Logo.png

echo Creating Square44x44Logo.targetsize-24_altform-unplated.png...
magick -size 24x24 xc:%BG_COLOR% -fill %PRIMARY% -gravity center -pointsize 14 -annotate 0 "N" Square44x44Logo.targetsize-24_altform-unplated.png

echo Creating SmallTile.png (71x71)...
magick -size 71x71 xc:%BG_COLOR% -fill %PRIMARY% -gravity center -pointsize 32 -annotate 0 "N" SmallTile.png

echo Creating Square150x150Logo.png...
magick -size 150x150 xc:%BG_COLOR% -fill %PRIMARY% -gravity center -pointsize 48 -annotate 0 "NEDFLIX" Square150x150Logo.png

echo Creating Wide310x150Logo.png...
magick -size 310x150 xc:%BG_COLOR% -fill %PRIMARY% -gravity center -pointsize 48 -annotate 0 "NEDFLIX" Wide310x150Logo.png

echo Creating LargeTile.png (310x310)...
magick -size 310x310 xc:%BG_COLOR% -fill %PRIMARY% -gravity center -pointsize 72 -annotate 0 "NEDFLIX" LargeTile.png

echo Creating SplashScreen.png (620x300)...
magick -size 620x300 xc:%BG_COLOR% -fill %PRIMARY% -gravity center -pointsize 72 -annotate 0 "NEDFLIX" SplashScreen.png

echo Creating nedflix.ico...
magick -size 256x256 xc:%BG_COLOR% -fill %PRIMARY% -gravity center -pointsize 128 -annotate 0 "N" -define icon:auto-resize=256,128,64,48,32,16 nedflix.ico

echo.
echo ========================================
echo Assets generated successfully!
echo ========================================
goto end

:create_svg
:: Create minimal placeholder SVGs that can be converted later
echo Creating placeholder files...

:: Create a simple placeholder for each required asset
(
echo ^<svg xmlns="http://www.w3.org/2000/svg" width="150" height="150"^>
echo   ^<rect width="150" height="150" fill="#1a1a2e"/^>
echo   ^<text x="75" y="90" text-anchor="middle" fill="#e94560" font-size="48" font-family="Arial"^>N^</text^>
echo ^</svg^>
) > placeholder.svg

echo.
echo Placeholder created: placeholder.svg
echo.
echo To complete setup:
echo   1. Open placeholder.svg in an image editor
echo   2. Export as PNG at required sizes (see README.md)
echo   3. Or use Visual Studio Asset Generator
echo.

:end
pause
