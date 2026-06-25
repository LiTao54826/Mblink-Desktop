@echo off
chcp 65001 >nul
echo ============================================
echo   MBlink Modern Desktop Demo - Build ^& Run
echo ============================================
echo.

cd /d %~dp0\..\..

echo [1/2] Building esm_loader (Release)...
cmake --build build --config Release --target esm_loader 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo  BUILD FAILED! Trying Debug...
    cmake --build build --config Debug --target esm_loader 2>&1
    if %ERRORLEVEL% NEQ 0 (
        echo  Both Release and Debug build failed!
        pause
        exit /b 1
    )
    set EXE=build\bin\Debug\esm_loader.exe
) else (
    set EXE=build\bin\Release\esm_loader.exe
)

echo.
echo [2/2] Running Modern Desktop Demo (borderless)...
echo Command: %EXE% examples\modern_desktop_demo\app.js --borderless --width 1100 --height 700
echo.
%EXE% examples\modern_desktop_demo\app.js --borderless --width 1100 --height 700

pause

