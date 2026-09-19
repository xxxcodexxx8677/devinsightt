#include "devinsight/memory_tracker.hpp"
#include "devinsight/scheduler.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <cstring>

// Simulating a real web API request packet
struct HttpRequest {
    int request_id;
    std::string endpoint;
    size_t payload_size;
};

// Route 1: Clean handler (Correctly allocates and frees request buffers)
void handle_get_profile(int request_id) {
    // Allocate temporary cache buffer for user profile
    char* profile_cache = (char*)DEV_ALLOC(2048); // 2 KB buffer
    std::snprintf(profile_cache, 2048, "User data for request #%d", request_id);

    // Simulate database query delay
    std::this_thread::sleep_for(std::chrono::microseconds(300));

    // Developer correctly frees the memory
    DEV_FREE(profile_cache);
}

// Route 2: Buggy handler (Developer forgot to free image buffer on certain error paths!)
void handle_upload_image(int request_id) {
    // Allocate image processing buffer
    size_t image_size = 64 * 1024; // 64 KB image
    char* image_buffer = (char*)DEV_ALLOC(image_size);

    // Simulate image processing
    std::memset(image_buffer, 0xAB, image_size);
    std::this_thread::sleep_for(std::chrono::microseconds(500));

    // REAL BUG: In request #42 and #77, an early return or missed free occurred!
    if (request_id == 42 || request_id == 77) {
        // Developer forgot DEV_FREE(image_buffer)!
        return; 
    }

    // Normal path frees memory
    DEV_FREE(image_buffer);
}

int main() {
    std::cout << "======================================================================\n";
    std::cout << "  REAL-WORLD USE CASE: High-Throughput Web API Server Simulation     \n";
    std::cout << "======================================================================\n";
    std::cout << "Starting server with 4 worker threads...\n";

    devinsight::TaskScheduler server_workers(4);

    const int incoming_requests = 100;
    std::cout << "Dispatching " << incoming_requests << " concurrent HTTP requests...\n\n";

    // Simulate client requests coming in from network
    for (int req_id = 1; req_id <= incoming_requests; ++req_id) {
        if (req_id % 3 == 0) {
            // Image upload endpoint
            server_workers.submit([req_id]() {
                handle_upload_image(req_id);
            }, devinsight::TaskPriority::NORMAL);
        } else {
            // Profile read endpoint
            server_workers.submit([req_id]() {
                handle_get_profile(req_id);
            }, devinsight::TaskPriority::HIGH);
        }
    }

    // Wait for all 100 requests to be processed by worker threads
    server_workers.wait_all();
    std::cout << "[SERVER] All 100 requests handled across worker threads.\n";
    std::cout << "[SERVER] Running End-of-Day DevInsight Memory Health Audit...\n";

    // Audit the server memory health
    devinsight::MemoryTracker::instance().dump_leak_report();

    std::cout << "Notice how out of 100 requests, DevInsight caught the EXACT requests\n";
    std::cout << "and line number in handle_upload_image() where memory was leaked!\n";
    std::cout << "======================================================================\n";

    return 0;
}
