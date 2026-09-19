#include "devinsight/memory_tracker.hpp"
#include <cstdlib>
#include <iomanip>
#include <sstream>

namespace devinsight {

thread_local bool MemoryTracker::is_internal_op_ = false;

alignas(MemoryTracker) static char g_tracker_storage[sizeof(MemoryTracker)];
static std::atomic<bool> g_tracker_ready{false};
static std::mutex g_init_mutex;

MemoryTracker::MemoryTracker() = default;

MemoryTracker& MemoryTracker::instance() {
    if (!g_tracker_ready.load(std::memory_order_acquire)) {
        std::lock_guard<std::mutex> lock(g_init_mutex);
        if (!g_tracker_ready.load(std::memory_order_relaxed)) {
            is_internal_op_ = true;
            new (g_tracker_storage) MemoryTracker();
            is_internal_op_ = false;
            g_tracker_ready.store(true, std::memory_order_release);
        }
    }
    return *reinterpret_cast<MemoryTracker*>(g_tracker_storage);
}

bool MemoryTracker::is_ready() {
    return g_tracker_ready.load(std::memory_order_relaxed);
}

bool MemoryTracker::is_internal_active() {
    return is_internal_op_;
}

void MemoryTracker::set_internal_active(bool active) {
    is_internal_op_ = active;
}

void* MemoryTracker::track_allocate(size_t size, const char* file, int line) {
    if (size == 0) size = 1;

    void* ptr = std::malloc(size);
    if (!ptr) return nullptr;

    if (is_internal_op_) {
        return ptr;
    }

    is_internal_op_ = true;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        AllocationRecord record;
        record.address = ptr;
        record.size = size;
        record.file = file;
        record.line = line;
        record.timestamp = std::chrono::system_clock::now();

        registry_[ptr] = record;

        stats_.active_bytes += size;
        stats_.total_allocated_bytes += size;
        stats_.active_allocations++;
        stats_.total_allocations++;

        if (stats_.active_bytes > stats_.peak_bytes) {
            stats_.peak_bytes = stats_.active_bytes;
        }
    }
    is_internal_op_ = false;

    return ptr;
}

void MemoryTracker::track_deallocate(void* ptr) {
    if (!ptr) return;

    if (is_internal_op_) {
        std::free(ptr);
        return;
    }

    is_internal_op_ = true;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = registry_.find(ptr);
        if (it != registry_.end()) {
            stats_.active_bytes -= it->second.size;
            stats_.active_allocations--;
            stats_.total_deallocations++;
            registry_.erase(it);
        }
    }
    is_internal_op_ = false;

    std::free(ptr);
}

MemoryStats MemoryTracker::get_stats() const {
    bool prev = is_internal_op_;
    is_internal_op_ = true;
    MemoryStats snapshot;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        snapshot = stats_;
    }
    is_internal_op_ = prev;
    return snapshot;
}

bool MemoryTracker::is_tracked(void* ptr) const {
    bool prev = is_internal_op_;
    is_internal_op_ = true;
    bool tracked = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        tracked = (registry_.find(ptr) != registry_.end());
    }
    is_internal_op_ = prev;
    return tracked;
}

void MemoryTracker::reset() {
    bool prev = is_internal_op_;
    is_internal_op_ = true;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        registry_.clear();
        stats_ = MemoryStats{};
    }
    is_internal_op_ = prev;
}

void MemoryTracker::dump_leak_report(std::ostream& out) const {
    bool prev = is_internal_op_;
    is_internal_op_ = true; // Protect all stream formatting allocations from entering the tracker

    // Take snapshot of registry and stats to minimize lock hold time
    std::vector<AllocationRecord> leaked_records;
    MemoryStats current_stats;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        current_stats = stats_;
        leaked_records.reserve(registry_.size());
        for (const auto& pair : registry_) {
            leaked_records.push_back(pair.second);
        }
    }

    out << "\n======================================================================\n";
    out << "                 DEVINSIGHT MEMORY AUDIT REPORT                       \n";
    out << "======================================================================\n";
    out << std::left << std::setw(28) << "Total Allocations" 
        << ": " << current_stats.total_allocations << "\n";
    out << std::left << std::setw(28) << "Total Deallocations" 
        << ": " << current_stats.total_deallocations << "\n";
    out << std::left << std::setw(28) << "Total Bytes Allocated" 
        << ": " << current_stats.total_allocated_bytes << " bytes (" 
        << std::fixed << std::setprecision(2) << (current_stats.total_allocated_bytes / 1024.0) << " KB)\n";
    out << std::left << std::setw(28) << "Peak Heap Usage" 
        << ": " << current_stats.peak_bytes << " bytes (" 
        << std::fixed << std::setprecision(2) << (current_stats.peak_bytes / 1024.0) << " KB)\n";
    out << std::left << std::setw(28) << "Unfreed Allocations" 
        << ": " << leaked_records.size() << " block(s)\n";
    out << std::left << std::setw(28) << "Unfreed Bytes (Leaks)" 
        << ": " << current_stats.active_bytes << " bytes\n";
    out << "----------------------------------------------------------------------\n";

    if (leaked_records.empty()) {
        out << "[PASSED] Clean execution: 0 memory leaks detected!\n";
    } else {
        out << "[FAILED] CRITICAL: " << leaked_records.size() << " UNFREED MEMORY LEAK(S) DETECTED!\n\n";
        out << std::left 
            << std::setw(18) << "Memory Address"
            << std::setw(14) << "Bytes"
            << "Source Location\n";
        out << "----------------------------------------------------------------------\n";

        for (const auto& rec : leaked_records) {
            std::ostringstream loc;
            if (rec.file) {
                loc << rec.file << ":" << rec.line;
            } else {
                loc << "unknown";
            }

            out << std::left 
                << std::setw(18) << rec.address
                << std::setw(14) << rec.size
                << loc.str() << "\n";
        }
    }
    out << "======================================================================\n\n";

    is_internal_op_ = prev;
}

} // namespace devinsight
