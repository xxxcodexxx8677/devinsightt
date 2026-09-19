#include "devinsight/memory_tracker.hpp"
#include "devinsight/scheduler.hpp"
#include "devinsight/cli_monitor.hpp"
#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>

struct Particle {
    float x, y, z;
    float vx, vy, vz;
    char tag[32];
};

void simulate_heavy_compute(int iterations) {
    volatile double result = 0.0;
    for (int i = 0; i < iterations; ++i) {
        result += std::sin(i * 0.01) * std::cos(i * 0.01);
    }
}

int main() {
    std::cout << "Starting DevInsight Comprehensive Runtime Demo...\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    auto& tracker = devinsight::MemoryTracker::instance();
    devinsight::TaskScheduler scheduler(6); // 6 worker threads
    devinsight::CliMonitor monitor(tracker, scheduler, std::chrono::milliseconds(150));

    // Start real-time CLI dashboard
    monitor.start();

    // =========================================================================
    // STAGE 1: Heap Allocation & Intentional Leak Simulation
    // =========================================================================
    std::vector<void*> temp_buffers;
    for (int i = 0; i < 20; ++i) {
        size_t alloc_size = (i + 1) * 16 * 1024; // 16 KB to 320 KB
        void* buf = DEV_ALLOC(alloc_size);
        temp_buffers.push_back(buf);
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
    }

    // Deliberate memory leak 1: Unfreed simulation buffer
    void* intentional_leak_buffer = DEV_ALLOC(128 * 1024); // 128 KB leak
    (void)intentional_leak_buffer; // Not added to temp_buffers, never freed!

    // Deliberate memory leak 2: Particle object
    Particle* leaked_particle = DEV_NEW(Particle);
    leaked_particle->x = 100.0f;
    (void)leaked_particle; // Never deleted!

    // Free normal buffers
    for (void* buf : temp_buffers) {
        DEV_FREE(buf);
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
    temp_buffers.clear();

    // =========================================================================
    // STAGE 2: High-Throughput Concurrency & Multithreaded Memory Tracking
    // =========================================================================
    const int total_tasks = 120;
    for (int i = 0; i < total_tasks; ++i) {
        scheduler.submit([i]() {
            // Memory allocation inside concurrent worker thread
            size_t local_size = 4096 + (i % 8) * 1024;
            void* worker_mem = DEV_ALLOC(local_size);

            // Compute workload
            simulate_heavy_compute(25000);

            // Deallocate worker memory
            DEV_FREE(worker_mem);
        });

        if (i % 15 == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(60));
        }
    }

    // Wait for all queued tasks to finish executing
    scheduler.wait_all();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Stop CLI monitor before final report output
    monitor.stop();

    // Clear screen and print final completion message
    std::cout << "\n\nWorkload completed successfully.\n";

    // =========================================================================
    // STAGE 3: Automatic Post-Mortem Audit & Leak Diagnostics
    // =========================================================================
    tracker.dump_leak_report(std::cout);

    std::cout << "[INFO] Notice how DevInsight pinpointed the exact line numbers\n";
    std::cout << "       for both the 128KB buffer and the Particle leak above!\n";

    return 0;
}
