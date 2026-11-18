################################################################################
# Makefile for the IFJ26 project                                               #
################################################################################
# Author: Jan Kostka (xkostkj00)                                               #
# Date: 8. 10. 2026                                                            #
################################################################################
# Works on: Windows, Linux and MacOS.                                          #
# Uses clang, but should also work with gcc.                                   #
# For tests, make sure you have python installed.                              #
################################################################################
# Commands:                                                                    #
#   make                        - build the project                            #
#   make debug                  - build with debug symbols                     #
#   make test                   - run all tests                                #
#   make test-unit              - run unit tests                               #
#   make test-e2e               - run end-to-end tests                         #
#   make test-memory            - run memory tests                             #
#   make pack                   - run tests and pack the submission archive    #
#   make pack-force             - pack the submission archive without tests    #
#   make docs                   - generate documentation                       #
#   make clean                  - remove build artifacts                       #
################################################################################


# Set default recepie
.DEFAULT_GOAL := release

################################################################################
# Compiler configuration.                                                      #
################################################################################
CC = clang
CSTD ?= c17

CFLAGS_COMMON := -std=$(CSTD) -Wall -Wextra -pedantic -MMD -MP
CFLAGS_RELEASE := $(CFLAGS_COMMON) -O3
CFLAGS_DEBUG := $(CFLAGS_COMMON) -g -O0

################################################################################
# OS detection and adaptation logic                                            #
################################################################################

# Binary file extension
ifeq ($(OS),Windows_NT)
    EXE := .exe
else
    EXE :=
endif

# Host OS name
ifeq ($(OS),Windows_NT)
    HOST_OS := windows
else
    HOST_OS := $(shell uname -s | tr 'A-Z' 'a-z')

    ifeq ($(HOST_OS),)
        HOST_OS := unix
    endif
endif

################################################################################
# Source, build and test directories                                           #
################################################################################

# Source directory
SRC_DIR := src

# Build root directory
BUILD_ROOT := build
BUILD_DIR := $(BUILD_ROOT)/$(HOST_OS)

# Build sub-directories
RELEASE_BUILD_DIR := $(BUILD_DIR)/release
DEBUG_BUILD_DIR := $(BUILD_DIR)/debug
TEST_BUILD_DIR := $(BUILD_DIR)/tests

# Unit tests directory
UNIT_TESTS_DIR := tests/unit

# Script directory (scripts normally, tests/scripts in submitted .zip)
SCRIPT_DIR := $(if $(wildcard scripts/run_unit.py),scripts,tests/scripts)

################################################################################
# Project source and build files                                               #
################################################################################

# Final binary name
BIN_NAME := ifj26$(EXE)

