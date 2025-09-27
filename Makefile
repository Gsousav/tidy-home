# Makefile for tidy-home
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
TARGET = tidy-home
SOURCE = tidy-home.c
INSTALL_DIR = $(HOME)/bin

# Default target
all: $(TARGET)

# Build the binary
$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE)

# Install to ~/bin
install: $(TARGET)
	@mkdir -p $(INSTALL_DIR)
	cp $(TARGET) $(INSTALL_DIR)/
	@echo "Installed $(TARGET) to $(INSTALL_DIR)"
	@echo "Make sure $(INSTALL_DIR) is in your PATH"
	@echo "Add this to your ~/.bashrc or ~/.zshrc if not already present:"
	@echo "  export PATH=\"\$$HOME/bin:\$$PATH\""

# Clean build artifacts
clean:
	rm -f $(TARGET)

# Uninstall
uninstall:
	rm -f $(INSTALL_DIR)/$(TARGET)
	@echo "Removed $(TARGET) from $(INSTALL_DIR)"

# Development build with debug symbols
debug: CFLAGS += -g -DDEBUG
debug: $(TARGET)

# Run tests (basic functionality check)
test: $(TARGET)
	./$(TARGET) --help
	./$(TARGET) --version
	./$(TARGET) --dry-run

.PHONY: all install clean uninstall debug test
