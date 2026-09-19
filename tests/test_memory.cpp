#include "devinsight/memory_tracker.hpp"
#include <cassert>
#include <iostream>
#include <sstream>

void test_basic_allocation_tracking() {
    auto& tracker = devinsight::MemoryTracker::instance();
    tracker.reset();

    void* ptr1 = DEV_ALLOC(1024);
    assert(ptr1 != nullptr);
    assert(tracker.is_tracked(ptr1));

    auto stats = tracker.get_stats();
    assert(stats.active_bytes == 1024);
    assert(stats.peak_bytes == 1024);
    assert(stats.active_allocations == 1);
    assert(stats.total_allocations == 1);

    void* ptr2 = DEV_ALLOC(2048);
    assert(ptr2 != nullptr);
    assert(tracker.is_tracked(ptr2));

    stats = tracker.get_stats();
    assert(stats.active_bytes == 3072);
    assert(stats.peak_bytes == 3072);
    assert(stats.active_allocations == 2);

    DEV_FREE(ptr1);
    assert(!tracker.is_tracked(ptr1));
    stats = tracker.get_stats();
    assert(stats.active_bytes == 2048);
    assert(stats.peak_bytes == 3072); // peak remains 3072
    assert(stats.active_allocations == 1);

    DEV_FREE(ptr2);
    assert(!tracker.is_tracked(ptr2));
    stats = tracker.get_stats();
    assert(stats.active_bytes == 0);
    assert(stats.active_allocations == 0);

    std::cout << "[TEST PASSED] test_basic_allocation_tracking\n";
}

void test_typed_object_allocation() {
    struct TestNode {
        int x;
        double y;
        TestNode(int a, double b) : x(a), y(b) {}
    };

    auto& tracker = devinsight::MemoryTracker::instance();
    tracker.reset();

    TestNode* node = DEV_NEW(TestNode, 42, 3.14159);
    assert(node != nullptr);
    assert(node->x == 42);
    assert(node->y == 3.14159);
    assert(tracker.is_tracked(node));

    auto stats = tracker.get_stats();
    assert(stats.active_bytes == sizeof(TestNode));

    DEV_DELETE(node);
    stats = tracker.get_stats();
    assert(stats.active_bytes == 0);

    std::cout << "[TEST PASSED] test_typed_object_allocation\n";
}

void test_leak_detection_report() {
    auto& tracker = devinsight::MemoryTracker::instance();
    tracker.reset();

    void* intentional_leak = DEV_ALLOC(512);
    (void)intentional_leak; // Deliberately leave unfreed

    std::ostringstream ss;
    tracker.dump_leak_report(ss);
    std::string report = ss.str();

    assert(report.find("CRITICAL: 1 UNFREED MEMORY LEAK(S) DETECTED!") != std::string::npos);
    assert(report.find("512") != std::string::npos);

    // Clean up
    DEV_FREE(intentional_leak);
    std::cout << "[TEST PASSED] test_leak_detection_report\n";
}

int main() {
    std::cout << ">>> Running DevInsight Memory Tracker Unit Tests <<<\n";
    test_basic_allocation_tracking();
    test_typed_object_allocation();
    test_leak_detection_report();
    std::cout << ">>> ALL MEMORY TRACKER TESTS PASSED SUCCESSFULLY! <<<\n";
    return 0;
}
