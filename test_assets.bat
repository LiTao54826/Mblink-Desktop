@echo off
REM 测试 app_bundler 资源打包功能
REM 将 test_assets.js 和 assets 目录打包进 exe

setlocal

REM 设置路径
set BUNDLER=build\bin\Release\app_bundler.exe
set INPUT=examples\preact_demo\test_assets.js
set ASSETS=examples\preact_demo\assets
set OUTPUT=test_assets.exe

REM 检查 bundler 是否存在
if not exist "%BUNDLER%" (
    echo 错误: 找不到 app_bundler.exe
    echo 请先构建项目: cmake --build build --config Release
    exit /b 1
)

REM 检查输入文件
if not exist "%INPUT%" (
    echo 错误: 找不到输入文件 %INPUT%
    exit /b 1
)

REM 检查资源目录
if not exist "%ASSETS%" (
    echo 错误: 找不到资源目录 %ASSETS%
    exit /b 1
)

echo ========================================
echo   打包测试资源到 exe
echo ========================================
echo.

REM 执行打包
"%BUNDLER%" "%INPUT%" -o "%OUTPUT%" --assets "%ASSETS%" --title "资源测试" --verbose

if %ERRORLEVEL% EQU 0 (
    echo.
    echo 打包成功! 运行 %OUTPUT% 测试资源加载
) else (
    echo.
    echo 打包失败!
)

endlocal
