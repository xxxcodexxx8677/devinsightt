@echo off
setlocal

if not exist "%~dp0bin\devinsight_demo.exe" (
    echo Binary not found. Running build.bat first...
    call "%~dp0build.bat"
    if %errorlevel% neq 0 exit /b %errorlevel%
)

echo.
echo Launching DevInsight Demo...
echo.
"%~dp0bin\devinsight_demo.exe"
