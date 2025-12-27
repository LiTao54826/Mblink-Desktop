@echo off
REM 富文本编辑器测试运行脚本

echo ========================================
echo 富文本编辑 API 测试套件
echo ========================================
echo.

:menu
echo 请选择要运行的测试：
echo.
echo 1. 快速测试 (推荐)
echo 2. API 单元测试
echo 3. 基础功能测试
echo 4. 交互式编辑器
echo 5. 运行所有测试
echo 0. 退出
echo.

set /p choice="请输入选项 (0-5): "

if "%choice%"=="1" goto quick_test
if "%choice%"=="2" goto api_test
if "%choice%"=="3" goto app_test
if "%choice%"=="4" goto interactive
if "%choice%"=="5" goto all_tests
if "%choice%"=="0" goto end

echo 无效选项，请重新选择
echo.
goto menu

:quick_test
echo.
echo ========================================
echo 运行快速测试...
echo ========================================
build\bin\Release\esm_loader.exe examples\rich_text_editor\quick_test.js
goto menu

:api_test
echo.
echo ========================================
echo 运行 API 单元测试...
echo ========================================
build\bin\Release\esm_loader.exe examples\rich_text_editor\api_test.js
goto menu

:app_test
echo.
echo ========================================
echo 运行基础功能测试...
echo ========================================
build\bin\Release\esm_loader.exe examples\rich_text_editor\app.js
goto menu

:interactive
echo.
echo ========================================
echo 启动交互式编辑器...
echo ========================================
build\bin\Release\esm_loader.exe examples\rich_text_editor\interactive_editor.js
goto menu

:all_tests
echo.
echo ========================================
echo 运行所有测试...
echo ========================================
echo.
echo [1/3] 快速测试
build\bin\Release\esm_loader.exe examples\rich_text_editor\quick_test.js
echo.
echo [2/3] API 单元测试
build\bin\Release\esm_loader.exe examples\rich_text_editor\api_test.js
echo.
echo [3/3] 基础功能测试
build\bin\Release\esm_loader.exe examples\rich_text_editor\app.js
echo.
echo ========================================
echo 所有测试完成！
echo ========================================
goto menu

:end
echo.
echo 感谢使用！
exit /b 0
