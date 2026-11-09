CXX ?= g++
CXXFLAGS := -std=c++20 -Wall -Wextra -Wpedantic -O0 -g0
BUILD_DIR := build
COMMON := matrix.cpp benchmark_support.cpp
VENV := $(abspath ../../.venv)
PYTHON := $(VENV)/bin/python

.PHONY: all main test benchmark clean

all: test

main: $(BUILD_DIR)/matrix_app
	./$(BUILD_DIR)/matrix_app

test: $(BUILD_DIR)/matrix_tests
	./$(BUILD_DIR)/matrix_tests

benchmark: $(BUILD_DIR)/matrix_benchmark
	./$(BUILD_DIR)/matrix_benchmark 10 100 10 10
	$(PYTHON) plot_results.py

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/matrix_app: $(COMMON) main.cpp matrix.hpp benchmark_support.hpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(COMMON) main.cpp -o $@

$(BUILD_DIR)/matrix_tests: $(COMMON) test.cpp matrix.hpp benchmark_support.hpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(COMMON) test.cpp -o $@

$(BUILD_DIR)/matrix_benchmark: $(COMMON) benchmark.cpp matrix.hpp benchmark_support.hpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(COMMON) benchmark.cpp -o $@

clean:
	rm -rf $(BUILD_DIR) data plots