# All source files
SRCS := $(wildcard $(SRC_DIR)/*.c)

# Release build binary, objects and dependencies
RELEASE_BIN := $(BIN_NAME)
RELEASE_OBJS := $(patsubst $(SRC_DIR)/%.c,$(RELEASE_BUILD_DIR)/%.o,$(SRCS))
RELEASE_DEPS := $(RELEASE_OBJS:.o=.d)

################################################################################
# Debugging/testing source files and binaries                                  #
################################################################################

# Debug build binary, object files and dependencies
DEBUG_BIN := $(DEBUG_BUILD_DIR)/$(BIN_NAME)
DEBUG_OBJS := $(patsubst $(SRC_DIR)/%.c,$(DEBUG_BUILD_DIR)/%.o,$(SRCS))
DEBUG_DEPS := $(DEBUG_OBJS:.o=.d)

# Unit tests source files
UNIT_SRCS := $(wildcard $(UNIT_TESTS_DIR)/*.c)

# Unit tests file names, binaries and dependencies
UNIT_NAMES := $(patsubst $(UNIT_TESTS_DIR)/%.c,%,$(UNIT_SRCS))
UNIT_BINS := $(addprefix $(TEST_BUILD_DIR)/,$(addsuffix $(EXE),$(UNIT_NAMES)))
UNIT_DEPS := $(addprefix $(TEST_BUILD_DIR)/,$(addsuffix .d,$(UNIT_NAMES)))
UNIT_OBJS := $(filter-out $(DEBUG_BUILD_DIR)/ifj26.o,$(DEBUG_OBJS))

# Unit test helper sources, objects, and dependencies
UNIT_HELPER_SRCS := $(wildcard $(UNIT_TESTS_DIR)/helpers/*.c)
UNIT_HELPER_OBJS := $(patsubst $(UNIT_TESTS_DIR)/helpers/%.c,$(TEST_BUILD_DIR)/helper_%.o,$(UNIT_HELPER_SRCS))
UNIT_DEPS += $(UNIT_HELPER_OBJS:.o=.d)

################################################################################
# Test configuration                                                           #
################################################################################

# Whether to use Valgrind for memory testing
VALGRIND_FLAG := --valgrind
ifeq ($(VALGRIND),0)
    VALGRIND_FLAG := --no-valgrind
endif

# Python executable to use for running tests
ifeq ($(OS),Windows_NT)
    PY ?= python
else
    PY ?= python3
endif

# CFLAGS for unit tests (including flags required for Snow = testing library)
CFLAGS_UNIT := -std=gnu17 $(filter-out -std=%,$(CFLAGS_DEBUG)) -Wno-strict-prototypes -DSNOW_ENABLED -Itests/vendor -Isrc
ifeq ($(OS),Windows_NT)
    # Windows MSVC-compatible Snow flags
    CFLAGS_UNIT += -Itests/vendor/snow_win -DSNOW_USE_FORK=0 -DSNOW_USE_FNMATCH=0
endif

# Dependencies for memory test (Linux only, requires Valgrind)
ifeq ($(HOST_OS),linux)
    # Modified ifj26.c with renamed main to compiler_main
    $(DEBUG_BUILD_DIR)/memory_main.o: $(SRC_DIR)/ifj26.c | $(DEBUG_BUILD_DIR)
	@$(CC) $(CFLAGS_DEBUG) -Dmain=compiler_main -c -o $@ $<

    # Memory test executable
    $(DEBUG_BUILD_DIR)/memory_driver: tests/memory_driver.c $(DEBUG_BUILD_DIR)/memory_main.o $(UNIT_OBJS)
	@$(CC) $(CFLAGS_DEBUG) -Isrc -Wl,--wrap=ctx_malloc_helper,--wrap=ctx_realloc_helper -o $@ $^

    # Add recepie dependency
    test-memory: $(DEBUG_BUILD_DIR)/memory_driver

    # Correctly invalidate memory_main.d on rebuild
    -include $(DEBUG_BUILD_DIR)/memory_main.d
endif

################################################################################
# Recipes                                                                      #
################################################################################

# Build the release binary
release: $(RELEASE_BIN)
$(RELEASE_BIN): $(RELEASE_OBJS)
	@$(CC) $(CFLAGS_RELEASE) -o $@ $^

# Build individual object files (release)
$(RELEASE_BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(RELEASE_BUILD_DIR)
	@$(CC) $(CFLAGS_RELEASE) -c -o $@ $<

# Build the debug binary
debug: $(DEBUG_BIN)
$(DEBUG_BIN): $(DEBUG_OBJS)
	@$(CC) $(CFLAGS_DEBUG) -o $@ $^

# Build individual object files (debug)
$(DEBUG_BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(DEBUG_BUILD_DIR)
	@$(CC) $(CFLAGS_DEBUG) -c -o $@ $<

# Build unit test helper objects
$(TEST_BUILD_DIR)/helper_%.o: $(UNIT_TESTS_DIR)/helpers/%.c | $(TEST_BUILD_DIR)
	@$(CC) $(CFLAGS_UNIT) -MF $(TEST_BUILD_DIR)/helper_$*.d -c -o $@ $<

# Build unit test executables
$(TEST_BUILD_DIR)/%$(EXE): tests/unit/%.c $(UNIT_OBJS) $(UNIT_HELPER_OBJS) | $(TEST_BUILD_DIR)
	@$(CC) $(CFLAGS_UNIT) -MF $(TEST_BUILD_DIR)/$*.d -o $@ $< $(filter %.o,$^)

# Run unit tests
test-unit: $(UNIT_BINS)
	@$(PY) $(SCRIPT_DIR)/run_unit.py $(VALGRIND_FLAG)

# Run end-to-end tests
test-e2e: debug
	@$(PY) $(SCRIPT_DIR)/run_e2e.py $(VALGRIND_FLAG) --compiler "$(abspath $(DEBUG_BIN))" --build-dir "$(TEST_BUILD_DIR)"

# Run memory tests (Linux only, requires Valgrind)
test-memory: debug
	@$(PY) $(SCRIPT_DIR)/run_e2e.py --compiler "$(abspath $(DEBUG_BIN))" --build-dir "$(TEST_BUILD_DIR)" --memory $(if $(filter linux,$(HOST_OS)),--memory-driver "$(DEBUG_BUILD_DIR)/memory_driver")

# Run all tests
test: test-unit test-e2e test-memory

# Run tests and pack the submission archive
pack:
	+@$(MAKE) clean
	+@$(MAKE) -k test
	@$(PY) $(SCRIPT_DIR)/pack.py "$(MAKE)"

# Pack the submission archive without running tests
pack-force:
	+@$(MAKE) clean
	@$(PY) $(SCRIPT_DIR)/pack.py "$(MAKE)"

# Generate documentation
docs: $(BUILD_ROOT)
	@$(PY) -c "from pathlib import Path; Path('$(BUILD_ROOT)/dokumentace.pdf').touch()"

################################################################################
# Folder/file utilities                                                        #
################################################################################

# Ensure the build directories exist
ifeq ($(OS),Windows_NT)
    MKDIR = cmd.exe /C mkdir $(subst /,\,$@)
else
    MKDIR = mkdir $@
endif

$(BUILD_ROOT):
	@-$(MKDIR)
$(BUILD_DIR): | $(BUILD_ROOT)
	@-$(MKDIR)
$(RELEASE_BUILD_DIR) $(DEBUG_BUILD_DIR) $(TEST_BUILD_DIR): | $(BUILD_DIR)
	@-$(MKDIR)

# Clean up build artifacts
clean:
ifeq ($(OS),Windows_NT)
	@-cmd.exe /C if exist $(BUILD_ROOT) rmdir /S /Q $(BUILD_ROOT)
	@-cmd.exe /C if exist $(RELEASE_BIN) del $(RELEASE_BIN)
else
	@-rm -rf $(BUILD_ROOT) $(RELEASE_BIN)
endif

################################################################################
# Make configuration                                                           #
################################################################################

# Non-filename recipes
.PHONY: release debug test test-unit test-e2e test-memory pack pack-force docs clean

# Correctly rebuild dependency changes
-include $(RELEASE_DEPS) $(DEBUG_DEPS) $(UNIT_DEPS)
