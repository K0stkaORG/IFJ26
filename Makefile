# Compiler configuration. Command line wins, e.g. `make CC=gcc`.
CC = clang
CSTD ?= c17

CFLAGS_COMMON := -std=$(CSTD) -Wall -Wextra -pedantic -MMD -MP
CFLAGS_RELEASE := $(CFLAGS_COMMON) -O3
CFLAGS_DEBUG := $(CFLAGS_COMMON) -g -O0

# Target binary name (with .exe extension on Windows)
RELEASE_BIN := ifj26
ifeq ($(OS),Windows_NT)
RELEASE_BIN := $(RELEASE_BIN).exe
endif

# Source and build directories
SRC_DIR := src
BUILD_DIR := build
RELEASE_BUILD_DIR := $(BUILD_DIR)/release
DEBUG_BUILD_DIR := $(BUILD_DIR)/debug

# All source files
SRCS := $(wildcard $(SRC_DIR)/*.c)

# Release build objects and dependencies
RELEASE_OBJS := $(patsubst $(SRC_DIR)/%.c,$(RELEASE_BUILD_DIR)/%.o,$(SRCS))
RELEASE_DEPS := $(RELEASE_OBJS:.o=.d)

# Debug build binary, objects and dependencies
DEBUG_BIN := $(DEBUG_BUILD_DIR)/$(notdir $(RELEASE_BIN))
DEBUG_OBJS := $(patsubst $(SRC_DIR)/%.c,$(DEBUG_BUILD_DIR)/%.o,$(SRCS))
DEBUG_DEPS := $(DEBUG_OBJS:.o=.d)

# Non-filename targets
.PHONY: all debug test test-unit test-e2e benchmark clean

# Default target
all: $(RELEASE_BIN)

# Build the release binary
$(RELEASE_BIN): $(RELEASE_OBJS)
	$(CC) $(CFLAGS_RELEASE) -o $@ $^

# Build the debug binary
debug: $(DEBUG_BIN)
$(DEBUG_BIN): $(DEBUG_OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS_DEBUG) -o $@ $^

# Build individual object files (release)
$(RELEASE_BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(RELEASE_BUILD_DIR)
	$(CC) $(CFLAGS_RELEASE) -c -o $@ $<

# Build individual object files (debug)
$(DEBUG_BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(DEBUG_BUILD_DIR)
	$(CC) $(CFLAGS_DEBUG) -c -o $@ $<

# Ensure the per-profile build directories exist
$(RELEASE_BUILD_DIR) $(DEBUG_BUILD_DIR): | $(BUILD_DIR)
	@-cd $(BUILD_DIR) && mkdir release debug

# Ensure the main build directory exists
$(BUILD_DIR):
	@-mkdir $(BUILD_DIR)

# Run unit tests
test-unit: debug
	echo "Not implemented"

# Run end-to-end tests
test-e2e: debug
	echo "Not implemented"

# Run all tests
test: test-unit test-e2e

# Run benchmarks
benchmark:
	echo "Not implemented"

# Clean up build artifacts
clean:
ifeq ($(OS),Windows_NT)
	@-if exist $(BUILD_DIR) rmdir /S /Q $(BUILD_DIR)
	@-if exist $(RELEASE_BIN) del $(RELEASE_BIN)
else
	@-rm -rf $(BUILD_DIR) $(RELEASE_BIN)
endif

-include $(RELEASE_DEPS) $(DEBUG_DEPS)
