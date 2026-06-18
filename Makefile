CXX 			:= g++
BENCH_DIR		:= benchmarks
TEST_DIR		:= tests
BENCH_SRCS 		:= $(wildcard $(BENCH_DIR)/*.cpp)
TEST_SRCS		:= $(wildcard $(TEST_DIR)/*.cpp)

DEBUG_DIR 		:= debug
RELEASE_DIR 	:= release

DEBUG_FLAGS 	:= -std=c++20 -Wall -Wextra -g -O0 -DDEBUG
RELEASE_FLAGS 	:= -std=c++20 -Wall -Wextra -O2 -DNDEBUG

BENCHMARK_FLAGS	:= $(shell pkg-config --cflags --libs benchmark)

.PHONY: all debug release clean run-debug run-release

all: debug release

debug:
	mkdir -p $(DEBUG_DIR)
	$(CXX) $(DEBUG_FLAGS) $(BENCH_SRCS) $(BENCHMARK_FLAGS) -o $(DEBUG_DIR)/bench
	$(CXX) $(DEBUG_FLAGS) $(TEST_SRCS) -o $(DEBUG_DIR)/tests

release:
	mkdir -p $(RELEASE_DIR)
	$(CXX) $(RELEASE_FLAGS) $(BENCH_SRCS) $(BENCHMARK_FLAGS) -o $(RELEASE_DIR)/bench
	$(CXX) $(RELEASE_FLAGS) -UNDEBUG $(TEST_SRCS) -o $(RELEASE_DIR)/tests

run-debug: debug
	./$(DEBUG_DIR)/bench

run-release: release
	./$(RELEASE_DIR)/bench

run-debug-tests: debug
	./$(DEBUG_DIR)/tests

run-release-tests: release
	./$(RELEASE_DIR)/tests

clean:
	rm -rf $(DEBUG_DIR) $(RELEASE_DIR)

