#include "devinsight/cli_monitor.hpp"
#include <iostream>
#include <iomanip>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace devinsight {

static void enable_virtual_terminal() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }
#endif
}

CliMonitor::CliMonitor(const MemoryTracker& mem_tracker, 
                       const TaskScheduler& scheduler,
                       std::chrono::milliseconds refresh_rate)
    : mem_tracker_(mem_tracker),
      scheduler_(scheduler),
      refresh_rate_(refresh_rate) {
    enable_virtual_terminal();
}

CliMonitor::~CliMonitor() {
    stop();
}

void CliMonitor::start() {
    if (!is_active_.exchange(true)) {
        monitor_thread_ = std::thread(&CliMonitor::monitor_loop, this);
    }
}

void CliMonitor::stop() {
    if (is_active_.exchange(false)) {
        if (monitor_thread_.joinable()) {
            monitor_thread_.join();
        }
    }
}

void CliMonitor::render_frame() const {
    auto mem = mem_tracker_.get_stats();
    auto sched = scheduler_.get_stats();

    // Move cursor to top-left (without full clear to avoid flicker)
    std::cout << "\033[H";

    std::cout << "\033[1;36m======================================================================\033[0m\n";
    std::cout << "\033[1;32m      DEVINSIGHT: REAL-TIME RUNTIME TELEMETRY & SYSTEM MONITOR        \033[0m\n";
    std::cout << "\033[1;36m======================================================================\033[0m\n";

    // Memory subsystem block
    std::cout << "\033[1;33m[ MEMORY ENGINE METRICS ]\033[0m\n";
    std::cout << "  Active Heap In Use    : " 
              << std::fixed << std::setprecision(2) << (mem.active_bytes / 1024.0) << " KB (" 
              << mem.active_bytes << " bytes)          \n";
    std::cout << "  Peak Heap Usage       : " 
              << std::fixed << std::setprecision(2) << (mem.peak_bytes / 1024.0) << " KB (" 
              << mem.peak_bytes << " bytes)          \n";
    std::cout << "  Cumulative Allocated  : " 
              << std::fixed << std::setprecision(2) << (mem.total_allocated_bytes / 1024.0) << " KB          \n";
    std::cout << "  Active Allocations    : " << mem.active_allocations << " block(s)          \n";
    std::cout << "  Total Allocations     : " << mem.total_allocations << "          \n";
    std::cout << "  Total Deallocations   : " << mem.total_deallocations << "          \n";

    std::cout << "\033[1;36m----------------------------------------------------------------------\033[0m\n";

    // Concurrency / Scheduler block
    std::cout << "\033[1;35m[ CONCURRENCY & SCHEDULER ENGINE ]\033[0m\n";
    std::cout << "  Worker Threads Pool   : " << sched.worker_count << " threads          \n";
    std::cout << "  Active Workers Busy   : " << sched.active_workers << " threads          \n";
    std::cout << "  Tasks In Queue        : " << sched.pending_tasks << " tasks          \n";
    std::cout << "  Tasks Completed       : " << sched.completed_tasks << " tasks          \n";
    std::cout << "  Average Queue Wait    : " << std::fixed << std::setprecision(1) << sched.avg_wait_us << " us          \n";
    std::cout << "  Average Execution     : " << std::fixed << std::setprecision(1) << sched.avg_exec_us << " us          \n";

    std::cout << "\033[1;36m======================================================================\033[0m\n";
    std::cout << "\033[90mPress Ctrl+C to terminate or wait for workload completion...\033[0m      \n";
    std::cout.flush();
}

void CliMonitor::monitor_loop() {
    // Initial clear screen
    std::cout << "\033[2J\033[H";
    while (is_active_.load()) {
        render_frame();
        std::this_thread::sleep_for(refresh_rate_);
    }
}

} // namespace devinsight
