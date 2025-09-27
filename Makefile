# Makefile for tidy-home
# Project settings
PROJECT = tidy-home
VERSION = 1.0.0

# Directories
SRC_DIR = src
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
BIN_DIR = $(BUILD_DIR)/bin
TEST_DIR = tests

# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
LDFLAGS = 

# Target and source files
TARGET = $(BIN_DIR)/$(PROJECT)
SOURCE = $(SRC_DIR)/main.c
INSTALL_DIR = $(HOME)/bin

# Default target
all: $(TARGET)

# Create build directories
$(OBJ_DIR) $(BIN_DIR):
	@mkdir -p $@

# Build the binary
$(TARGET): $(SOURCE) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE)

# Install to ~/bin
install: $(TARGET)
	@mkdir -p $(INSTALL_DIR)
	cp $(TARGET) $(INSTALL_DIR)/$(PROJECT)
	@echo "Installed $(PROJECT) to $(INSTALL_DIR)"
	@echo "Make sure $(INSTALL_DIR) is in your PATH"
	@echo "Add this to your ~/.bashrc or ~/.zshrc if not already present:"
	@echo "  export PATH=\"\$$HOME/bin:\$$PATH\""

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)

# Uninstall
uninstall:
	rm -f $(INSTALL_DIR)/$(PROJECT)
	@echo "Removed $(PROJECT) from $(INSTALL_DIR)"

# Development build with debug symbols
debug: CFLAGS += -g -DDEBUG
debug: clean $(TARGET)

# Run tests (basic functionality check)
test: $(TARGET)
	$(TARGET) --help
	$(TARGET) --version
	$(TARGET) --dry-run

# Create a release package
package: clean $(TARGET)
	@mkdir -p dist
	tar -czf dist/$(PROJECT)-$(VERSION).tar.gz \
		--exclude='.git*' \
		--exclude='build' \
		--exclude='dist' \
		--transform 's,^,$(PROJECT)-$(VERSION)/,' \
		.
	@echo "Created dist/$(PROJECT)-$(VERSION).tar.gz"

# Help target
help:
	@echo "Available targets:"
	@echo "  all      - Build the binary (default)"
	@echo "  install  - Install to ~/bin"
	@echo "  clean    - Remove build artifacts"
	@echo "  uninstall- Remove installed binary"
	@echo "  debug    - Build with debug symbols"
	@echo "  test     - Run basic functionality tests"
	@echo "  package  - Create release tarball"
	@echo "  help     - Show this help message"

.PHONY: all install clean uninstall debug test package help
