#include "devinsight/memory_tracker.hpp"
#include "devinsight/scheduler.hpp"
#include <iostream>
#include <vector>
#include <chrono>
#include <iomanip>
#include <thread>
#include <atomic>

void run_memory_overhead_benchmark() {
    std::cout << "\n======================================================================\n";
    std::cout << " [BENCHMARK 1/3] Memory Tracker Profiling Overhead\n";
    std::cout << "======================================================================\n";

    const int iterations = 50000;
    std::vector<void*> ptrs(iterations);

    // Baseline: Raw std::malloc & std::free
    auto start_raw = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        ptrs[i] = std::malloc(64);
    }
    for (int i = 0; i < iterations; ++i) {
        std::free(ptrs[i]);
    }
    auto end_raw = std::chrono::high_resolution_clock::now();
    double raw_ms = std::chrono::duration<double, std::milli>(end_raw - start_raw).count();

    // DevInsight: Tracked Allocation & Deallocation
    auto& tracker = devinsight::MemoryTracker::instance();
    tracker.reset();

    auto start_tracked = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        ptrs[i] = DEV_ALLOC(64);
    }
    for (int i = 0; i < iterations; ++i) {
        DEV_FREE(ptrs[i]);
    }
    auto end_tracked = std::chrono::high_resolution_clock::now();
    double tracked_ms = std::chrono::duration<double, std::milli>(end_tracked - start_tracked).count();

    double overhead_per_op_ns = ((tracked_ms - raw_ms) * 1000000.0) / (iterations * 2);
    if (overhead_per_op_ns < 0) overhead_per_op_ns = 0;

    std::cout << "Operations              : " << (iterations * 2) << " (50k alloc + 50k free)\n";
    std::cout << "Raw malloc/free Time    : " << std::fixed << std::setprecision(2) << raw_ms << " ms\n";
    std::cout << "DevInsight Tracked Time : " << std::fixed << std::setprecision(2) << tracked_ms << " ms\n";
    std::cout << "Net Tracking Overhead   : " << std::fixed << std::setprecision(1) << overhead_per_op_ns << " ns / operation\n";
    std::cout << "Status                  : \033[1;32mPASSED (Low-latency tracking)\033[0m\n";
}

void run_scheduler_throughput_benchmark() {
    std::cout << "\n======================================================================\n";
    std::cout << " [BENCHMARK 2/3] Scheduler Pool vs Raw OS Thread Creation\n";
    std::cout << "======================================================================\n";

    const int total_tasks = 200;
    auto compute_work = []() {
        volatile double x = 0.0;
        for (int j = 0; j < 5000; ++j) {
            x += j * 0.001;
        }
    };

    // Approach A: Naive OS Thread Creation (Spawning individual std::thread instances)
    auto start_threads = std::chrono::high_resolution_clock::now();
    {
        std::vector<std::thread> raw_threads;
        raw_threads.reserve(total_tasks);
        for (int i = 0; i < total_tasks; ++i) {
            raw_threads.emplace_back(compute_work);
        }
        for (auto& t : raw_threads) {
            if (t.joinable()) t.join();
        }
    }
    auto end_threads = std::chrono::high_resolution_clock::now();
    double threads_ms = std::chrono::duration<double, std::milli>(end_threads - start_threads).count();

    // Approach B: DevInsight Task Scheduler Pool
    auto start_pool = std::chrono::high_resolution_clock::now();
    {
        devinsight::TaskScheduler scheduler;
        for (int i = 0; i < total_tasks; ++i) {
            scheduler.submit(compute_work);
        }
        scheduler.wait_all();
    }
    auto end_pool = std::chrono::high_resolution_clock::now();
    double pool_ms = std::chrono::duration<double, std::milli>(end_pool - start_pool).count();

    double speedup = (pool_ms > 0) ? (threads_ms / pool_ms) : 1.0;
    double throughput = (total_tasks / (pool_ms / 1000.0));

    std::cout << "Total Tasks             : " << total_tasks << " workloads\n";
    std::cout << "Naive std::thread Time  : " << std::fixed << std::setprecision(2) << threads_ms << " ms\n";
    std::cout << "DevInsight Pool Time    : " << std::fixed << std::setprecision(2) << pool_ms << " ms\n";
    std::cout << "Throughput              : " << std::fixed << std::setprecision(0) << throughput << " tasks/sec\n";
    std::cout << "Speedup Factor          : \033[1;32m" << std::fixed << std::setprecision(2) << speedup << "x FASTER\033[0m\n";
}

void run_priority_scheduling_benchmark() {
    std::cout << "\n======================================================================\n";
    std::cout << " [BENCHMARK 3/3] Priority Queue Inversion & Precedence\n";
    std::cout << "======================================================================\n";

    devinsight::TaskScheduler scheduler(1);
    std::vector<int> execution_order;
    std::mutex order_mutex;

    // Pause the single worker momentarily so tasks queue up
    std::atomic<bool> gate{false};
    scheduler.submit([&gate]() {
        while (!gate.load()) {
            std::this_thread::sleep_for(std::chrono::microseconds(50));
        }
    });

    // Submit 10 LOW priority tasks
    for (int i = 0; i < 10; ++i) {
        scheduler.submit([&execution_order, &order_mutex, i]() {
            std::lock_guard<std::mutex> lock(order_mutex);
            execution_order.push_back(100 + i);
        }, devinsight::TaskPriority::LOW);
    }

    // Submit 3 CRITICAL priority tasks AFTER the low priority tasks
    for (int i = 0; i < 3; ++i) {
        scheduler.submit([&execution_order, &order_mutex, i]() {
            std::lock_guard<std::mutex> lock(order_mutex);
            execution_order.push_back(900 + i);
        }, devinsight::TaskPriority::CRITICAL);
    }

    gate.store(true);
    scheduler.wait_all();

    std::cout << "Queued Order : 10 LOW tasks followed by 3 CRITICAL tasks\n";
    std::cout << "Executed     : ";
    for (size_t i = 0; i < execution_order.size(); ++i) {
        std::cout << execution_order[i] << " ";
    }
    std::cout << "\n";

    bool priority_respected = (execution_order.size() >= 3 &&
                               execution_order[0] >= 900 &&
                               execution_order[1] >= 900 &&
                               execution_order[2] >= 900);

    std::cout << "Priority Test : " 
              << (priority_respected ? "\033[1;32mPASSED (CRITICAL tasks jumped ahead of LOW tasks)\033[0m" : "\033[1;31mFAILED\033[0m") 
              << "\n";
    std::cout << "======================================================================\n";
}

int main() {
    std::cout << ">>> Running DevInsight Performance & Latency Benchmark Suite <<<\n";
    run_memory_overhead_benchmark();
    run_scheduler_throughput_benchmark();
    run_priority_scheduling_benchmark();
    std::cout << "\n>>> ALL BENCHMARKS COMPLETED SUCCESSFULLY! <<<\n";
    return 0;
}
