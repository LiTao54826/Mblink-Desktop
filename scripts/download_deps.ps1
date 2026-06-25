# MBlink Third-Party Dependencies Download Script (PowerShell)
# Purpose: Automatically download and configure all dependencies
# PowerShell version with better UTF-8 support and error handling

# Set UTF-8 encoding
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$OutputEncoding = [System.Text.Encoding]::UTF8

Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "  MBlink Dependencies Download (PowerShell)" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host ""

# ==========================================
# Proxy Configuration
# ==========================================
# Method 1: Set via environment variables (Recommended)
#   $env:HTTP_PROXY = "http://127.0.0.1:7890"
#   $env:HTTPS_PROXY = "http://127.0.0.1:7890"
#
# Method 2: Set directly below (Uncomment)
#   $Proxy = "http://127.0.0.1:7890"
#
# Method 3: Set when running script
#   $env:HTTP_PROXY="http://127.0.0.1:7890"; .\download_deps.ps1
# ==========================================

# Read proxy from environment variables
$Proxy = $env:HTTP_PROXY
if (-not $Proxy) {
    $Proxy = $env:HTTPS_PROXY
}

# If needed, set proxy directly here (Uncomment)
# $Proxy = "http://127.0.0.1:7890"
# $Proxy = "socks5://127.0.0.1:7890"

if ($Proxy) {
    Write-Host "==========================================" -ForegroundColor Cyan
    Write-Host "  Proxy Configuration" -ForegroundColor Cyan
    Write-Host "==========================================" -ForegroundColor Cyan
    Write-Host "[OK] Using proxy: $Proxy" -ForegroundColor Green

    # Set environment variables
    $env:http_proxy = $Proxy
    $env:https_proxy = $Proxy
    $env:HTTP_PROXY = $Proxy
    $env:HTTPS_PROXY = $Proxy
    $env:ALL_PROXY = $Proxy

    # Configure Git proxy
    git config --global http.proxy "$Proxy" 2>$null
    git config --global https.proxy "$Proxy" 2>$null

    Write-Host "[OK] Git proxy configured" -ForegroundColor Green

    # Configure PowerShell Web proxy
    $ProxyUri = New-Object System.Uri($Proxy)
    $WebProxy = New-Object System.Net.WebProxy($ProxyUri, $false)
    [System.Net.WebRequest]::DefaultWebProxy = $WebProxy

    Write-Host "[OK] PowerShell proxy configured" -ForegroundColor Green
    Write-Host ""
} else {
    Write-Host "==========================================" -ForegroundColor Yellow
    Write-Host "  No Proxy Configured" -ForegroundColor Yellow
    Write-Host "==========================================" -ForegroundColor Yellow
    Write-Host "[INFO] If download fails, please set proxy:" -ForegroundColor Yellow
    Write-Host '  $env:HTTP_PROXY = "http://127.0.0.1:7890"' -ForegroundColor Yellow
    Write-Host '  $env:HTTPS_PROXY = "http://127.0.0.1:7890"' -ForegroundColor Yellow
    Write-Host "Or edit script and uncomment: `$Proxy=" -ForegroundColor Yellow
    Write-Host ""
}

# Switch to third_party directory
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent $ScriptDir
$ThirdPartyDir = Join-Path $ProjectRoot "third_party"

if (-not (Test-Path $ThirdPartyDir)) {
    New-Item -ItemType Directory -Path $ThirdPartyDir | Out-Null
}

Set-Location $ThirdPartyDir

# Check if Git is installed
function Test-Command {
    param($Command)
    try {
        Get-Command $Command -ErrorAction Stop | Out-Null
        return $true
    } catch {
        return $false
    }
}

if (-not (Test-Command "git")) {
    Write-Host "[ERROR] Git not found, please install Git for Windows" -ForegroundColor Red
    Write-Host "Download: https://git-scm.com/download/win" -ForegroundColor Yellow
    Read-Host "Press Enter to exit"
    exit 1
}

