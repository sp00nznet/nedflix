# ========================================
# Nedflix Xbox - PowerShell Build Script
# ========================================
# Supports both Client and Desktop versions
#
# Client: Connects to remote Nedflix server
# Desktop: Standalone with embedded server (no auth)

param(
    [Parameter(Position=0)]
    [ValidateSet("build", "publish", "deploy", "clean", "copy-webui", "help")]
    [string]$Command = "help",

    [Parameter()]
    [ValidateSet("Client", "Desktop")]
    [string]$Version = "Client",

    [Parameter()]
    [ValidateSet("Debug", "Release", "Xbox")]
    [string]$Configuration = "Release",

    [Parameter()]
    [ValidateSet("x64", "x86", "ARM64")]
    [string]$Platform = "x64",

    [Parameter()]
    [string]$XboxIP,

    [Parameter()]
    [string]$XboxPin
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $ScriptDir

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Nedflix Xbox - Build Script" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

function Install-DotNetSDK {
    Write-Host "Installing .NET SDK..." -ForegroundColor Yellow
    Write-Host ""

    # Check if winget is available (Windows 11 / Windows 10 with App Installer)
    if (Get-Command winget -ErrorAction SilentlyContinue) {
        Write-Host "Installing .NET SDK via winget..." -ForegroundColor Cyan
        winget install Microsoft.DotNet.SDK.8 --silent --accept-package-agreements --accept-source-agreements

        if ($LASTEXITCODE -eq 0) {
            Write-Host ".NET SDK installed successfully!" -ForegroundColor Green
            # Refresh environment variables
            $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")
            return $true
        }
    }

    # Check if Chocolatey is available
    if (Get-Command choco -ErrorAction SilentlyContinue) {
        Write-Host "Installing .NET SDK via Chocolatey..." -ForegroundColor Cyan
        choco install dotnet-sdk -y

        if ($LASTEXITCODE -eq 0) {
            Write-Host ".NET SDK installed successfully!" -ForegroundColor Green
            # Refresh environment variables
            $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")
            return $true
        }
    }

    # Manual download and install
    Write-Host "Downloading .NET SDK installer..." -ForegroundColor Cyan
    $dotnetVersion = "8.0.1"
    $installerUrl = "https://download.visualstudio.microsoft.com/download/pr/69f3e301-5243-4c5f-8b8e-e98e5d59ef3a/74e7b8d48b6a7bb60ec6eb0e5c6c8f0e/dotnet-sdk-8.0.101-win-x64.exe"
    $installerPath = "$env:TEMP\dotnet-sdk-installer.exe"

    try {
        Write-Host "Downloading from: $installerUrl" -ForegroundColor Cyan
        [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
        Invoke-WebRequest -Uri $installerUrl -OutFile $installerPath -UseBasicParsing

        Write-Host "Installing .NET SDK (this may take a few minutes)..." -ForegroundColor Cyan
        Write-Host "You may see a UAC prompt..." -ForegroundColor Yellow

        Start-Process -FilePath $installerPath -ArgumentList "/install","/quiet","/norestart" -Wait -Verb RunAs

        # Clean up
        Remove-Item $installerPath -Force -ErrorAction SilentlyContinue

        # Refresh environment variables
        $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")

        Write-Host ""
        Write-Host ".NET SDK installation complete!" -ForegroundColor Green
        Write-Host "Please restart your PowerShell session to use the SDK" -ForegroundColor Yellow
        return $true
    }
    catch {
        Write-Host "ERROR: Failed to download or install .NET SDK" -ForegroundColor Red
        Write-Host "Please install manually from: https://dotnet.microsoft.com/download/dotnet/8.0" -ForegroundColor Yellow
        return $false
    }
}

function Test-DotNetSDK {
    try {
        $version = dotnet --version
        Write-Host "Found .NET SDK: $version" -ForegroundColor Green
        return $true
    }
    catch {
        Write-Host ".NET SDK not found. Installing automatically..." -ForegroundColor Yellow
        Write-Host ""

        $installed = Install-DotNetSDK

        if ($installed) {
            # Try again after install
            try {
                $version = dotnet --version
                Write-Host "Found .NET SDK: $version" -ForegroundColor Green
                return $true
            }
            catch {
                Write-Host "ERROR: .NET SDK was installed but not found in PATH" -ForegroundColor Red
                Write-Host "Please restart your PowerShell session and run this script again" -ForegroundColor Yellow
                return $false
            }
        }
        else {
            Write-Host "ERROR: Failed to install .NET SDK" -ForegroundColor Red
            Write-Host "Please install manually from: https://dotnet.microsoft.com/download/dotnet/8.0" -ForegroundColor Yellow
            return $false
        }
    }
}

function Get-BuildConfiguration {
    param($Ver, $Config)

    # Map version + config to actual build configuration
    switch ($Ver) {
        "Client" {
            switch ($Config) {
                "Debug" { return "Debug" }
                "Release" { return "Client" }
                "Xbox" { return "ClientXbox" }
            }
        }
        "Desktop" {
            switch ($Config) {
                "Debug" { return "Desktop" }
                "Release" { return "Desktop" }
                "Xbox" { return "DesktopXbox" }
            }
        }
    }
    return "Release"
}

function Copy-WebUI {
    Write-Host "Copying Web UI files for Desktop mode..." -ForegroundColor Yellow

    $webUIPath = Join-Path $ScriptDir "WebUI"
    $desktopPublic = Join-Path $ScriptDir "..\desktop\public"

    if (-not (Test-Path $webUIPath)) {
        New-Item -ItemType Directory -Path $webUIPath | Out-Null
    }

    if (Test-Path $desktopPublic) {
        Copy-Item -Path "$desktopPublic\*" -Destination $webUIPath -Recurse -Force
        Write-Host "Web UI files copied successfully!" -ForegroundColor Green
    }
    else {
        Write-Host "WARNING: Desktop public folder not found at $desktopPublic" -ForegroundColor Yellow
        Write-Host "Desktop mode requires the Web UI files." -ForegroundColor Yellow
    }
}

function Build-Project {
    param($Ver, $Config, $Plat)

    $buildConfig = Get-BuildConfiguration -Ver $Ver -Config $Config

    Write-Host ""
    Write-Host "Building $Ver version ($buildConfig, $Plat)..." -ForegroundColor Yellow

    # Ensure WebUI exists for Desktop builds
    if ($Ver -eq "Desktop") {
        Copy-WebUI
    }

    dotnet build -c $buildConfig -p:Platform=$Plat

    if ($LASTEXITCODE -eq 0) {
        Write-Host "Build succeeded!" -ForegroundColor Green
    }
    else {
        Write-Host "Build failed!" -ForegroundColor Red
        exit 1
    }
}

function Publish-Package {
    param($Ver, $Plat)

    # For Xbox deployment, use the Xbox configuration
    $buildConfig = Get-BuildConfiguration -Ver $Ver -Config "Xbox"

    Write-Host ""
    Write-Host "Creating $Ver MSIX package for Xbox..." -ForegroundColor Yellow

    # Ensure WebUI exists for Desktop builds
    if ($Ver -eq "Desktop") {
        Copy-WebUI
    }

    dotnet publish -c $buildConfig -p:Platform=$Plat -p:AppxPackageSigningEnabled=false

    if ($LASTEXITCODE -eq 0) {
        $outputPath = "bin\$buildConfig\net6.0-windows10.0.19041.0\win10-$Plat\publish\"
        Write-Host ""
        Write-Host "$Ver package created successfully!" -ForegroundColor Green
        Write-Host "Output: $outputPath" -ForegroundColor Cyan

        $msixFile = Get-ChildItem -Path $outputPath -Filter "*.msix" -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($msixFile) {
            Write-Host "Package file: $($msixFile.Name)" -ForegroundColor Cyan
        }

        Write-Host ""
        Write-Host "Version Info:" -ForegroundColor Yellow
        if ($Ver -eq "Client") {
            Write-Host "  This is the CLIENT version" -ForegroundColor White
            Write-Host "  - Connects to your remote Nedflix server" -ForegroundColor White
            Write-Host "  - Configure server URL on first launch" -ForegroundColor White
        }
        else {
            Write-Host "  This is the DESKTOP version" -ForegroundColor White
            Write-Host "  - Standalone with embedded server" -ForegroundColor White
            Write-Host "  - No authentication required" -ForegroundColor White
            Write-Host "  - Access media from Xbox storage" -ForegroundColor White
        }
    }
    else {
        Write-Host "Package creation failed!" -ForegroundColor Red
        exit 1
    }
}

function Deploy-ToXbox {
    param($Ver, $IP, $Pin)

    if (-not $IP) {
        Write-Host "ERROR: Xbox IP address required" -ForegroundColor Red
        Write-Host "Usage: .\build.ps1 deploy -Version $Ver -XboxIP 192.168.1.100" -ForegroundColor Yellow
        exit 1
    }

    $buildConfig = Get-BuildConfiguration -Ver $Ver -Config "Xbox"
    $publishPath = "bin\$buildConfig\net6.0-windows10.0.19041.0\win10-x64\publish\"
    $msixFile = Get-ChildItem -Path $publishPath -Filter "*.msix" -ErrorAction SilentlyContinue | Select-Object -First 1

    if (-not $msixFile) {
        Write-Host "No MSIX package found. Building first..." -ForegroundColor Yellow
        Publish-Package -Ver $Ver -Plat "x64"
        $msixFile = Get-ChildItem -Path $publishPath -Filter "*.msix" | Select-Object -First 1
    }

    Write-Host ""
    Write-Host "Deploying $Ver version to Xbox at $IP..." -ForegroundColor Yellow
    Write-Host "Package: $($msixFile.FullName)" -ForegroundColor Cyan

    $devicePortalUrl = "https://${IP}:11443"

    Write-Host ""
    Write-Host "Manual deployment steps:" -ForegroundColor Cyan
    Write-Host "  1. Open browser to: $devicePortalUrl" -ForegroundColor White
    Write-Host "  2. Accept the certificate warning" -ForegroundColor White
    Write-Host "  3. Log in with your Xbox Dev Portal credentials" -ForegroundColor White
    Write-Host "  4. Navigate to 'Apps' > 'Install app'" -ForegroundColor White
    Write-Host "  5. Select: $($msixFile.FullName)" -ForegroundColor White
    Write-Host "  6. Click 'Install'" -ForegroundColor White
}

function Show-Help {
    Write-Host "Usage: .\build.ps1 <command> [options]" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Commands:" -ForegroundColor Yellow
    Write-Host "  build       Build the project"
    Write-Host "  publish     Create MSIX package for Xbox sideloading"
    Write-Host "  deploy      Deploy to Xbox (requires -XboxIP)"
    Write-Host "  clean       Clean build output"
    Write-Host "  copy-webui  Copy Web UI files for Desktop mode"
    Write-Host "  help        Show this help message"
    Write-Host ""
    Write-Host "Options:" -ForegroundColor Yellow
    Write-Host "  -Version        Client or Desktop (default: Client)"
    Write-Host "  -Configuration  Debug, Release, or Xbox (default: Release)"
    Write-Host "  -Platform       x64, x86, or ARM64 (default: x64)"
    Write-Host "  -XboxIP         Xbox IP address (for deploy command)"
    Write-Host "  -XboxPin        Xbox Device Portal PIN (optional)"
    Write-Host ""
    Write-Host "Version Differences:" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "  CLIENT version:" -ForegroundColor White
    Write-Host "    - Connects to your Nedflix server over the network"
    Write-Host "    - Requires server URL configuration"
    Write-Host "    - Best for accessing your media library remotely"
    Write-Host ""
    Write-Host "  DESKTOP version:" -ForegroundColor White
    Write-Host "    - Standalone app with embedded web server"
    Write-Host "    - No authentication required"
    Write-Host "    - Access media stored on Xbox or connected USB"
    Write-Host "    - Works offline (no server needed)"
    Write-Host ""
    Write-Host "Examples:" -ForegroundColor Yellow
    Write-Host "  .\build.ps1 build -Version Client -Configuration Debug"
    Write-Host "  .\build.ps1 publish -Version Desktop"
    Write-Host "  .\build.ps1 deploy -Version Client -XboxIP 192.168.1.100"
    Write-Host ""
    Write-Host "Quick Start:" -ForegroundColor Cyan
    Write-Host "  # Build Client for Xbox"
    Write-Host "  .\build.ps1 publish -Version Client"
    Write-Host ""
    Write-Host "  # Build Desktop for Xbox"
    Write-Host "  .\build.ps1 publish -Version Desktop"
}

# Main execution
if (-not (Test-DotNetSDK)) {
    exit 1
}

switch ($Command) {
    "build" {
        Build-Project -Ver $Version -Config $Configuration -Plat $Platform
    }
    "publish" {
        Publish-Package -Ver $Version -Plat $Platform
    }
    "deploy" {
        Deploy-ToXbox -Ver $Version -IP $XboxIP -Pin $XboxPin
    }
    "clean" {
        Write-Host "Cleaning build output..." -ForegroundColor Yellow
        dotnet clean
        if (Test-Path "bin") { Remove-Item -Recurse -Force "bin" }
        if (Test-Path "obj") { Remove-Item -Recurse -Force "obj" }
        if (Test-Path "WebUI") { Remove-Item -Recurse -Force "WebUI" }
        Write-Host "Clean complete!" -ForegroundColor Green
    }
    "copy-webui" {
        Copy-WebUI
    }
    "help" {
        Show-Help
    }
    default {
        Show-Help
    }
}
