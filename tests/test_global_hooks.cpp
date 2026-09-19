#include "devinsight/global_hooks.hpp"
#include <cassert>
#include <iostream>
#include <sstream>

void test_standard_new_delete_interception() {
    auto& tracker = devinsight::MemoryTracker::instance();
    tracker.reset();

    // Standard C++ new without any macros!
    int* num = new int(42);
    assert(num != nullptr);
    assert(*num == 42);
    assert(tracker.is_tracked(num));

    auto stats = tracker.get_stats();
    assert(stats.active_bytes >= sizeof(int));
    assert(stats.active_allocations >= 1);

    // Standard array new[]
    char* buffer = new char[256];
    assert(tracker.is_tracked(buffer));

    delete num;
    delete[] buffer;

    std::cout << "[TEST PASSED] test_standard_new_delete_interception\n";
}

void test_unfreed_standard_new_leak() {
    auto& tracker = devinsight::MemoryTracker::instance();
    tracker.reset();

    // Intentional leak with standard new
    double* leak = new double(2.718);
    assert(tracker.is_tracked(leak));

    // Check stats and report
    auto stats = tracker.get_stats();
    assert(stats.active_allocations >= 1);

    std::ostringstream ss;
    tracker.dump_leak_report(ss);
    std::string report = ss.str();

    assert(report.find("UNFREED MEMORY LEAK(S) DETECTED!") != std::string::npos);
    assert(report.find("[global new]") != std::string::npos);

    delete leak; // clean up
    std::cout << "[TEST PASSED] test_unfreed_standard_new_leak\n";
}

int main() {
    std::cout << ">>> Running DevInsight Global Hook Interception Tests <<<\n";
    test_standard_new_delete_interception();
    test_unfreed_standard_new_leak();
    std::cout << ">>> ALL GLOBAL HOOK TESTS PASSED SUCCESSFULLY! <<<\n";
    return 0;
}
