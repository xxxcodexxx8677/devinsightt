#include "devinsight/scheduler.hpp"
#include <iostream>

namespace devinsight {

TaskScheduler::TaskScheduler(size_t num_workers) {
    if (num_workers == 0) {
        num_workers = std::thread::hardware_concurrency();
        if (num_workers == 0) num_workers = 4;
    }
    worker_count_ = num_workers;

    workers_.reserve(worker_count_);
    for (size_t i = 0; i < worker_count_; ++i) {
        workers_.emplace_back(&TaskScheduler::worker_loop, this, i);
    }
}

TaskScheduler::~TaskScheduler() {
    shutdown();
}

uint64_t TaskScheduler::submit(std::function<void()> task, TaskPriority priority) {
    if (!is_running_.load()) {
        throw std::runtime_error("Cannot submit to a stopped TaskScheduler");
    }

    uint64_t id = next_task_id_++;
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        task_queue_.push(TaskItem{
            id,
            priority,
            std::move(task),
            std::chrono::steady_clock::now()
        });
    }
    cv_task_available_.notify_one();
    return id;
}

void TaskScheduler::worker_loop(size_t /*worker_id*/) {
    while (true) {
        TaskItem item;
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            cv_task_available_.wait(lock, [this]() {
                return !is_running_.load() || !task_queue_.empty();
            });

            if (!is_running_.load() && task_queue_.empty()) {
                return;
            }

            // In std::priority_queue, top() retrieves highest-priority element
            item = std::move(const_cast<TaskItem&>(task_queue_.top()));
            task_queue_.pop();
            active_workers_++;
        }

        auto start_exec = std::chrono::steady_clock::now();
        auto wait_duration = std::chrono::duration_cast<std::chrono::microseconds>(
            start_exec - item.queued_time);

        // Execute task
        try {
            if (item.func) {
                item.func();
            }
        } catch (const std::exception& e) {
            std::cerr << "[TaskScheduler] Uncaught exception in task " 
                      << item.id << ": " << e.what() << "\n";
        } catch (...) {
            std::cerr << "[TaskScheduler] Unknown exception in task " 
                      << item.id << "\n";
        }

        auto end_exec = std::chrono::steady_clock::now();
        auto exec_duration = std::chrono::duration_cast<std::chrono::microseconds>(
            end_exec - start_exec);

        {
            std::lock_guard<std::mutex> lock(metrics_mutex_);
            total_wait_us_ += wait_duration.count();
            total_exec_us_ += exec_duration.count();
        }

        completed_tasks_++;
        active_workers_--;

        // Notify barrier if queue is empty and all workers done
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            if (task_queue_.empty() && active_workers_.load() == 0) {
                cv_all_done_.notify_all();
            }
        }
    }
}

void TaskScheduler::wait_all() {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    cv_all_done_.wait(lock, [this]() {
        return task_queue_.empty() && active_workers_.load() == 0;
    });
}

void TaskScheduler::shutdown() {
    bool expected = true;
    if (is_running_.compare_exchange_strong(expected, false)) {
        cv_task_available_.notify_all();
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
        workers_.clear();
    }
}

SchedulerStats TaskScheduler::get_stats() const {
    SchedulerStats stats;
    stats.worker_count = worker_count_;
    stats.active_workers = active_workers_.load();
    stats.completed_tasks = completed_tasks_.load();

    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        stats.pending_tasks = task_queue_.size();
    }

    size_t completed = stats.completed_tasks;
    if (completed > 0) {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        stats.avg_wait_us = static_cast<double>(total_wait_us_) / completed;
        stats.avg_exec_us = static_cast<double>(total_exec_us_) / completed;
    }

    return stats;
}

} // namespace devinsight
