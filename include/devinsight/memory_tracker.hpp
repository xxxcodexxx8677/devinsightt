#ifndef DEVINSIGHT_MEMORY_TRACKER_HPP
#define DEVINSIGHT_MEMORY_TRACKER_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <vector>
#include <iostream>
#include <atomic>

namespace devinsight {

struct AllocationRecord {
    void* address{nullptr};
    size_t size{0};
    const char* file{nullptr};
    int line{0};
    std::chrono::system_clock::time_point timestamp;
};

struct MemoryStats {
    size_t active_bytes{0};
    size_t peak_bytes{0};
    size_t total_allocated_bytes{0};
    size_t active_allocations{0};
    size_t total_allocations{0};
    size_t total_deallocations{0};
};

class MemoryTracker {
public:
    static MemoryTracker& instance();
    static bool is_ready();
    static bool is_internal_active();
    static void set_internal_active(bool active);

    // Allocation hooks
    void* track_allocate(size_t size, const char* file, int line);
    void track_deallocate(void* ptr);

    // Snapshot of current system state
    MemoryStats get_stats() const;

    // Report generation
    void dump_leak_report(std::ostream& out = std::cout) const;

    // Reset statistics (primarily for testing)
    void reset();

    // Query specific allocation
    bool is_tracked(void* ptr) const;

    MemoryTracker();
    ~MemoryTracker() = default;

private:
    MemoryTracker(const MemoryTracker&) = delete;
    MemoryTracker& operator=(const MemoryTracker&) = delete;

    mutable std::mutex mutex_;
    std::unordered_map<void*, AllocationRecord> registry_;
    MemoryStats stats_;

    // Thread-local reentrancy guard
    static thread_local bool is_internal_op_;
};

} // namespace devinsight

// Tracking macros for user applications
#define DEV_ALLOC(size) \
    devinsight::MemoryTracker::instance().track_allocate((size), __FILE__, __LINE__)

#define DEV_FREE(ptr) \
    devinsight::MemoryTracker::instance().track_deallocate(ptr)

// Helper template for typed allocation
template <typename T, typename... Args>
T* dev_create(const char* file, int line, Args&&... args) {
    void* mem = devinsight::MemoryTracker::instance().track_allocate(sizeof(T), file, line);
    if (!mem) return nullptr;
    return new (mem) T(std::forward<Args>(args)...);
}

template <typename T>
void dev_destroy(T* ptr) {
    if (!ptr) return;
    ptr->~T();
    devinsight::MemoryTracker::instance().track_deallocate(static_cast<void*>(ptr));
}

#define DEV_NEW(Type, ...) \
    dev_create<Type>(__FILE__, __LINE__, ##__VA_ARGS__)

#define DEV_DELETE(ptr) \
    dev_destroy(ptr)

#endif // DEVINSIGHT_MEMORY_TRACKER_HPP
