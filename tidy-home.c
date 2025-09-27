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
int create_directory(const char *path);
int file_exists(const char *path);
int is_directory(const char *path);
int ask_confirmation(const char *message);
int move_file(const char *src, const char *dest);
int move_files_by_pattern(const char *pattern, const char *dest_dir, const char *description);
int move_specific_files(const char **files, int count, const char *dest_dir, const char *description);
int is_project_directory(const char *path);
void scan_and_suggest_projects(void);
char *get_home_path(const char *relative_path);
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
    
    // Move each file
    int success = 1;
    for (size_t i = 0; i < glob_result.gl_pathc; i++) {
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

int move_specific_files(const char **files, int count, const char *dest_dir, const char *description) {
    char **found_files = malloc(count * sizeof(char*));
    int found_count = 0;
    
    if (!found_files) {
        log_error("Memory allocation failed");
        return 0;
    }
    
    // Check which files exist
    for (int i = 0; i < count; i++) {
        char *full_path = get_home_path(files[i]);
        if (full_path && file_exists(full_path)) {
            found_files[found_count] = full_path;
            found_count++;
        } else if (full_path) {
            free(full_path);
        }
    }
    
    if (found_count == 0) {
        if (verbose) log_info("No specific files found for %s", description);
        free(found_files);
        return 1;
    }
    
    log_info("Moving %s (%d files found)", description, found_count);
    
    // Show files if interactive
    if (interactive) {
        printf("Files to move:\n");
        for (int i = 0; i < found_count; i++) {
            printf("  - %s\n", basename(found_files[i]));
        }
        
        char question[256];
        snprintf(question, sizeof(question), "Move these files to %s?", dest_dir);
        if (!ask_confirmation(question)) {
            log_info("Skipping %s", description);
            for (int i = 0; i < found_count; i++) {
                free(found_files[i]);
            }
            free(found_files);
            return 1;
        }
    }
    
    // Move files
    int success = 1;
    for (int i = 0; i < found_count; i++) {
        char dest_path[MAX_PATH];
        snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_dir, basename(found_files[i]));
        
        if (verbose || dry_run) {
            printf("  %s -> %s/\n", basename(found_files[i]), dest_dir);
        }
        
        if (!move_file(found_files[i], dest_path)) {
            success = 0;
        }
        free(found_files[i]);
    }
    
    free(found_files);
    return success;
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
    
    // Move stray files to Misc
    log_info("=== Moving stray files to ~/Misc ===");
    move_files_by_pattern("1*", misc_dir, "Numbered files");
    move_files_by_pattern("cbmgm.md*", misc_dir, "cbmgm markdown files");
    move_files_by_pattern("sys_info_page.html*", misc_dir, "System info pages");
    move_files_by_pattern("text.swift*", misc_dir, "Swift text files");
    move_files_by_pattern("*.pdf", misc_dir, "PDF files");
    
    // Move backup/temp files to tmp
    log_info("=== Moving backup/temp files to ~/tmp ===");
    move_files_by_pattern(".bash_history-*.tmp", tmp_dir, "Bash history backups");
    move_files_by_pattern("*~", tmp_dir, "Backup files (ending with ~)");
    move_files_by_pattern("*.un~", tmp_dir, "Vim undo files");
    move_files_by_pattern("*un~*", tmp_dir, "Vim backup files");
    move_files_by_pattern("*.dta*", tmp_dir, "Data files");
    move_files_by_pattern("*.tmp", tmp_dir, "Temporary files");
    move_files_by_pattern("*.bak", tmp_dir, "Backup files");
    
    // Scan for project directories
    log_info("=== Scanning for project directories ===");
    scan_and_suggest_projects();
    
    if (dry_run) {
        log_success("Dry run completed - no files were actually moved");
    } else {
        log_success("Home directory cleanup completed");
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
