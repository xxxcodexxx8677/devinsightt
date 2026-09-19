#include "devinsight/scheduler.hpp"
#include <cassert>
#include <iostream>
#include <atomic>
#include <vector>

void test_scheduler_task_execution() {
    devinsight::TaskScheduler scheduler(4);

    std::atomic<int> counter{0};
    const int num_tasks = 1000;

    for (int i = 0; i < num_tasks; ++i) {
        scheduler.submit([&counter]() {
            counter++;
        });
    }

    scheduler.wait_all();
    assert(counter.load() == num_tasks);

    auto stats = scheduler.get_stats();
    assert(stats.completed_tasks == num_tasks);
    assert(stats.pending_tasks == 0);
    assert(stats.active_workers == 0);

    std::cout << "[TEST PASSED] test_scheduler_task_execution (" << num_tasks << " tasks)\n";
}

void test_high_concurrency_stress() {
    devinsight::TaskScheduler scheduler(8);

    const int task_count = 5000;
    std::atomic<int64_t> sum{0};

    for (int i = 1; i <= task_count; ++i) {
        scheduler.submit([&sum, i]() {
            sum.fetch_add(i);
        });
    }

    scheduler.wait_all();

    int64_t expected = (static_cast<int64_t>(task_count) * (task_count + 1)) / 2;
    assert(sum.load() == expected);

    auto stats = scheduler.get_stats();
    assert(stats.completed_tasks == task_count);

    std::cout << "[TEST PASSED] test_high_concurrency_stress (" << task_count << " tasks, sum = " << sum.load() << ")\n";
}

int main() {
    std::cout << ">>> Running DevInsight Task Scheduler Unit Tests <<<\n";
    test_scheduler_task_execution();
    test_high_concurrency_stress();
    std::cout << ">>> ALL SCHEDULER TESTS PASSED SUCCESSFULLY! <<<\n";
    return 0;
}
