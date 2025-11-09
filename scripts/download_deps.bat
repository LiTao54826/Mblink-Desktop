@echo off
REM LightUI Third-Party Dependencies Download Script (Windows)
REM Purpose: Automatically download and configure all dependencies
REM
REM NOTE: This script uses English only to avoid encoding issues.
REM       For better Chinese support, please use PowerShell script:
REM       .\scripts\download_deps.ps1

setlocal enabledelayedexpansion

echo ==========================================
echo   LightUI Dependencies Download Script
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
REM
REM Method 3: System proxy settings
REM   Configure in Windows Settings
REM ==========================================

REM Read proxy from environment variables
if defined HTTP_PROXY (
    set PROXY=%HTTP_PROXY%
) else if defined HTTPS_PROXY (
    set PROXY=%HTTPS_PROXY%
)

REM If needed, set proxy directly here (Uncomment one line)
REM NOTE: Only choose ONE proxy type, do not uncomment both lines
REM HTTP proxy (Recommended, better compatibility)
set PROXY=http://127.0.0.1:7890
REM SOCKS5 proxy (Try this if HTTP doesn't work)
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
    git config --global http.proxy "%PROXY%"
    git config --global https.proxy "%PROXY%"

    echo [OK] Git proxy configured
    echo.
) else (
    echo ==========================================
    echo   No Proxy Configured
    echo ==========================================
    echo [INFO] If download fails, please set proxy:
    echo   set HTTP_PROXY=http://127.0.0.1:7890
    echo   set HTTPS_PROXY=http://127.0.0.1:7890
    echo Or edit script and uncomment: set PROXY=
    echo.
)

cd /d "%~dp0..\third_party"

REM 检查Git是否安装
where git >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [错误] 未找到 git，请先安装 Git for Windows
    echo 下载地址: https://git-scm.com/download/win
    pause
    exit /b 1
)

REM 下载QuickJS
echo [1/4] 下载 QuickJS...
if exist "quickjs" (
    echo QuickJS 已存在，跳过
) else (
    echo 正在下载 QuickJS...

    REM 构建PowerShell命令（支持代理）
    if defined PROXY (
        powershell -Command "$proxy = New-Object System.Net.WebProxy('%PROXY%'); $proxy.BypassProxyOnLocal = $false; [System.Net.WebRequest]::DefaultWebProxy = $proxy; Invoke-WebRequest -Uri 'https://bellard.org/quickjs/quickjs-2024-01-13.tar.xz' -OutFile 'quickjs.tar.xz'"
    ) else (
        powershell -Command "Invoke-WebRequest -Uri 'https://bellard.org/quickjs/quickjs-2024-01-13.tar.xz' -OutFile 'quickjs.tar.xz'"
    )

    if %ERRORLEVEL% EQU 0 (
        echo [成功] QuickJS 下载完成
        REM 解压需要7-Zip或其他工具
        echo [提示] 请手动解压 quickjs.tar.xz 到 quickjs 目录
        echo   或使用 7-Zip: 7z x quickjs.tar.xz ^&^& 7z x quickjs.tar
        echo   或使用 tar (Windows 10+): tar -xf quickjs.tar.xz
        pause
    ) else (
        echo [错误] QuickJS 下载失败
    )
)
echo.

REM 下载SDL3
echo [2/4] 下载 SDL3...
if exist "SDL3" (
    echo SDL3 已存在，跳过
) else (
    echo 正在克隆 SDL3...
    git clone --depth 1 https://github.com/libsdl-org/SDL SDL3
    if %ERRORLEVEL% EQU 0 (
        echo [成功] SDL3 下载完成
    ) else (
        echo [错误] SDL3 下载失败
    )
)
echo.

REM 下载Yoga
echo [3/4] 下载 Yoga...
if exist "yoga" (
    echo Yoga 已存在，跳过
) else (
    echo 正在克隆 Yoga...
    git clone --depth 1 https://github.com/facebook/yoga.git
    if %ERRORLEVEL% EQU 0 (
        echo [成功] Yoga 下载完成
    ) else (
        echo [错误] Yoga 下载失败
    )
)
echo.

REM 下载Skia（可选）
echo [4/4] 下载 Skia (可选)...
if exist "skia" (
    echo Skia 已存在，跳过
) else (
    set /p DOWNLOAD_SKIA="Skia 较大且编译时间长，是否下载? (y/N): "
    if /i "!DOWNLOAD_SKIA!"=="y" (
        echo 正在克隆 Skia...
        git clone https://github.com/google/skia.git
        if %ERRORLEVEL% EQU 0 (
            echo [成功] Skia 下载完成
            echo [注意] Skia 需要运行 python3 tools/git-sync-deps
            echo [注意] 然后使用 GN 和 Ninja 编译
        ) else (
            echo [错误] Skia 下载失败
        )
    ) else (
        echo 跳过 Skia 下载
        echo 提示: 可以稍后手动下载或使用预编译版本
    )
)
echo.

echo ==========================================
echo   依赖库下载完成！
echo ==========================================
echo.
echo 下一步:
echo   1. 编译QuickJS: cd third_party\quickjs ^&^& nmake (需要MSVC)
echo   2. 配置CMake: mkdir build ^&^& cd build ^&^& cmake ..
echo   3. 编译项目: cmake --build . --config Release
echo.
echo 详细信息请查看: third_party\README.md
echo.
pause

