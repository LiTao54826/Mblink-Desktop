# MBink - Download Skia Prebuilt Binaries
# This script downloads precompiled Skia binaries from skia-python project

param(
    [string]$Version = "m116",
    [string]$Platform = "windows-x64"
)

$ErrorActionPreference = "Stop"

# Set UTF-8 encoding
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$OutputEncoding = [System.Text.Encoding]::UTF8

Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "  Download Skia Prebuilt Binaries" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host ""

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent $ScriptDir
$ThirdPartyDir = Join-Path $ProjectRoot "third_party"
$SkiaDir = Join-Path $ThirdPartyDir "skia-prebuilt"

# Check if already exists
if (Test-Path $SkiaDir) {
    Write-Host "[INFO] Skia prebuilt already exists at: $SkiaDir" -ForegroundColor Yellow
    $response = Read-Host "Overwrite? (y/N)"
    if ($response -ne "y" -and $response -ne "Y") {
        Write-Host "Cancelled." -ForegroundColor Yellow
        exit 0
    }
    Remove-Item $SkiaDir -Recurse -Force
}

# Download URL (from skia-python project)
$Url = "https://github.com/kyamagu/skia-python/releases/download/$Version/skia-$Version-$Platform.zip"
$ZipFile = Join-Path $ThirdPartyDir "skia.zip"

Write-Host "[1/3] Downloading Skia $Version for $Platform..." -ForegroundColor Yellow
Write-Host "URL: $Url" -ForegroundColor Cyan
Write-Host ""

try {
    # Check proxy configuration
    $Proxy = $env:HTTP_PROXY
    if (-not $Proxy) {
        $Proxy = $env:HTTPS_PROXY
    }
    
    if ($Proxy) {
        Write-Host "[INFO] Using proxy: $Proxy" -ForegroundColor Cyan
        $ProxyUri = New-Object System.Uri($Proxy)
        $WebProxy = New-Object System.Net.WebProxy($ProxyUri, $false)
        [System.Net.WebRequest]::DefaultWebProxy = $WebProxy
    }
    
    Invoke-WebRequest -Uri $Url -OutFile $ZipFile -UseBasicParsing
    Write-Host "[OK] Downloaded successfully" -ForegroundColor Green
} catch {
    Write-Host "[ERROR] Download failed: $_" -ForegroundColor Red
    Write-Host ""
    Write-Host "Available versions:" -ForegroundColor Yellow
    Write-Host "  m116 (Chrome 116, August 2023)" -ForegroundColor White
    Write-Host "  m114 (Chrome 114, May 2023)" -ForegroundColor White
    Write-Host "  m110 (Chrome 110, February 2023)" -ForegroundColor White
    Write-Host ""
    Write-Host "Try a different version:" -ForegroundColor Cyan
    Write-Host "  .\scripts\download_skia_prebuilt.ps1 -Version m114" -ForegroundColor White
    Write-Host ""
    Write-Host "Or check the releases page:" -ForegroundColor Cyan
    Write-Host "  https://github.com/kyamagu/skia-python/releases" -ForegroundColor White
    exit 1
}

Write-Host ""
Write-Host "[2/3] Extracting..." -ForegroundColor Yellow

try {
    Expand-Archive $ZipFile -DestinationPath $SkiaDir -Force
    Write-Host "[OK] Extracted to: $SkiaDir" -ForegroundColor Green
} catch {
    Write-Host "[ERROR] Extraction failed: $_" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "[3/3] Cleaning up..." -ForegroundColor Yellow
Remove-Item $ZipFile -Force
Write-Host "[OK] Cleaned up" -ForegroundColor Green

Write-Host ""
Write-Host "==========================================" -ForegroundColor Green
Write-Host "  Skia Prebuilt Installed!" -ForegroundColor Green
Write-Host "==========================================" -ForegroundColor Green
Write-Host ""
Write-Host "Location: $SkiaDir" -ForegroundColor Cyan
Write-Host ""

# Check directory structure
if (Test-Path (Join-Path $SkiaDir "include")) {
    Write-Host "[OK] Found include directory" -ForegroundColor Green
} else {
    Write-Host "[WARNING] include directory not found" -ForegroundColor Yellow
}

if (Test-Path (Join-Path $SkiaDir "lib")) {
    Write-Host "[OK] Found lib directory" -ForegroundColor Green
} else {
    Write-Host "[WARNING] lib directory not found" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "Next steps:" -ForegroundColor Cyan
Write-Host "  1. Configure CMake with Skia support:" -ForegroundColor White
Write-Host "     cd build" -ForegroundColor Gray
Write-Host "     cmake .. -DMBINK_USE_SKIA=ON" -ForegroundColor Gray
Write-Host ""
Write-Host "  2. Build project:" -ForegroundColor White
Write-Host "     cmake --build . --config Release" -ForegroundColor Gray
Write-Host ""

Read-Host "Press Enter to exit"

