@echo off
REM MBlink Third-Party Dependencies Download Script (Windows)
REM Purpose: Automatically download and configure all dependencies
REM
REM NOTE: This script uses English only to avoid encoding issues.
REM       For Chinese support, please use PowerShell script:
REM       .\scripts\download_deps.ps1

setlocal enabledelayedexpansion

echo ==========================================
echo   MBlink Dependencies Download Script
echo ==========================================
echo.

REM ==========================================
REM Proxy Configuration
REM ==========================================
REM Method 1: Set via environment variables (Recommended)
REM   set HTTP_PROXY=http://127.0.0.1:7890
REM   set HTTPS_PROXY=http://127.0.0.1:7890
REM
REM Method 2: Set directly below (Uncomment)
REM   set PROXY=http://127.0.0.1:7890
REM ==========================================

REM Read proxy from environment variables
if defined HTTP_PROXY (
    set PROXY=%HTTP_PROXY%
) else if defined HTTPS_PROXY (
    set PROXY=%HTTPS_PROXY%
)

REM If needed, set proxy directly here (Uncomment one line)
REM set PROXY=http://127.0.0.1:7890
REM set PROXY=socks5://127.0.0.1:7890

if defined PROXY (
    echo ==========================================
    echo   Proxy Configuration
    echo ==========================================
    echo [OK] Using proxy: %PROXY%

    REM Set environment variables
    set http_proxy=%PROXY%
    set https_proxy=%PROXY%
    set HTTP_PROXY=%PROXY%
    set HTTPS_PROXY=%PROXY%
    set ALL_PROXY=%PROXY%
    
    REM Configure Git proxy
    git config --global http.proxy "%PROXY%" 2>nul
    git config --global https.proxy "%PROXY%" 2>nul
    
    echo [OK] Git proxy configured
    echo.
) else (
    echo ==========================================
    echo   No Proxy Configured
    echo ==========================================
    echo [INFO] If download fails, please set proxy:
    echo   set HTTP_PROXY=http://127.0.0.1:7890
    echo   set HTTPS_PROXY=http://127.0.0.1:7890
    echo.
)

REM Switch to third_party directory
cd /d "%~dp0..\third_party" 2>nul || (
    mkdir "%~dp0..\third_party" 2>nul
    cd /d "%~dp0..\third_party"
)

REM Check if Git is installed
where git >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Git not found, please install Git for Windows
    echo Download: https://git-scm.com/download/win
    pause
    exit /b 1
)

REM ==========================================
REM Download QuickJS
REM ==========================================
echo [1/4] Downloading QuickJS...

if exist "quickjs" (
    echo [OK] QuickJS already exists, skipping
    goto :download_sdl3
)

echo Downloading QuickJS...

REM Build PowerShell command (with proxy support)
if defined PROXY (
    powershell -Command "$proxy = New-Object System.Net.WebProxy('%PROXY%'); $proxy.BypassProxyOnLocal = $false; [System.Net.WebRequest]::DefaultWebProxy = $proxy; Invoke-WebRequest -Uri 'https://bellard.org/quickjs/quickjs-2024-01-13.tar.xz' -OutFile 'quickjs.tar.xz'"
) else (
    powershell -Command "Invoke-WebRequest -Uri 'https://bellard.org/quickjs/quickjs-2024-01-13.tar.xz' -OutFile 'quickjs.tar.xz'"
)

if %ERRORLEVEL% EQU 0 (
    echo [OK] QuickJS downloaded
    echo [INFO] Please extract quickjs.tar.xz to quickjs directory
    echo   Using 7-Zip: 7z x quickjs.tar.xz ^&^& 7z x quickjs.tar
    echo   Using tar (Windows 10+): tar -xf quickjs.tar.xz
    echo.
) else (
    echo [ERROR] QuickJS download failed
    echo.
)

:download_sdl3
REM ==========================================
REM Download SDL3
REM ==========================================
echo [2/4] Downloading SDL3...

if exist "SDL3" (
    echo [OK] SDL3 already exists, skipping
    goto :download_yoga
)

echo Cloning SDL3...
git clone --depth 1 https://github.com/libsdl-org/SDL SDL3

if %ERRORLEVEL% EQU 0 (
    echo [OK] SDL3 downloaded
) else (
    echo [ERROR] SDL3 download failed
)
echo.

:download_yoga
REM ==========================================
REM Download Yoga
REM ==========================================
echo [3/4] Downloading Yoga...

if exist "yoga" (
    echo [OK] Yoga already exists, skipping
    goto :download_skia
)

echo Cloning Yoga...
git clone --depth 1 https://github.com/facebook/yoga.git

if %ERRORLEVEL% EQU 0 (
    echo [OK] Yoga downloaded
) else (
    echo [ERROR] Yoga download failed
)
echo.

:download_skia
REM ==========================================
REM Download Skia (Optional)
REM ==========================================
echo [4/4] Downloading Skia (Optional)...

if exist "skia" (
    echo [OK] Skia already exists, skipping
    goto :done
)

set /p DOWNLOAD_SKIA="Skia is large and takes long to compile. Download? (y/N): "

if /i "!DOWNLOAD_SKIA!"=="y" (
    echo Cloning Skia...
    git clone https://github.com/google/skia.git
    
    if %ERRORLEVEL% EQU 0 (
        echo [OK] Skia downloaded
        echo [NOTE] Skia requires: python3 tools/git-sync-deps
        echo [NOTE] Then build with GN and Ninja
    ) else (
        echo [ERROR] Skia download failed
    )
) else (
    echo Skipping Skia download
    echo [INFO] You can download it later or use prebuilt binaries
)
echo.

:done
REM ==========================================
REM Summary
REM ==========================================
echo ==========================================
echo   Download Complete!
echo ==========================================
echo.
echo Next steps:
echo   1. Compile QuickJS: cd third_party\quickjs ^&^& nmake (requires MSVC)
echo   2. Configure CMake: mkdir build ^&^& cd build ^&^& cmake ..
echo   3. Build project: cmake --build . --config Release
echo.
echo For details, see: third_party\README.md
echo.

pause