# Download QuickJS
function Download-QuickJS {
    Write-Host "[1/4] Downloading QuickJS..." -ForegroundColor Yellow

    if (Test-Path "quickjs") {
        Write-Host "[OK] QuickJS already exists, skipping" -ForegroundColor Green
        return
    }

    Write-Host "Downloading QuickJS..." -ForegroundColor Cyan

    try {
        $url = "https://bellard.org/quickjs/quickjs-2024-01-13.tar.xz"
        $output = "quickjs.tar.xz"

        # Use Invoke-WebRequest to download
        Invoke-WebRequest -Uri $url -OutFile $output -UseBasicParsing

        Write-Host "[OK] QuickJS downloaded" -ForegroundColor Green
        Write-Host "[INFO] Please extract quickjs.tar.xz to quickjs directory" -ForegroundColor Yellow
        Write-Host "  Using tar (Windows 10+): tar -xf quickjs.tar.xz" -ForegroundColor Yellow
        Write-Host "  Or using 7-Zip: 7z x quickjs.tar.xz; 7z x quickjs.tar" -ForegroundColor Yellow
        Write-Host ""
    } catch {
        Write-Host "[ERROR] QuickJS download failed: $_" -ForegroundColor Red
        Write-Host ""
    }
}

# Download SDL3
function Download-SDL3 {
    Write-Host "[2/4] Downloading SDL3..." -ForegroundColor Yellow

    if (Test-Path "SDL3") {
        Write-Host "[OK] SDL3 already exists, skipping" -ForegroundColor Green
        return
    }

    Write-Host "Cloning SDL3..." -ForegroundColor Cyan

    try {
        git clone --depth 1 https://github.com/libsdl-org/SDL SDL3

        if ($LASTEXITCODE -eq 0) {
            Write-Host "[OK] SDL3 downloaded" -ForegroundColor Green
        } else {
            Write-Host "[ERROR] SDL3 download failed" -ForegroundColor Red
        }
    } catch {
        Write-Host "[ERROR] SDL3 download failed: $_" -ForegroundColor Red
    }

    Write-Host ""
}

# Download Yoga
function Download-Yoga {
    Write-Host "[3/4] Downloading Yoga..." -ForegroundColor Yellow

    if (Test-Path "yoga") {
        Write-Host "[OK] Yoga already exists, skipping" -ForegroundColor Green
        return
    }

    Write-Host "Cloning Yoga..." -ForegroundColor Cyan

    try {
        git clone --depth 1 https://github.com/facebook/yoga.git

        if ($LASTEXITCODE -eq 0) {
            Write-Host "[OK] Yoga downloaded" -ForegroundColor Green
        } else {
            Write-Host "[ERROR] Yoga download failed" -ForegroundColor Red
        }
    } catch {
        Write-Host "[ERROR] Yoga download failed: $_" -ForegroundColor Red
    }

    Write-Host ""
}

# Download Skia (Optional)
function Download-Skia {
    Write-Host "[4/4] Downloading Skia (Optional)..." -ForegroundColor Yellow

    if (Test-Path "skia") {
        Write-Host "[OK] Skia already exists, skipping" -ForegroundColor Green
        return
    }

    $response = Read-Host "Skia is large and takes long to compile. Download? (y/N)"

    if ($response -ne "y" -and $response -ne "Y") {
        Write-Host "Skipping Skia download" -ForegroundColor Yellow
        Write-Host "[INFO] You can download it later or use prebuilt binaries" -ForegroundColor Yellow
        return
    }

    Write-Host "Cloning Skia..." -ForegroundColor Cyan

    try {
        git clone https://github.com/google/skia.git

        if ($LASTEXITCODE -eq 0) {
            Write-Host "[OK] Skia downloaded" -ForegroundColor Green
            Write-Host "[NOTE] Skia requires: python3 tools/git-sync-deps" -ForegroundColor Yellow
            Write-Host "[NOTE] Then build with GN and Ninja" -ForegroundColor Yellow
        } else {
            Write-Host "[ERROR] Skia download failed" -ForegroundColor Red
        }
    } catch {
        Write-Host "[ERROR] Skia download failed: $_" -ForegroundColor Red
    }

    Write-Host ""
}

# Main function
function Main {
    # Download all dependencies
    Download-QuickJS
    Download-SDL3
    Download-Yoga
    Download-Skia

    Write-Host ""
    Write-Host "==========================================" -ForegroundColor Green
    Write-Host "  Download Complete!" -ForegroundColor Green
    Write-Host "==========================================" -ForegroundColor Green
    Write-Host ""
    Write-Host "Next steps:" -ForegroundColor Cyan
    Write-Host "  1. Compile QuickJS: cd third_party\quickjs; nmake (requires MSVC)" -ForegroundColor White
    Write-Host "  2. Configure CMake: mkdir build; cd build; cmake .." -ForegroundColor White
    Write-Host "  3. Build project: cmake --build . --config Release" -ForegroundColor White
    Write-Host ""
    Write-Host "For details, see: third_party\README.md" -ForegroundColor Cyan
    Write-Host ""
}

# Run main function
Main

# Wait for user input
Read-Host "Press Enter to exit"

