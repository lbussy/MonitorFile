# -----------------------------------------------------------------------------
# @file Makefile
# @brief Build system for the monitorfile test executable.
#
# This Makefile automates the process of building and testing monitorfile.
#
# @targets
# - `test`: Builds the test executable `monitorfile_test` with debug flags.
# - `lint`: Runs cppcheck for static code analysis on the source files.
# - `clean`: Cleans up build artifacts.
# - `help`: Displays available make targets and their descriptions.
# -----------------------------------------------------------------------------

# Target names
NAME        = monitorfile
TEST_NAME   = $(NAME)_test
TEST_LOGS   = testfile.txt

# Compiler settings
CXX         = g++
CXXFLAGS    += -Wall -Werror -fmax-errors=5
CXXFLAGS    += -Wno-psabi -std=c++17 -MMD -MP

# Test sources
TEST_SRC    = main.cpp
TEST_OBJ    = $(TEST_SRC:.cpp=.o)
TEST_DEP    = $(TEST_OBJ:.o=.d)

# Ensure parallel compilation
MAKEFLAGS ?= -j$(shell nproc)

# Clean old test logs before running tests
clean_logs:
	$(RM) $(TEST_LOGS)

# Build the test executable (Only compiles main.cpp)
test: clean_logs $(TEST_OBJ)
	$(CXX) $(CXXFLAGS) $(TEST_OBJ) -o $(TEST_NAME)
	@echo "Running $(TEST_NAME) in test mode."
	./$(TEST_NAME)

# Compile test source files into object files
main.o: main.cpp monitorfile.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up build artifacts
clean:
	-$(RM) $(TEST_OBJ) $(TEST_DEP) $(TEST_NAME) $(TEST_LOGS)

# Include dependency files
-include $(TEST_DEP)

# Lint target: performs static code analysis with cppcheck
lint:
	@command -v cppcheck >/dev/null 2>&1 || { \
	    echo "Error: cppcheck not found. Please install it using:"; \
	    echo "  sudo apt install cppcheck   # Debian/Ubuntu"; \
	    echo "  sudo dnf install cppcheck   # Fedora"; \
	    echo "  brew install cppcheck       # macOS (Homebrew)"; \
	    echo "  choco install cppcheck      # Windows (Chocolatey)"; \
	    exit 1; \
	}
	cppcheck --enable=all --inconclusive --max-configs=100 --std=c++17 --language=c++ $(TEST_SRC) --suppress=missingIncludeSystem

# Help target: displays available make targets and their descriptions
help:
	@echo "Available Make targets:"
	@echo "  test       - Build the test executable with debug flags."
	@echo "  lint       - Run cppcheck for static code analysis."
	@echo "  clean      - Clean up build artifacts."
	@echo "  help       - Display this help message."

.PHONY: test lint clean help clean_logs