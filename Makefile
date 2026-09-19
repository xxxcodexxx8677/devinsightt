CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2 -pthread -Iinclude

SRC = src/memory_tracker.cpp src/scheduler.cpp src/cli_monitor.cpp
OBJ = $(SRC:.cpp=.o)

all: bin/devinsight_demo bin/test_memory bin/test_scheduler

bin:
	mkdir -p bin

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

bin/devinsight_demo: $(OBJ) examples/demo_workload.cpp | bin
	$(CXX) $(CXXFLAGS) examples/demo_workload.cpp $(OBJ) -o $@

bin/test_memory: $(OBJ) tests/test_memory.cpp | bin
	$(CXX) $(CXXFLAGS) tests/test_memory.cpp $(OBJ) -o $@

bin/test_scheduler: $(OBJ) tests/test_scheduler.cpp | bin
	$(CXX) $(CXXFLAGS) tests/test_scheduler.cpp $(OBJ) -o $@

test: bin/test_memory bin/test_scheduler
	./bin/test_memory
	./bin/test_scheduler

clean:
	rm -f src/*.o bin/*

.PHONY: all test clean
