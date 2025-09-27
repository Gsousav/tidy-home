#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <glob.h>
#include <ctype.h>

#define MAX_PATH 1024
#define MAX_FILES 1000
#define VERSION "1.0.0"

// Color codes
#define COLOR_RED     "\033[0;31m"
#define COLOR_GREEN   "\033[0;32m"
#define COLOR_YELLOW  "\033[1;33m"
#define COLOR_BLUE    "\033[0;34m"
#define COLOR_RESET   "\033[0m"

// Global options
static int dry_run = 0;
static int verbose = 0;
static int interactive = 0;
static int no_color = 0;

// Logging macros
#define log_info(fmt, ...) \
    printf("%s[INFO]%s " fmt "\n", no_color ? "" : COLOR_BLUE, no_color ? "" : COLOR_RESET, ##__VA_ARGS__)

#define log_success(fmt, ...) \
    printf("%s[SUCCESS]%s " fmt "\n", no_color ? "" : COLOR_GREEN, no_color ? "" : COLOR_RESET, ##__VA_ARGS__)

#define log_warning(fmt, ...) \
    printf("%s[WARNING]%s " fmt "\n", no_color ? "" : COLOR_YELLOW, no_color ? "" : COLOR_RESET, ##__VA_ARGS__)

#define log_error(fmt, ...) \
    fprintf(stderr, "%s[ERROR]%s " fmt "\n", no_color ? "" : COLOR_RED, no_color ? "" : COLOR_RESET, ##__VA_ARGS__)

#define log_dry_run(fmt, ...) \
    printf("%s[DRY RUN]%s " fmt "\n", no_color ? "" : COLOR_YELLOW, no_color ? "" : COLOR_RESET, ##__VA_ARGS__)

// Function prototypes
void show_help(void);
void show_version(void);
char *get_home_path(const char *relative_path);
int file_exists(const char *path);
int is_directory(const char *path);
int create_directory(const char *path);
int ask_confirmation(const char *message);
int move_file(const char *src, const char *dest);
int should_preserve_file(const char *filepath);
int is_project_directory(const char *path);
int move_files_by_pattern(const char *pattern, const char *dest_dir, const char *description);
void scan_and_suggest_projects(void);
void scan_and_move_stray_dirs(void);
void cleanup_home(void);

void show_help(void) {
    printf("tidy-home v%s - Intelligent home folder cleanup\n\n", VERSION);
    printf("USAGE:\n");
    printf("    tidy-home [OPTIONS]\n\n");
    printf("OPTIONS:\n");
    printf("    -d, --dry-run      Show what would be moved without actually moving files\n");
    printf("    -v, --verbose      Show detailed output\n");
    printf("    -i, --interactive  Ask before moving each category of files\n");
    printf("    -n, --no-color     Disable colored output\n");
    printf("    -h, --help         Show this help message\n");
    printf("    -V, --version      Show version information\n\n");
    printf("DESCRIPTION:\n");
    printf("    Safely organizes your home directory by moving files into appropriate\n");
    printf("    subdirectories based on file patterns and types. Creates:\n");
    printf("    - ~/Misc     for miscellaneous files\n");
    printf("    - ~/tmp      for temporary and backup files\n");
    printf("    - ~/Projects for detected project directories\n\n");
    printf("EXAMPLES:\n");
    printf("    tidy-home --dry-run     # Preview changes without moving files\n");
    printf("    tidy-home -vi           # Interactive mode with verbose output\n");
    printf("    tidy-home               # Clean up home directory\n\n");
}

void show_version(void) {
    printf("tidy-home version %s\n", VERSION);
    printf("A safe home directory organizer written in C\n");
}

char *get_home_path(const char *relative_path) {
    const char *home = getenv("HOME");
    if (!home) {
        log_error("HOME environment variable not set");
        return NULL;
    }
    
    char *full_path = malloc(MAX_PATH);
    if (!full_path) {
        log_error("Memory allocation failed");
        return NULL;
    }
    
    if (relative_path && strlen(relative_path) > 0) {
        snprintf(full_path, MAX_PATH, "%s/%s", home, relative_path);
    } else {
        strncpy(full_path, home, MAX_PATH - 1);
        full_path[MAX_PATH - 1] = '\0';
    }
    
    return full_path;
}

int file_exists(const char *path) {
    return access(path, F_OK) == 0;
}

int is_directory(const char *path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

int create_directory(const char *path) {
    if (is_directory(path)) {
        return 1; // Already exists
    }
    
    if (dry_run) {
        log_dry_run("Would create directory: %s", path);
        return 1;
    }
    
    if (mkdir(path, 0755) == 0) {
        if (verbose) log_info("Created directory: %s", path);
        return 1;
    } else {
        log_error("Failed to create directory %s: %s", path, strerror(errno));
        return 0;
    }
}

int ask_confirmation(const char *message) {
    if (!interactive) return 1;
    
    printf("%s [Y/n]: ", message);
    fflush(stdout);
    
    char response[10];
    if (fgets(response, sizeof(response), stdin) == NULL) {
        return 0;
    }
    
    // Default to yes if just Enter is pressed
    if (response[0] == '\n') return 1;
    
    return (response[0] == 'y' || response[0] == 'Y');
}

int move_file(const char *src, const char *dest) {
    if (dry_run) {
        log_dry_run("Would move: %s -> %s", src, dest);
        return 1;
    }
    
    if (rename(src, dest) == 0) {
        if (verbose) log_info("Moved: %s -> %s", basename((char*)src), dirname((char*)dest));
        return 1;
    } else {
        log_error("Failed to move %s to %s: %s", src, dest, strerror(errno));
        return 0;
    }
}

int should_preserve_file(const char *filepath) {
    const char *basename_file = basename((char*)filepath);
    
    // Always preserve critical dotfiles and configs
    const char *critical_files[] = {
        ".bashrc", ".zshrc", ".profile", ".bash_profile", ".vimrc", ".gitconfig",
        ".bash_history", ".python_history", ".lesshst", ".viminfo"
    };
    
    for (size_t i = 0; i < sizeof(critical_files) / sizeof(critical_files[0]); i++) {
        if (strcmp(basename_file, critical_files[i]) == 0) {
            return 1;
        }
    }
    
    // Preserve standard directories
    const char *standard_dirs[] = {
        "Desktop", "Documents", "Downloads", "Pictures", "Videos", "Music",
        "Public", "Templates", "bin", "Projects", "Misc", "tmp", "dev"
    };
    
    for (size_t i = 0; i < sizeof(standard_dirs) / sizeof(standard_dirs[0]); i++) {
        if (strcmp(basename_file, standard_dirs[i]) == 0) {
            return 1;
        }
    }
    
    // Preserve important hidden directories
    const char *important_hidden[] = {
        ".config", ".local", ".cache", ".ssh", ".gnupg", ".git"
    };
    
    for (size_t i = 0; i < sizeof(important_hidden) / sizeof(important_hidden[0]); i++) {
        if (strcmp(basename_file, important_hidden[i]) == 0) {
            return 1;
        }
    }
    
    return 0;
}

int is_project_directory(const char *path) {
    if (!is_directory(path)) return 0;
    
    char check_path[MAX_PATH];
    const char *project_indicators[] = {
        "package.json", "Cargo.toml", "setup.py", "requirements.txt",
        "Makefile", "CMakeLists.txt", ".git", "pom.xml", "build.gradle"
    };
    
    for (size_t i = 0; i < sizeof(project_indicators) / sizeof(project_indicators[0]); i++) {
        snprintf(check_path, sizeof(check_path), "%s/%s", path, project_indicators[i]);
        if (file_exists(check_path)) return 1;
    }
    
    // Check directory name patterns
    const char *name = basename((char*)path);
    if (strstr(name, "project") || strstr(name, "code") || 
        strncmp(name, "ml-", 3) == 0 || strcmp(name, "raylib") == 0) {
        return 1;
    }
    
    return 0;
}

int move_files_by_pattern(const char *pattern, const char *dest_dir, const char *description) {
    char *home_pattern = get_home_path(pattern);
    if (!home_pattern) return 0;
    
    glob_t glob_result;
    int glob_flags = GLOB_NOSORT;
    
    if (glob(home_pattern, glob_flags, NULL, &glob_result) != 0) {
        if (verbose) log_info("No files matching pattern '%s'", pattern);
        free(home_pattern);
        return 1;
    }
    
    if (glob_result.gl_pathc == 0) {
        if (verbose) log_info("No files matching pattern '%s'", pattern);
        globfree(&glob_result);
        free(home_pattern);
        return 1;
    }
    
    log_info("%s (%zu files found)", description, glob_result.gl_pathc);
    
    // Show files if interactive
    if (interactive) {
        printf("Files to move:\n");
        for (size_t i = 0; i < glob_result.gl_pathc; i++) {
            printf("  - %s\n", basename(glob_result.gl_pathv[i]));
        }
        
        char question[256];
        snprintf(question, sizeof(question), "Move these files to %s?", dest_dir);
        if (!ask_confirmation(question)) {
            log_info("Skipping %s", description);
            globfree(&glob_result);
            free(home_pattern);
            return 1;
        }
    }
    
    // Move each file, but skip preserved ones
    int success = 1;
    for (size_t i = 0; i < glob_result.gl_pathc; i++) {
        // Skip preserved files
        if (should_preserve_file(glob_result.gl_pathv[i])) {
            if (verbose) {
                log_info("Skipping preserved file: %s", basename(glob_result.gl_pathv[i]));
            }
            continue;
        }
        
        char dest_path[MAX_PATH];
        snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_dir, basename(glob_result.gl_pathv[i]));
        
        if (verbose || dry_run) {
            printf("  %s -> %s/\n", basename(glob_result.gl_pathv[i]), dest_dir);
        }
        
        if (!move_file(glob_result.gl_pathv[i], dest_path)) {
            success = 0;
        }
    }
    
    globfree(&glob_result);
    free(home_pattern);
    return success;
}

void scan_and_suggest_projects(void) {
    char *home = get_home_path("");
    if (!home) return;
    
    DIR *dir = opendir(home);
    if (!dir) {
        log_error("Cannot open home directory");
        free(home);
        return;
    }
    
    log_info("Scanning for project directories...");
    
    struct dirent *entry;
    int found_projects = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue; // Skip hidden dirs
        if (strcmp(entry->d_name, "Projects") == 0) continue; // Skip Projects dir itself
        
        char full_path[MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s/%s", home, entry->d_name);
        
        if (is_project_directory(full_path)) {
            if (found_projects == 0) {
                printf("\nDetected project directories:\n");
            }
            printf("  - %s\n", entry->d_name);
            found_projects++;
        }
    }
    
    if (found_projects > 0) {
        printf("\nYou can move these to ~/Projects manually if desired:\n");
        printf("  mv ~/project_name ~/Projects/\n");
    } else {
        log_info("No project directories detected");
    }
    
    closedir(dir);
    free(home);
}

void scan_and_move_stray_dirs(void) {
    char *home = get_home_path("");
    if (!home) return;
    
    char *misc_dir = get_home_path("Misc");
    if (!misc_dir) {
        free(home);
        return;
    }
    
    DIR *dir = opendir(home);
    if (!dir) {
        log_error("Cannot open home directory");
        free(home);
        free(misc_dir);
        return;
    }
    
    log_info("Scanning for stray directories to organize...");
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue; // Skip hidden dirs
        
        char full_path[MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s/%s", home, entry->d_name);
        
        // Only process directories
        if (!is_directory(full_path)) continue;
        
        // Skip if it's a preserved directory
        if (should_preserve_file(full_path)) continue;
        
        // Skip if it's a detected project
        if (is_project_directory(full_path)) continue;
        
        // Check for directories that look like they should be moved
        const char *name = entry->d_name;
        int should_move = 0;
        
        // Single character directories or weird names
        if (strlen(name) == 1 && (name[0] == '-' || isdigit(name[0]))) {
            should_move = 1;
        }
        // Directories with obvious temporary/download patterns
        else if (strstr(name, "tmp") || strstr(name, "temp") || 
                 strstr(name, "download") || strstr(name, "extract") ||
                 strstr(name, "backup") || strstr(name, "old")) {
            should_move = 1;
        }
        // Application directories that aren't in standard locations
        else if (strstr(name, "linux-x64") || strstr(name, "AppImage") ||
                 strstr(name, "portable") || strstr(name, "standalone") ||
                 strcmp(name, "ado") == 0) { // Add specific cases like 'ado'
            should_move = 1;
        }
        
        if (should_move) {
            if (interactive) {
                char question[512];
                snprintf(question, sizeof(question), 
                    "Move directory '%s' to ~/Misc?", name);
                if (!ask_confirmation(question)) continue;
            }
            
            char dest_path[MAX_PATH];
            snprintf(dest_path, sizeof(dest_path), "%s/%s", misc_dir, name);
            
            if (verbose || dry_run) {
                printf("  %s/ -> Misc/\n", name);
            }
            
            if (!move_file(full_path, dest_path)) {
                log_warning("Failed to move directory %s", name);
            }
        }
    }
    
    closedir(dir);
    free(home);
    free(misc_dir);
}

void cleanup_home(void) {
    // Create necessary directories
    char *misc_dir = get_home_path("Misc");
    char *tmp_dir = get_home_path("tmp");
    char *projects_dir = get_home_path("Projects");
    
    if (!misc_dir || !tmp_dir || !projects_dir) {
        log_error("Failed to construct directory paths");
        goto cleanup;
    }
    
    if (!create_directory(misc_dir) || !create_directory(tmp_dir) || !create_directory(projects_dir)) {
        log_error("Failed to create necessary directories");
        goto cleanup;
    }
    
    // Move backup/temp files to tmp (do this first as it's safest)
    log_info("=== Moving backup/temp files to ~/tmp ===");
    move_files_by_pattern("*.un~", tmp_dir, "Vim undo files");
    move_files_by_pattern("*~", tmp_dir, "Backup files (ending with ~)");
    move_files_by_pattern("*.tmp", tmp_dir, "Temporary files");
    move_files_by_pattern("*.bak", tmp_dir, "Backup files");
    move_files_by_pattern(".bash_history-*", tmp_dir, "Bash history backups");
    move_files_by_pattern("*.log", tmp_dir, "Log files");
    move_files_by_pattern("*.old", tmp_dir, "Old files");
    
    // Move obvious stray files to Misc
    log_info("=== Moving stray files to ~/Misc ===");
    move_files_by_pattern("[0-9]*", misc_dir, "Files starting with numbers");
    move_files_by_pattern("*.html", misc_dir, "HTML files");
    move_files_by_pattern("*.pdf", misc_dir, "PDF files");
    move_files_by_pattern("*.md", misc_dir, "Markdown files");
    move_files_by_pattern("*.txt", misc_dir, "Text files");
    move_files_by_pattern("*.doc", misc_dir, "Word documents");
    move_files_by_pattern("*.docx", misc_dir, "Word documents");
    move_files_by_pattern("*.csv", misc_dir, "CSV files");
    move_files_by_pattern("*.json", misc_dir, "JSON files");
    move_files_by_pattern("*.xml", misc_dir, "XML files");
    move_files_by_pattern("*.sql", misc_dir, "SQL files");
    move_files_by_pattern("*.db", misc_dir, "Database files");
    move_files_by_pattern("*.sqlite*", misc_dir, "SQLite database files");
    move_files_by_pattern("*.dta", tmp_dir, "Stata data files");
    
    // Handle programming files that aren't in projects (be careful with these)
    move_files_by_pattern("*.py", misc_dir, "Standalone Python files");
    move_files_by_pattern("*.js", misc_dir, "Standalone JavaScript files");
    move_files_by_pattern("*.swift", misc_dir, "Standalone Swift files");
    
    // Move archive files to tmp
    move_files_by_pattern("*.zip", tmp_dir, "ZIP archives");
    move_files_by_pattern("*.tar.gz", tmp_dir, "Compressed archives");
    move_files_by_pattern("*.tar", tmp_dir, "Archive files");
    move_files_by_pattern("*.gz", tmp_dir, "Compressed files");
    move_files_by_pattern("*.rar", tmp_dir, "RAR archives");
    move_files_by_pattern("*.7z", tmp_dir, "7-Zip archives");
    
    // Scan and move stray directories
    log_info("=== Organizing stray directories ===");
    scan_and_move_stray_dirs();
    
    // Scan for project directories (suggestion only)
    log_info("=== Scanning for project directories ===");
    scan_and_suggest_projects();
    
    if (dry_run) {
        log_success("Dry run completed - no files were actually moved");
    } else {
        log_success("Home directory cleanup completed");
        log_info("Check ~/Misc, ~/tmp, and consider organizing ~/Projects");
    }

cleanup:
    free(misc_dir);
    free(tmp_dir);
    free(projects_dir);
}

int main(int argc, char *argv[]) {
    int option;
    static struct option long_options[] = {
        {"dry-run", no_argument, 0, 'd'},
        {"verbose", no_argument, 0, 'v'},
        {"interactive", no_argument, 0, 'i'},
        {"no-color", no_argument, 0, 'n'},
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'V'},
        {0, 0, 0, 0}
    };
    
    while ((option = getopt_long(argc, argv, "dvihnV", long_options, NULL)) != -1) {
        switch (option) {
            case 'd':
                dry_run = 1;
                break;
            case 'v':
                verbose = 1;
                break;
            case 'i':
                interactive = 1;
                break;
            case 'n':
                no_color = 1;
                break;
            case 'h':
                show_help();
                return 0;
            case 'V':
                show_version();
                return 0;
            case '?':
                fprintf(stderr, "Try 'tidy-home --help' for more information.\n");
                return 1;
            default:
                break;
        }
    }
    
    // Check if we're being run from the home directory or provide info
    char *home = get_home_path("");
    if (!home) {
        log_error("Could not determine home directory");
        return 1;
    }
    
    if (dry_run) {
        log_info("Running in DRY RUN mode - no files will be moved");
    }
    
    printf("tidy-home v%s - Cleaning up %s\n", VERSION, home);
    
    cleanup_home();
    
    free(home);
    return 0;
}
