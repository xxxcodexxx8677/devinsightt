#ifndef DEVINSIGHT_SCHEDULER_HPP
#define DEVINSIGHT_SCHEDULER_HPP

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <chrono>
#include <memory>

namespace devinsight {

enum class TaskPriority : int {
    LOW = 1,
    NORMAL = 2,
    HIGH = 3,
    CRITICAL = 4
};

struct TaskMetrics {
    uint64_t task_id{0};
    TaskPriority priority{TaskPriority::NORMAL};
    std::chrono::microseconds wait_duration{0};
    std::chrono::microseconds exec_duration{0};
};

struct SchedulerStats {
    size_t worker_count{0};
    size_t pending_tasks{0};
    size_t active_workers{0};
    size_t completed_tasks{0};
    double avg_wait_us{0.0};
    double avg_exec_us{0.0};
};

class TaskScheduler {
public:
    explicit TaskScheduler(size_t num_workers = 0);
    ~TaskScheduler();

    TaskScheduler(const TaskScheduler&) = delete;
    TaskScheduler& operator=(const TaskScheduler&) = delete;

    // Submit a callable with optional priority (default = NORMAL)
    uint64_t submit(std::function<void()> task, TaskPriority priority = TaskPriority::NORMAL);

    // Wait until all submitted tasks are completed
    void wait_all();

    // Stop accepting new tasks and gracefully join worker threads
    void shutdown();

    // Fetch snapshot of metrics
    SchedulerStats get_stats() const;

private:
    struct TaskItem {
        uint64_t id{0};
        TaskPriority priority{TaskPriority::NORMAL};
        std::function<void()> func;
        std::chrono::steady_clock::time_point queued_time;

        // Custom comparator: higher priority first; if equal, FIFO order
        bool operator<(const TaskItem& other) const {
            if (static_cast<int>(priority) == static_cast<int>(other.priority)) {
                return queued_time > other.queued_time; // earlier timestamp has precedence
            }
            return static_cast<int>(priority) < static_cast<int>(other.priority);
        }
    };

    void worker_loop(size_t worker_id);

    size_t worker_count_{0};
    std::vector<std::thread> workers_;
    std::priority_queue<TaskItem> task_queue_;

    mutable std::mutex queue_mutex_;
    std::condition_variable cv_task_available_;
    std::condition_variable cv_all_done_;

    std::atomic<bool> is_running_{true};
    std::atomic<uint64_t> next_task_id_{1};
    std::atomic<size_t> active_workers_{0};
    std::atomic<size_t> completed_tasks_{0};

    // Cumulative timing metrics
    mutable std::mutex metrics_mutex_;
    uint64_t total_wait_us_{0};
    uint64_t total_exec_us_{0};
};

} // namespace devinsight

#endif // DEVINSIGHT_SCHEDULER_HPP
