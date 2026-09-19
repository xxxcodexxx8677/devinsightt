#include "devinsight/global_hooks.hpp"
#include <new>
#include <cstdlib>
#include <atomic>

namespace devinsight {

static std::atomic<bool> g_hooks_active{true};

void set_global_hooks_enabled(bool enabled) {
    g_hooks_active.store(enabled);
}

bool is_global_hooks_enabled() {
    return g_hooks_active.load();
}

} // namespace devinsight

void* operator new(std::size_t size) {
    if (!devinsight::MemoryTracker::is_ready() || 
        devinsight::MemoryTracker::is_internal_active() || 
        !devinsight::is_global_hooks_enabled()) {
        void* ptr = std::malloc(size ? size : 1);
        if (!ptr) throw std::bad_alloc();
        return ptr;
    }

    void* ptr = devinsight::MemoryTracker::instance().track_allocate(size, "[global new]", 0);
    if (!ptr) throw std::bad_alloc();
    return ptr;
}

void* operator new[](std::size_t size) {
    if (!devinsight::MemoryTracker::is_ready() || 
        devinsight::MemoryTracker::is_internal_active() || 
        !devinsight::is_global_hooks_enabled()) {
        void* ptr = std::malloc(size ? size : 1);
        if (!ptr) throw std::bad_alloc();
        return ptr;
    }

    void* ptr = devinsight::MemoryTracker::instance().track_allocate(size, "[global new[]]", 0);
    if (!ptr) throw std::bad_alloc();
    return ptr;
}

void operator delete(void* ptr) noexcept {
    if (!ptr) return;

    if (!devinsight::MemoryTracker::is_ready() || 
        devinsight::MemoryTracker::is_internal_active() || 
        !devinsight::is_global_hooks_enabled()) {
        std::free(ptr);
        return;
    }

    devinsight::MemoryTracker::instance().track_deallocate(ptr);
}

void operator delete[](void* ptr) noexcept {
    if (!ptr) return;

    if (!devinsight::MemoryTracker::is_ready() || 
        devinsight::MemoryTracker::is_internal_active() || 
        !devinsight::is_global_hooks_enabled()) {
        std::free(ptr);
        return;
    }

    devinsight::MemoryTracker::instance().track_deallocate(ptr);
}

void operator delete(void* ptr, std::size_t /*size*/) noexcept {
    ::operator delete(ptr);
}

void operator delete[](void* ptr, std::size_t /*size*/) noexcept {
    ::operator delete[](ptr);
}
