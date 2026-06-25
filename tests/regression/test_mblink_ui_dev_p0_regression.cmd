@echo off
setlocal
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0test_mblink_ui_dev_p0_regression.ps1" %*
exit /b %errorlevel%
