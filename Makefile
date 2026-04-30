CXX 			:= g++
SRC_DIR 		:= benchmarks
SRCS 			:= $(wildcard $(SRC_DIR)/*.cpp)

DEBUG_DIR 		:= debug
RELEASE_DIR 	:= release
DEBUG_OUT 		:= $(DEBUG_DIR)/bench
RELEASE_OUT 	:= $(RELEASE_DIR)/bench

DEBUG_FLAGS 	:= -std=c++20 -Wall -Wextra -g -O0 -DDEBUG
RELEASE_FLAGS 	:= -std=c++20 -Wall -Wextra -O2 -DNDEBUG

BENCHMARK_FLAGS	:= $(shell pkg-config --cflags --libs benchmark)

.PHONY: all debug release clean run-debug run-release

all: debug release

debug: | $(DEBUG_DIR)/
	$(CXX) $(DEBUG_FLAGS) $(SRCS) $(BENCHMARK_FLAGS) -o $(DEBUG_OUT)

release: | $(RELEASE_DIR)/
	$(CXX) $(RELEASE_FLAGS) $(SRCS) $(BENCHMARK_FLAGS) -o $(RELEASE_OUT)

$(DEBUG_DIR)/:
	mkdir -p $@

$(RELEASE_DIR)/:
	mkdir -p $@

run-debug: debug
	./$(DEBUG_OUT)

run-release: release
	./$(RELEASE_OUT)

clean:
	rm -rf $(DEBUG_DIR) $(RELEASE_DIR)

