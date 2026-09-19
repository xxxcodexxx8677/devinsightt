@echo off
setlocal

echo ===================================================
echo           DEVINSIGHT BUILD SCRIPT (C++17)
echo ===================================================

set ROOT=%~dp0
set CXX=g++

where g++ >nul 2>&1
if %ERRORLEVEL% neq 0 (
    if exist "%ROOT%..\tools\w64devkit\bin\g++.exe" (
        set CXX=%ROOT%..\tools\w64devkit\bin\g++.exe
        set PATH=%ROOT%..\tools\w64devkit\bin;%PATH%
    ) else (
        echo [ERROR] g++ compiler not found in PATH or ..\tools\w64devkit\bin.
        exit /b 1
    )
)

echo Using Compiler: %CXX%

if not exist "%ROOT%bin" (
    mkdir "%ROOT%bin"
)

echo.
echo [1/5] Compiling test_memory...
"%CXX%" -std=c++17 -O2 -I"%ROOT%include" "%ROOT%src\memory_tracker.cpp" "%ROOT%tests\test_memory.cpp" -o "%ROOT%bin\test_memory.exe"
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Failed to compile test_memory.
    exit /b 1
)

echo [2/5] Compiling test_scheduler...
"%CXX%" -std=c++17 -O2 -I"%ROOT%include" "%ROOT%src\scheduler.cpp" "%ROOT%tests\test_scheduler.cpp" -o "%ROOT%bin\test_scheduler.exe"
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Failed to compile test_scheduler.
    exit /b 1
)

echo [3/5] Compiling test_global_hooks...
"%CXX%" -std=c++17 -O2 -I"%ROOT%include" "%ROOT%src\memory_tracker.cpp" "%ROOT%src\global_hooks.cpp" "%ROOT%tests\test_global_hooks.cpp" -o "%ROOT%bin\test_global_hooks.exe"
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Failed to compile test_global_hooks.
    exit /b 1
)

echo [4/5] Compiling devinsight_demo...
"%CXX%" -std=c++17 -O2 -I"%ROOT%include" "%ROOT%src\memory_tracker.cpp" "%ROOT%src\scheduler.cpp" "%ROOT%src\cli_monitor.cpp" "%ROOT%examples\demo_workload.cpp" -o "%ROOT%bin\devinsight_demo.exe"
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Failed to compile devinsight_demo.
    exit /b 1
)

echo [5/5] Compiling devinsight_benchmark...
"%CXX%" -std=c++17 -O2 -I"%ROOT%include" "%ROOT%src\memory_tracker.cpp" "%ROOT%src\scheduler.cpp" "%ROOT%examples\benchmark.cpp" -o "%ROOT%bin\devinsight_benchmark.exe"
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Failed to compile devinsight_benchmark.
    exit /b 1
)

echo.
echo ===================================================
echo [SUCCESS] DevInsight successfully built into .\bin!
echo ===================================================
echo  - bin\test_memory.exe
echo  - bin\test_scheduler.exe
echo  - bin\test_global_hooks.exe
echo  - bin\devinsight_demo.exe
echo  - bin\devinsight_benchmark.exe
echo ===================================================
