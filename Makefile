# Cross-Platform Tetris Makefile
# Windows (MinGW/MSYS2/Cygwin) and Unix/Linux/macOS support

# Compiler settings
CC = gcc

# Basic compile flags
CFLAGS = -Wall -Wextra -std=c99 -O2 -Wno-sign-compare

# Source file
SRCFILE = tetris.c

# Platform detection
ifeq ($(OS),Windows_NT)
    # Windows environment
    EXECUTABLE = tetris.exe
    PLATFORM = Windows
    # Windows additional flags (if needed)
    LDFLAGS = 
    RM = del /Q
    CLEAN_TARGET = tetris.exe tetris_result.dat
    # Windows console encoding settings for Korean
    ECHO = @echo
else
    # Unix/Linux/macOS environment
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        PLATFORM = Linux
    endif
    ifeq ($(UNAME_S),Darwin)
        PLATFORM = macOS
    endif
    ifndef PLATFORM
        PLATFORM = Unix
    endif
    
    EXECUTABLE = tetris
    LDFLAGS = 
    RM = rm -f
    CLEAN_TARGET = tetris tetris_result.dat
    ECHO = @echo
endif

# Default target
.PHONY: all clean run help install debug release info test check

all: $(EXECUTABLE)
	$(ECHO) "==================================="
	$(ECHO) "Build Complete!"
	$(ECHO) "Platform: $(PLATFORM)"
	$(ECHO) "Executable: $(EXECUTABLE)"
	$(ECHO) "==================================="

# Main build rule
$(EXECUTABLE): $(SRCFILE)
	$(ECHO) "==================================="
	$(ECHO) "Compiling Tetris for $(PLATFORM)..."
	$(ECHO) "==================================="
	$(CC) $(CFLAGS) -o $(EXECUTABLE) $(SRCFILE) $(LDFLAGS)

# Debug build
debug: CFLAGS += -DDEBUG -g
debug: $(EXECUTABLE)
	$(ECHO) "Debug build complete"

# Release build (enhanced optimization)
release: CFLAGS += -DNDEBUG -O3
release: $(EXECUTABLE)
	$(ECHO) "Release build complete"

# Run
run: $(EXECUTABLE)
	$(ECHO) "==================================="
	$(ECHO) "Running Tetris Game..."
	$(ECHO) "==================================="
ifeq ($(OS),Windows_NT)
	.\$(EXECUTABLE)
else
	./$(EXECUTABLE)
endif

# Clean
clean:
	$(ECHO) "==================================="
	$(ECHO) "Cleaning build files..."
	$(ECHO) "==================================="
	-$(RM) $(CLEAN_TARGET)
	$(ECHO) "Clean complete"
# Help
help:
	$(ECHO) "==================================="
	$(ECHO) "Cross-Platform Tetris Makefile"
	$(ECHO) "==================================="
	$(ECHO) "Available commands:"
	$(ECHO) ""
	$(ECHO) "  make or make all     - Normal build"
	$(ECHO) "  make debug          - Debug build (with -g flag)"
	$(ECHO) "  make release        - Release build (optimized)"
	$(ECHO) "  make run            - Build and run"
	$(ECHO) "  make clean          - Clean build files"
	$(ECHO) "  make install        - Install to system"
	$(ECHO) "  make help           - Show this help"
	$(ECHO) ""
	$(ECHO) "Current platform: $(PLATFORM)"
	$(ECHO) "Target file: $(EXECUTABLE)"
	$(ECHO) "==================================="

# Install (optional)
install: $(EXECUTABLE)
ifeq ($(OS),Windows_NT)
	$(ECHO) "For Windows, manually add to PATH"
	$(ECHO) "or copy $(EXECUTABLE) to desired location"
else
	$(ECHO) "Installing to system..."
	sudo cp $(EXECUTABLE) /usr/local/bin/
	$(ECHO) "Install complete! Run 'tetris' from anywhere"
endif

# Show compiler and flag information
info:
	$(ECHO) "==================================="
	$(ECHO) "Build Environment Info"
	$(ECHO) "==================================="
	$(ECHO) "Compiler: $(CC)"
	$(ECHO) "Compile flags: $(CFLAGS)"
	$(ECHO) "Link flags: $(LDFLAGS)"
	$(ECHO) "Source file: $(SRCFILE)"
	$(ECHO) "Target file: $(EXECUTABLE)"
	$(ECHO) "Platform: $(PLATFORM)"
	$(ECHO) "==================================="

# Quick test build and run
test: clean all run

# Dependency check (for advanced users)
check:
	$(ECHO) "==================================="
	$(ECHO) "Checking system dependencies..."
	$(ECHO) "==================================="
	@$(CC) --version | head -n1 || echo "GCC not installed"
	@make --version | head -n1 || echo "Make not installed"
ifeq ($(OS),Windows_NT)
	$(ECHO) "Windows environment detected"
else
	@uname -a
endif
	$(ECHO) "==================================="