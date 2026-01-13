# ========================================
# Nedflix Xbox - PowerShell Build Script
# ========================================
# This script provides advanced build options
# and Xbox deployment utilities

param(
    [Parameter(Position=0)]
    [ValidateSet("build", "publish", "deploy", "clean", "help")]
    [string]$Command = "help",

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

function Test-DotNetSDK {
    try {
        $version = dotnet --version
        Write-Host "Found .NET SDK: $version" -ForegroundColor Green
        return $true
    }
    catch {
        Write-Host "ERROR: .NET SDK not found" -ForegroundColor Red
        Write-Host "Install from: https://dotnet.microsoft.com/download" -ForegroundColor Yellow
        return $false
    }
}

function Build-Project {
    param($Config, $Plat)

    Write-Host ""
    Write-Host "Building $Config ($Plat)..." -ForegroundColor Yellow

    dotnet build -c $Config -p:Platform=$Plat

    if ($LASTEXITCODE -eq 0) {
        Write-Host "Build succeeded!" -ForegroundColor Green
    }
    else {
        Write-Host "Build failed!" -ForegroundColor Red
        exit 1
    }
}

function Publish-Package {
    param($Config, $Plat)

    Write-Host ""
    Write-Host "Creating MSIX package..." -ForegroundColor Yellow

    dotnet publish -c $Config -p:Platform=$Plat -p:AppxPackageSigningEnabled=false

    if ($LASTEXITCODE -eq 0) {
        $outputPath = "bin\$Config\net6.0-windows10.0.19041.0\win10-$Plat\publish\"
        Write-Host ""
        Write-Host "Package created successfully!" -ForegroundColor Green
        Write-Host "Output: $outputPath" -ForegroundColor Cyan

        # Find the MSIX file
        $msixFile = Get-ChildItem -Path $outputPath -Filter "*.msix" -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($msixFile) {
            Write-Host "Package file: $($msixFile.Name)" -ForegroundColor Cyan
        }
    }
    else {
        Write-Host "Package creation failed!" -ForegroundColor Red
        exit 1
    }
}

function Deploy-ToXbox {
    param($IP, $Pin)

    if (-not $IP) {
        Write-Host "ERROR: Xbox IP address required" -ForegroundColor Red
        Write-Host "Usage: .\build.ps1 deploy -XboxIP 192.168.1.100 -XboxPin 123456" -ForegroundColor Yellow
        exit 1
    }

    # Find the MSIX package
    $publishPath = "bin\Release\net6.0-windows10.0.19041.0\win10-x64\publish\"
    $msixFile = Get-ChildItem -Path $publishPath -Filter "*.msix" -ErrorAction SilentlyContinue | Select-Object -First 1

    if (-not $msixFile) {
        Write-Host "No MSIX package found. Building first..." -ForegroundColor Yellow
        Publish-Package -Config "Release" -Plat "x64"
        $msixFile = Get-ChildItem -Path $publishPath -Filter "*.msix" | Select-Object -First 1
    }

    Write-Host ""
    Write-Host "Deploying to Xbox at $IP..." -ForegroundColor Yellow
    Write-Host "Package: $($msixFile.FullName)" -ForegroundColor Cyan

    # Xbox Device Portal API
    $devicePortalUrl = "https://${IP}:11443"
    $apiEndpoint = "$devicePortalUrl/api/app/packagemanager/package"

    # Create credentials if PIN provided
    $headers = @{}
    if ($Pin) {
        $pair = "admin:$Pin"
        $bytes = [System.Text.Encoding]::ASCII.GetBytes($pair)
        $base64 = [System.Convert]::ToBase64String($bytes)
        $headers["Authorization"] = "Basic $base64"
    }

    Write-Host ""
    Write-Host "NOTE: Automatic deployment requires:" -ForegroundColor Yellow
    Write-Host "  1. Xbox in Dev Mode" -ForegroundColor White
    Write-Host "  2. Device Portal enabled" -ForegroundColor White
    Write-Host "  3. Known PIN (set in Dev Home app)" -ForegroundColor White
    Write-Host ""
    Write-Host "Manual deployment steps:" -ForegroundColor Cyan
    Write-Host "  1. Open browser to: $devicePortalUrl" -ForegroundColor White
    Write-Host "  2. Accept the certificate warning" -ForegroundColor White
    Write-Host "  3. Log in with your Xbox Dev Portal credentials" -ForegroundColor White
    Write-Host "  4. Navigate to 'Apps' > 'Install app'" -ForegroundColor White
    Write-Host "  5. Select: $($msixFile.FullName)" -ForegroundColor White
    Write-Host "  6. Click 'Install'" -ForegroundColor White

    # Attempt automatic deployment
    try {
        # Skip certificate validation for self-signed cert
        [System.Net.ServicePointManager]::ServerCertificateValidationCallback = { $true }

        $fileBytes = [System.IO.File]::ReadAllBytes($msixFile.FullName)
        $boundary = [System.Guid]::NewGuid().ToString()

        $bodyLines = @(
            "--$boundary",
            "Content-Disposition: form-data; name=`"package`"; filename=`"$($msixFile.Name)`"",
            "Content-Type: application/octet-stream",
            "",
            [System.Text.Encoding]::Default.GetString($fileBytes),
            "--$boundary--"
        )
        $body = $bodyLines -join "`r`n"

        $result = Invoke-RestMethod -Uri $apiEndpoint -Method Post -Headers $headers -ContentType "multipart/form-data; boundary=$boundary" -Body $body -TimeoutSec 300

        Write-Host ""
        Write-Host "Deployment successful!" -ForegroundColor Green
    }
    catch {
        Write-Host ""
        Write-Host "Automatic deployment failed: $_" -ForegroundColor Yellow
        Write-Host "Please use manual deployment steps above." -ForegroundColor White
    }
}

function Show-Help {
    Write-Host "Usage: .\build.ps1 <command> [options]" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Commands:" -ForegroundColor Yellow
    Write-Host "  build     Build the project"
    Write-Host "  publish   Create MSIX package for sideloading"
    Write-Host "  deploy    Deploy to Xbox (requires -XboxIP)"
    Write-Host "  clean     Clean build output"
    Write-Host "  help      Show this help message"
    Write-Host ""
    Write-Host "Options:" -ForegroundColor Yellow
    Write-Host "  -Configuration  Debug, Release, or Xbox (default: Release)"
    Write-Host "  -Platform       x64, x86, or ARM64 (default: x64)"
    Write-Host "  -XboxIP         Xbox IP address (for deploy command)"
    Write-Host "  -XboxPin        Xbox Device Portal PIN (optional)"
    Write-Host ""
    Write-Host "Examples:" -ForegroundColor Yellow
    Write-Host "  .\build.ps1 build -Configuration Debug"
    Write-Host "  .\build.ps1 publish"
    Write-Host "  .\build.ps1 deploy -XboxIP 192.168.1.100"
    Write-Host ""
    Write-Host "Xbox Setup:" -ForegroundColor Cyan
    Write-Host "  1. Enable Dev Mode: Settings > System > Console info"
    Write-Host "  2. Install 'Dev Mode Activation' from Store"
    Write-Host "  3. Follow on-screen instructions to activate"
    Write-Host "  4. Note your Xbox IP from Dev Home app"
}

# Main execution
if (-not (Test-DotNetSDK)) {
    exit 1
}

switch ($Command) {
    "build" {
        Build-Project -Config $Configuration -Plat $Platform
    }
    "publish" {
        Publish-Package -Config $Configuration -Plat $Platform
    }
    "deploy" {
        Deploy-ToXbox -IP $XboxIP -Pin $XboxPin
    }
    "clean" {
        Write-Host "Cleaning build output..." -ForegroundColor Yellow
        dotnet clean
        if (Test-Path "bin") { Remove-Item -Recurse -Force "bin" }
        if (Test-Path "obj") { Remove-Item -Recurse -Force "obj" }
        Write-Host "Clean complete!" -ForegroundColor Green
    }
    "help" {
        Show-Help
    }
    default {
        Show-Help
    }
}
