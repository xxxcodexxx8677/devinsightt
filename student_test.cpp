#include "devinsight/memory_tracker.hpp"
#include <iostream>

int main() {
    std::cout << "======================================================\n";
    std::cout << "               MY FIRST STUDENT TEST                  \n";
    std::cout << "======================================================\n";

    // 1. Allocate an array of 500 integers (2,000 bytes) on the heap
    std::cout << "Allocating integer array (2,000 bytes)...\n";
    int* my_array = (int*)DEV_ALLOC(500 * sizeof(int));
    my_array[0] = 42;

    // 2. Allocate a small 128-byte string buffer
    std::cout << "Allocating string buffer (128 bytes)...\n";
    char* my_string = (char*)DEV_ALLOC(128);

    // 3. We properly free my_string:
    std::cout << "Freeing string buffer...\n";
    DEV_FREE(my_string);

    // 4. BUT we intentionally forget to free my_array!
    DEV_FREE(my_array);
    // (DevInsight will catch this mistake below!)

    std::cout << "\nRunning DevInsight Memory Audit Report...\n";
    devinsight::MemoryTracker::instance().dump_leak_report();

    std::cout << "Notice how DevInsight points right to the line where my_array was created!\n";
    std::cout << "======================================================\n";

    return 0;
}
