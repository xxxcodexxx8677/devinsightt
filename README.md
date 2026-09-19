# DevInsight: C++ Userspace Memory Profiler & Task Scheduler

[![Language](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![Build](https://img.shields.io/badge/Build-CMake%20%7C%20Make%20%7C%20Batch-brightgreen.svg)](#building-and-testing)
[![Tests](https://img.shields.io/badge/Tests-All%20Passing-brightgreen.svg)](#benchmark-results)

**DevInsight** is a lightweight, high-performance C++17 runtime toolkit for memory leak profiling, priority-driven task scheduling, and real-time terminal telemetry.

---

## Benchmark Results

The automated benchmark suite (`bin/devinsight_benchmark.exe`) measures real system performance:

| Metric | Raw Standard Baseline | DevInsight Engine | Result |
| :--- | :--- | :--- | :--- |
| **Allocation Latency** | 6.73 ms (100k ops) | 27.06 ms (100k ops) | **203 ns** overhead / operation |
| **Task Throughput** | 22.23 ms (`std::thread`) | 0.95 ms (Worker Pool) | **23.41x FASTER** (210,526 tasks/sec) |
| **Priority Inversion** | FIFO queuing | Priority Queue (`TaskPriority`) | **CRITICAL tasks preempt LOW tasks** |

---

## Key Features

1. **Zero-Code-Change Global `new`/`delete` Interception:**
   - Overloads global `operator new`, `operator new[]`, `operator delete`, `operator delete[]` to automatically track any standard C++ allocation without macros.
   - Also supports explicit source-code tracking macros (`DEV_ALLOC`, `DEV_FREE`, `DEV_NEW`, `DEV_DELETE`) for file and line attribution.
2. **Reentrancy Protection & Leak Detection:**
   - Uses atomic flags and thread-local guards to prevent recursive allocation loops when internal data structures allocate memory.
   - Automated post-mortem audit report pinpointing exact source file and line for every unfreed block.
3. **Priority Task Scheduler:**
   - Fixed worker thread pool sized to hardware concurrency.
   - Priority queue dispatching (`TaskPriority::CRITICAL`, `HIGH`, `NORMAL`, `LOW`).
   - Nanosecond-precision task metrics: tracks queue wait latency ($\Delta t_{\text{wait}}$) and CPU execution time ($\Delta t_{\text{exec}}$).
4. **Real-Time Terminal Telemetry (CLI Monitor):**
   - Live console dashboard rendered via ANSI escape sequences showing heap memory gauges and worker thread activity in real time.

---

## Directory Layout

```
devinsight/
├── include/devinsight/
│   ├── memory_tracker.hpp     # Memory profiler & leak detection API
│   ├── scheduler.hpp          # Priority-driven task scheduler
│   ├── global_hooks.hpp       # Global operator new/delete overrides
│   └── cli_monitor.hpp        # Live ANSI terminal monitor
├── src/
│   ├── memory_tracker.cpp     # Memory tracking implementation
│   ├── scheduler.cpp          # Worker thread loops & sync
│   ├── global_hooks.cpp       # Standard new/delete interception
│   └── cli_monitor.cpp        # Console rendering
├── tests/
│   ├── test_memory.cpp        # Memory tracking unit tests
│   ├── test_scheduler.cpp     # Concurrency stress tests
│   └── test_global_hooks.cpp  # Standard new/delete interception tests
├── examples/
│   ├── demo_workload.cpp      # Interactive demo with live telemetry
│   └── benchmark.cpp          # Performance & latency benchmark suite
├── build.bat / build.ps1      # One-click Windows build scripts
├── run_demo.bat               # Interactive demo launcher
├── Makefile / CMakeLists.txt  # Cross-platform build files
└── README.md
```

---

## Building and Testing

### Build All Targets (One-Click)
```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1
```
*(Or run `build.bat` in Command Prompt)*

### Run Automated Tests
```powershell
.\bin\test_memory.exe
.\bin\test_scheduler.exe
.\bin\test_global_hooks.exe
```

### Run Performance Benchmarks
```powershell
.\bin\devinsight_benchmark.exe
```

### Run the Interactive Live Demo
```powershell
.\bin\devinsight_demo.exe
```
*(Or run `run_demo.bat`)*
