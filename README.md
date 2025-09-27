# TidyHome

A fast, safe, and intelligent home directory organizer written in C.

TidyHome helps you clean up your home directory by automatically categorizing and moving files into organized subdirectories. It's designed to be safe, with dry-run capabilities and intelligent file detection.

## Features

- Smart Detection: Automatically identifies project directories, temporary files, and stray documents
- Dry Run Mode: Preview changes before making them with `--dry-run`
- Interactive Mode: Ask for confirmation before each operation with `--interactive`
- Organized Structure: Creates `~/Misc`, `~/tmp`, and `~/Projects` directories
- Safe Operations: Preserves dotfiles, standard directories, and important configurations
- Colored Output: Clear, informative logging with color-coded messages

## Installation

```bash
# Clone the repository
git clone git@github.com:Gsousav/tidy-home.git
cd TidyHome

# Build and install
make install
```

Make sure `~/bin` is in your PATH:
```bash
echo 'export PATH="$HOME/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

## Usage

```
Usage: tidy-home [OPTIONS]

Options:
  -d, --dry-run      Show what would be moved without actually moving files
  -v, --verbose      Show detailed output
  -i, --interactive  Ask before moving each category of files
  -n, --no-color     Disable colored output
  -h, --help         Show help message
  -V, --version      Show version information
```

### Examples

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

## What Gets Organized

**~/Misc Directory**
- PDF documents, markdown files, HTML files, text files
- Numbered files (1*, 2*, etc.)
- Other miscellaneous documents

**~/tmp Directory**
- Backup files ending with `~`
- Temporary files (*.tmp)
- Vim undo files (*.un~)
- Data files and bash history backups

**~/Projects Directory**
- Directories containing `package.json`, `Cargo.toml`, `setup.py`, `Makefile`, `.git`
- Directories with names containing "project", "code", "ml-"

Note: Project directories are only suggested for manual moving to avoid breaking execution paths.

## Development

```bash
# Debug build with symbols
make debug

# Run tests
make test

# Clean build artifacts
make clean
```

## License

MIT License
