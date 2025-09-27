# TidyHome

A fast, safe, and intelligent home directory organizer written in C.

## Overview

TidyHome helps you clean up your home directory by automatically categorizing and moving files into organized subdirectories. It's designed to be safe, with dry-run capabilities and intelligent file detection.

## Features

- **🔍 Smart Detection**: Automatically identifies project directories, temporary files, and stray documents
- **🏃 Dry Run Mode**: Preview changes before making them with `--dry-run`
- **🎯 Interactive Mode**: Ask for confirmation before each operation with `--interactive`  
- **📁 Organized Structure**: Creates `~/Misc`, `~/tmp`, and `~/Projects` directories
- **🛡️ Safe Operations**: Preserves dotfiles, standard directories, and important configurations
- **🎨 Colored Output**: Clear, informative logging with color-coded messages

## Installation

### Quick Install

```bash
# Clone the repository
git clone 
cd TidyHome

# Build and install
make install
```

### Manual Build

```bash
# Build the binary
make

# Install to ~/bin
make install

# Add ~/bin to PATH (if not already present)
echo 'export PATH="$HOME/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

## Usage

### Basic Usage

```bash
# Preview what would be cleaned up (recommended first run)
tidy-home --dry-run

# Clean up your home directory
tidy-home

# Interactive mode with confirmations
tidy-home --interactive

# Verbose output with details
tidy-home --verbose
```

### Command Line Options

```
-d, --dry-run      Show what would be moved without actually moving files
-v, --verbose      Show detailed output
-i, --interactive  Ask before moving each category of files
-n, --no-color     Disable colored output
-h, --help         Show help message
-V, --version      Show version information
```

## What Gets Organized

### ~/Misc Directory
- Numbered files (1*, 2*, etc.)
- PDF documents
- Markdown files (*.md)
- HTML files
- Text files
- Other miscellaneous documents

### ~/tmp Directory  
- Backup files ending with `~`
- Temporary files (*.tmp)
- Vim undo files (*.un~)
- Data files (*.dta)
- Bash history backups

### ~/Projects Directory
TidyHome detects project directories by looking for:
- `package.json` (Node.js)
- `Cargo.toml` (Rust)
- `setup.py`, `requirements.txt` (Python)
- `Makefile`, `CMakeLists.txt` (C/C++)
- `.git` directories (Git repos)
- Directory names containing "project", "code", "ml-"

**Note**: Project directories are only *suggested* for manual moving to avoid breaking execution paths.

## Safety Features

- **Dry Run First**: Always test with `--dry-run` before actual cleanup
- **Preserves Important Files**: Never touches dotfiles, system directories, or configurations
- **Smart Detection**: Only moves files that clearly belong in specific categories
- **Error Handling**: Graceful handling of permission issues and edge cases
- **Interactive Confirmations**: Optional prompts before each operation

## Examples

```bash
# Safe first-time usage
tidy-home --dry-run --verbose

# Interactive cleanup with confirmations
tidy-home --interactive

# Quick silent cleanup
tidy-home

# See what projects were detected
tidy-home --dry-run | grep -A 10 "project directories"
```

## Development

### Building for Development

```bash
# Debug build with symbols
make debug

# Run tests
make test

# Clean build artifacts
make clean
```

### Project Structure

```
TidyHome/
├── tidy-home.c      # Main source code
├── Makefile         # Build configuration
├── README.md        # This file
├── .gitignore       # Git ignore rules
└── LICENSE          # License file
```

## Contributing

1. Fork the repository
2. Create a feature branch: `git checkout -b feature-name`
3. Make your changes
4. Test thoroughly with `--dry-run`
5. Commit your changes: `git commit -am 'Add feature'`
6. Push to the branch: `git push origin feature-name`
7. Submit a pull request

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Changelog

### v1.0.0
- Initial release
- Basic file categorization
- Dry run and interactive modes  
- Project directory detection
- Colored output and comprehensive logging
