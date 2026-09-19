cmake_minimum_required(VERSION 3.16)
project(DevInsight VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
if (MSVC)
    add_compile_options(/W4 /EHsc /O2)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -O2 -pthread)
endif()

# Core static library
add_library(devinsight_core STATIC
    src/memory_tracker.cpp
    src/scheduler.cpp
    src/cli_monitor.cpp
)

target_include_directories(devinsight_core PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Demo executable
add_executable(devinsight_demo examples/demo_workload.cpp)
target_link_libraries(devinsight_demo PRIVATE devinsight_core)

# Tests
enable_testing()

add_executable(test_memory tests/test_memory.cpp)
target_link_libraries(test_memory PRIVATE devinsight_core)
add_test(NAME TestMemory COMMAND test_memory)

add_executable(test_scheduler tests/test_scheduler.cpp)
target_link_libraries(test_scheduler PRIVATE devinsight_core)
add_test(NAME TestScheduler COMMAND test_scheduler)
