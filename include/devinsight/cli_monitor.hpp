#ifndef DEVINSIGHT_CLI_MONITOR_HPP
#define DEVINSIGHT_CLI_MONITOR_HPP

#include "devinsight/memory_tracker.hpp"
#include "devinsight/scheduler.hpp"
#include <thread>
#include <atomic>
#include <chrono>

namespace devinsight {

class CliMonitor {
public:
    CliMonitor(const MemoryTracker& mem_tracker, 
               const TaskScheduler& scheduler,
               std::chrono::milliseconds refresh_rate = std::chrono::milliseconds(200));
    ~CliMonitor();

    // Start background display loop
    void start();

    // Stop background display loop
    void stop();

    // Single instantaneous frame render to console
    void render_frame() const;

private:
    void monitor_loop();

    const MemoryTracker& mem_tracker_;
    const TaskScheduler& scheduler_;
    std::chrono::milliseconds refresh_rate_;
    std::atomic<bool> is_active_{false};
    std::thread monitor_thread_;
};

} // namespace devinsight

#endif // DEVINSIGHT_CLI_MONITOR_HPP
