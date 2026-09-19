$ErrorActionPreference = "Stop"

Write-Host "===================================================" -ForegroundColor Cyan
Write-Host "          DEVINSIGHT BUILD SCRIPT (C++17)          " -ForegroundColor Green
Write-Host "===================================================" -ForegroundColor Cyan

$scriptDir = $PSScriptRoot
$gpp = Get-Command g++ -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source -ErrorAction SilentlyContinue

if (-not $gpp) {
    $fallbackGpp = Join-Path $scriptDir "..\tools\w64devkit\bin\g++.exe"
    if (Test-Path $fallbackGpp) {
        $gpp = (Resolve-Path $fallbackGpp).Path
        $env:PATH = "$((Resolve-Path (Join-Path $scriptDir '..\tools\w64devkit\bin')).Path);$env:PATH"
    } else {
        Write-Error "g++ compiler not found in PATH or ..\tools\w64devkit\bin"
        exit 1
    }
}

Write-Host "Using Compiler: $gpp" -ForegroundColor Yellow

$binDir = Join-Path $scriptDir "bin"
if (-not (Test-Path $binDir)) {
    New-Item -ItemType Directory -Path $binDir | Out-Null
}

$inc = Join-Path $scriptDir "include"

Write-Host "`n[1/6] Compiling test_memory..." -ForegroundColor Magenta
& $gpp -std=c++17 -O2 -I"$inc" "$scriptDir\src\memory_tracker.cpp" "$scriptDir\tests\test_memory.cpp" -o "$binDir\test_memory.exe"
if ($LASTEXITCODE -ne 0) { exit 1 }

Write-Host "[2/6] Compiling test_scheduler..." -ForegroundColor Magenta
& $gpp -std=c++17 -O2 -I"$inc" "$scriptDir\src\scheduler.cpp" "$scriptDir\tests\test_scheduler.cpp" -o "$binDir\test_scheduler.exe"
if ($LASTEXITCODE -ne 0) { exit 1 }

Write-Host "[3/6] Compiling test_global_hooks..." -ForegroundColor Magenta
& $gpp -std=c++17 -O2 -I"$inc" "$scriptDir\src\memory_tracker.cpp" "$scriptDir\src\global_hooks.cpp" "$scriptDir\tests\test_global_hooks.cpp" -o "$binDir\test_global_hooks.exe"
if ($LASTEXITCODE -ne 0) { exit 1 }

Write-Host "[4/6] Compiling devinsight_demo..." -ForegroundColor Magenta
& $gpp -std=c++17 -O2 -I"$inc" "$scriptDir\src\memory_tracker.cpp" "$scriptDir\src\scheduler.cpp" "$scriptDir\src\cli_monitor.cpp" "$scriptDir\examples\demo_workload.cpp" -o "$binDir\devinsight_demo.exe"
if ($LASTEXITCODE -ne 0) { exit 1 }

Write-Host "[5/6] Compiling devinsight_benchmark..." -ForegroundColor Magenta
& $gpp -std=c++17 -O2 -I"$inc" "$scriptDir\src\memory_tracker.cpp" "$scriptDir\src\scheduler.cpp" "$scriptDir\examples\benchmark.cpp" -o "$binDir\devinsight_benchmark.exe"
if ($LASTEXITCODE -ne 0) { exit 1 }

Write-Host "[6/6] Compiling real_world_server..." -ForegroundColor Magenta
& $gpp -std=c++17 -O2 -I"$inc" "$scriptDir\src\memory_tracker.cpp" "$scriptDir\src\scheduler.cpp" "$scriptDir\examples\real_world_server.cpp" -o "$binDir\real_world_server.exe"
if ($LASTEXITCODE -ne 0) { exit 1 }

Write-Host "`n===================================================" -ForegroundColor Cyan
Write-Host "[SUCCESS] All 6 DevInsight binaries built into .\bin!" -ForegroundColor Green
Write-Host "===================================================" -ForegroundColor Cyan
Write-Host "  - bin\test_memory.exe"
Write-Host "  - bin\test_scheduler.exe"
Write-Host "  - bin\test_global_hooks.exe"
Write-Host "  - bin\devinsight_demo.exe"
Write-Host "  - bin\devinsight_benchmark.exe"
Write-Host "  - bin\real_world_server.exe"
Write-Host "===================================================" -ForegroundColor Cyan
