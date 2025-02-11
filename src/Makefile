# -----------------------------------------------------------------------------
# @file Makefile
# @brief Makefile for building the TCP Server and test suite.
# @details
# This Makefile provides build rules for the TCP Server library and its test 
# suite. It supports compilation, cleaning, static analysis, and memory leak 
# testing using Valgrind.
#
# Targets:
#   - `make` (or `make all`): Builds the TCP server library (excluding main.cpp).
#   - `make clean`: Removes all object files and test binaries.
#   - `make test`: Builds and runs the test program (includes main.cpp).
#   - `make valgrind`: Runs the test program with Valgrind to check for memory leaks.
#   - `make lint`: Runs static analysis using cppcheck.
#   - `make help`: Displays available targets.
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# Compiler and flags
# -----------------------------------------------------------------------------
CXX := g++
CXXFLAGS := -Wall -Wextra -pedantic -std=c++17
LDFLAGS := 

# -----------------------------------------------------------------------------
# Directories
# -----------------------------------------------------------------------------
SRC_DIR := .
LCBLOG_DIR := LCBLog

# -----------------------------------------------------------------------------
# Source and Object Files
# -----------------------------------------------------------------------------
SRCS := $(SRC_DIR)/tcp_server.cpp $(LCBLOG_DIR)/lcblog.cpp $(SRC_DIR)/tcp_command_handler.cpp
OBJS := $(SRCS:.cpp=.o)

# -----------------------------------------------------------------------------
# Test Program Configuration
# -----------------------------------------------------------------------------
TEST_SRC := main.cpp
TEST_OBJ := $(TEST_SRC:.cpp=.o)
TEST_BIN := tcp_test

# -----------------------------------------------------------------------------
# Default Target: Build Library (Excludes main.cpp)
# -----------------------------------------------------------------------------
all: $(OBJS)

# -----------------------------------------------------------------------------
# Build Rules
# -----------------------------------------------------------------------------

# General build rule for all object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c "$<" -o "$@"

# -----------------------------------------------------------------------------
# Parallel Compilation
# -----------------------------------------------------------------------------
.PHONY: all clean test valgrind lint help
MAKEFLAGS += -j$(nproc)

# -----------------------------------------------------------------------------
# Clean Target: Remove All Compiled Files
# -----------------------------------------------------------------------------
clean:
	rm -f $(OBJS) $(TEST_OBJ) $(TEST_BIN)

# -----------------------------------------------------------------------------
# Compile and Run Test Program
# -----------------------------------------------------------------------------
test: $(OBJS) $(TEST_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $(TEST_BIN)  # Ensure output is tcp_test
	./$(TEST_BIN)

# -----------------------------------------------------------------------------
# Run Memory Leak Tests with Valgrind
# -----------------------------------------------------------------------------
valgrind: test
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TEST_BIN)

# -----------------------------------------------------------------------------
# Run Static Analysis with cppcheck
# -----------------------------------------------------------------------------
lint:
	@echo -e "\e[33m'cppcheck' seems to crap itself on Raspbian Bookworm 32, so I give up. The rest is commented out.\e[0m"
	@exit 0
	# @which cppcheck >/dev/null 2>&1 || { echo "Error: cppcheck not found. Please install it."; exit 1; }
	# cppcheck --enable=all --std=c++17 --force --check-config \
	#	-I. --quiet $(SRCS) $(TEST_SRC)

# -----------------------------------------------------------------------------
# Display Available Targets
# -----------------------------------------------------------------------------
help:
	@echo "Available targets:"
	@echo "  make (or make all)   - Build the TCP server library only"
	@echo "  make clean           - Remove all object files and test binary"
	@echo "  make test            - Build and run the test program"
	@echo "  make valgrind        - Build and run with valgrind to check for memory leaks"
	@echo "  make lint            - Run static analysis with cppcheck"
	@echo "  make help            - Show this help message"