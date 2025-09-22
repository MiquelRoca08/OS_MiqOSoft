#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <shell.h>
#include <shell_commands.h>
#include <string.h>
#include <syscall.h>
#include <vga_text.h>
#include <keyboard.h>
#include <io.h>
#include <x86.h>

//
// UTILITY FUNCTIONS - MUST BE FIRST
//

// Simple memcpy implementation
void* shell_memcpy(void* dest, const void* src, size_t n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}

// Simple memset implementation  
void* shell_memset(void* s, int c, size_t n) {
    unsigned char* p = s;
    while (n--) {
        *p++ = (unsigned char)c;
    }
    return s;
}

// Integer to string conversion
void format_number(char* buffer, int num) {
    if (num == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }

    int i = 0;
    int temp_num = num;
    char temp_str[11];

    while (temp_num > 0) {
        temp_str[i++] = (temp_num % 10) + '0';
        temp_num /= 10;
    }

    int j = 0;
    while (i > 0) {
        buffer[j++] = temp_str[--i];
    }
    buffer[j] = '\0';
}

// Simple getc implementation for shell
char shell_getc(void) {
    char c;
    while (!keyboard_buffer_pop(&c)) {
        keyboard_process_buffer();
    }
    return c;
}

// Simple snprintf implementation
int shell_snprintf(char* str, size_t size, const char* format, ...) {
    // Very basic implementation - just handle %d and %s
    const char* src = format;
    char* dst = str;
    size_t written = 0;
    
    // Get variable arguments (simplified)
    int* args = (int*)((char*)&format + sizeof(format));
    int arg_index = 0;
    
    while (*src && written < size - 1) {
        if (*src == '%' && *(src + 1)) {
            src++; // skip %
            if (*src == 'd') {
                // Convert integer to string
                int val = args[arg_index++];
                char temp[16];
                int len = 0;
                if (val == 0) {
                    temp[len++] = '0';
                } else {
                    if (val < 0) {
                        *dst++ = '-';
                        written++;
                        val = -val;
                    }
                    char digits[16];
                    int digit_count = 0;
                    while (val > 0) {
                        digits[digit_count++] = '0' + (val % 10);
                        val /= 10;
                    }
                    for (int i = digit_count - 1; i >= 0; i--) {
                        temp[len++] = digits[i];
                    }
                }
                for (int i = 0; i < len && written < size - 1; i++) {
                    *dst++ = temp[i];
                    written++;
                }
            } else if (*src == 's') {
                // Copy string
                char* s = (char*)args[arg_index++];
                while (*s && written < size - 1) {
                    *dst++ = *s++;
                    written++;
                }
            }
            src++;
        } else {
            *dst++ = *src++;
            written++;
        }
    }
    *dst = '\0';
    return written;
}

// Auxiliary function for hexadecimal conversion
uint32_t hex_str_to_int(const char* hex_str) {
    uint32_t result = 0;
    const char* hex = hex_str;
    
    if (hex[0] == '0' && hex[1] == 'x') hex += 2;
    
    while (*hex) {
        result <<= 4;
        if (*hex >= '0' && *hex <= '9') result += *hex - '0';
        else if (*hex >= 'a' && *hex <= 'f') result += *hex - 'a' + 10;
        else if (*hex >= 'A' && *hex <= 'F') result += *hex - 'A' + 10;
        hex++;
    }
    
    return result;
}

// Auxiliary function for decimal conversion
uint32_t dec_str_to_int(const char* dec_str) {
    uint32_t result = 0;
    while (*dec_str >= '0' && *dec_str <= '9') {
        result = result * 10 + (*dec_str - '0');
        dec_str++;
    }
    return result;
}

//
// External Declarations
//

// Functions from shell.c
extern int shell_strlen(const char* str);
extern int shell_strcmp(const char* s1, const char* s2);
extern char* shell_strchr(const char* str, int c);
extern char* shell_strstr(const char* haystack, const char* needle);
extern char* shell_strncpy(char* dest, const char* src, int n);

// Functions from syscall_test.c
extern void syscall_run_all_tests(void);
extern void syscall_test_heap_integrity(void);

// Functions from time subsystem
extern uint32_t sys_time(void);

// Functions from dmesg
extern void display_kernel_messages(void);
extern void dmesg_clear(void);
extern void dmesg_stats(void);
extern void kernel_add_message(char level, const char* component, const char* message);

//
// RAM-BASED FILE SYSTEM IMPLEMENTATION
//

#define MAX_FILES 64
#define MAX_FILENAME_LEN 32
#define MAX_FILE_SIZE 4096
#define MAX_PATH_DEPTH 8

typedef struct ram_file {
    char name[MAX_FILENAME_LEN];
    char* data;
    uint32_t size;
    uint32_t capacity;
    uint32_t is_directory;
    uint32_t parent_index;
    uint32_t in_use;
    uint32_t created_time;
} ram_file_t;

typedef struct ram_fs {
    ram_file_t files[MAX_FILES];
    uint32_t file_count;
    uint32_t current_dir;
    uint32_t initialized;
} ram_fs_t;

static ram_fs_t g_ramfs;

// Initialize the RAM file system
void ramfs_init(void) {
    if (g_ramfs.initialized) return;
    
    shell_memset(&g_ramfs, 0, sizeof(ram_fs_t));
    
    // Create root directory
    g_ramfs.files[0].name[0] = '/';
    g_ramfs.files[0].name[1] = '\0';
    g_ramfs.files[0].is_directory = 1;
    g_ramfs.files[0].parent_index = 0; // Root points to itself
    g_ramfs.files[0].in_use = 1;
    g_ramfs.files[0].created_time = sys_time();
    
    g_ramfs.file_count = 1;
    g_ramfs.current_dir = 0;
    g_ramfs.initialized = 1;
    
    printf("RAM File System initialized\n");
}

// Find file by name in current directory
int ramfs_find_file(const char* name, uint32_t parent_dir) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (g_ramfs.files[i].in_use && 
            g_ramfs.files[i].parent_index == parent_dir &&
            shell_strcmp(g_ramfs.files[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

// Find free file slot
int ramfs_find_free_slot(void) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (!g_ramfs.files[i].in_use) {
            return i;
        }
    }
    return -1;
}

// Create a new file or directory
int ramfs_create(const char* name, uint32_t is_directory) {
    if (!g_ramfs.initialized) ramfs_init();
    
    // Check if file already exists
    if (ramfs_find_file(name, g_ramfs.current_dir) >= 0) {
        return -1; // File exists
    }
    
    // Find free slot
    int slot = ramfs_find_free_slot();
    if (slot < 0) {
        return -2; // No free slots
    }
    
    // Create the file
    ram_file_t* file = &g_ramfs.files[slot];
    shell_strncpy(file->name, name, MAX_FILENAME_LEN - 1);
    file->name[MAX_FILENAME_LEN - 1] = '\0';
    
    if (!is_directory) {
        file->data = (char*)sys_malloc(256); // Initial size
        if (!file->data) return -3; // Out of memory
        file->capacity = 256;
        file->size = 0;
        file->data[0] = '\0';
    } else {
        file->data = NULL;
        file->capacity = 0;
        file->size = 0;
    }
    
    file->is_directory = is_directory;
    file->parent_index = g_ramfs.current_dir;
    file->in_use = 1;
    file->created_time = sys_time();
    
    g_ramfs.file_count++;
    return slot;
}

// Delete a file
int ramfs_delete(const char* name) {
    int file_index = ramfs_find_file(name, g_ramfs.current_dir);
    if (file_index < 0) return -1; // Not found
    
    ram_file_t* file = &g_ramfs.files[file_index];
    
    // Check if directory is empty
    if (file->is_directory) {
        for (int i = 0; i < MAX_FILES; i++) {
            if (g_ramfs.files[i].in_use && g_ramfs.files[i].parent_index == file_index) {
                return -2; // Directory not empty
            }
        }
    }
    
    // Free memory if it's a file
    if (file->data) {
        sys_free(file->data);
    }
    
    // Mark as free
    shell_memset(file, 0, sizeof(ram_file_t));
    g_ramfs.file_count--;
    
    return 0;
}

// Write data to file
int ramfs_write_file(const char* name, const char* data, uint32_t size) {
    int file_index = ramfs_find_file(name, g_ramfs.current_dir);
    if (file_index < 0) {
        // Create new file
        file_index = ramfs_create(name, 0);
        if (file_index < 0) return file_index;
    }
    
    ram_file_t* file = &g_ramfs.files[file_index];
    if (file->is_directory) return -1; // Can't write to directory
    
    // Expand buffer if needed
    if (size > file->capacity) {
        uint32_t new_capacity = ((size + 255) / 256) * 256; // Round up to 256 bytes
        char* new_data = (char*)sys_malloc(new_capacity);
        if (!new_data) return -3; // Out of memory
        
        if (file->data) {
            shell_memcpy(new_data, file->data, file->size);
            sys_free(file->data);
        }
        
        file->data = new_data;
        file->capacity = new_capacity;
    }
    
    // Write data
    shell_memcpy(file->data, data, size);
    file->size = size;
    if (size > 0) {
        file->data[size] = '\0'; // Null terminate for text files
    }
    
    return size;
}

// Read data from file
int ramfs_read_file(const char* name, char* buffer, uint32_t buffer_size) {
    int file_index = ramfs_find_file(name, g_ramfs.current_dir);
    if (file_index < 0) return -1; // Not found
    
    ram_file_t* file = &g_ramfs.files[file_index];
    if (file->is_directory) return -2; // Can't read directory as file
    
    uint32_t to_read = file->size < buffer_size ? file->size : buffer_size;
    shell_memcpy(buffer, file->data, to_read);
    
    return to_read;
}

// Get current directory path
void ramfs_get_current_path(char* buffer, uint32_t buffer_size) {
    if (g_ramfs.current_dir == 0) {
        shell_strncpy(buffer, "/", buffer_size);
        return;
    }
    
    // Build path backwards
    char temp_path[256];
    int pos = 255;
    temp_path[pos] = '\0';
    
    uint32_t dir_index = g_ramfs.current_dir;
    while (dir_index != 0) {
        ram_file_t* dir = &g_ramfs.files[dir_index];
        int name_len = shell_strlen(dir->name);
        
        pos -= name_len;
        if (pos < 0) break;
        shell_memcpy(&temp_path[pos], dir->name, name_len);
        
        pos--;
        if (pos < 0) break;
        temp_path[pos] = '/';
        
        dir_index = dir->parent_index;
    }
    
    shell_strncpy(buffer, &temp_path[pos], buffer_size);
}

//
// Auxiliary Function For Command Categorization
//

const char* get_command_category(const char* name) {
    // Basic Commands
    if (shell_strcmp(name, "help") == 0 || shell_strcmp(name, "clear") == 0 ||
        shell_strcmp(name, "echo") == 0 || shell_strcmp(name, "version") == 0) {
        return "Basic Commands";
    }
    // System Information
    else if (shell_strcmp(name, "memory") == 0 || shell_strcmp(name, "uptime") == 0 ||
             shell_strcmp(name, "cpuinfo") == 0 || shell_strcmp(name, "cpuid") == 0 ||
             shell_strcmp(name, "lsmod") == 0 || shell_strcmp(name, "dmesg") == 0) {
        return "System Information";
    }
    // File System
    else if (shell_strcmp(name, "ls") == 0 || shell_strcmp(name, "cd") == 0 ||
             shell_strcmp(name, "pwd") == 0 || shell_strcmp(name, "mkdir") == 0 ||
             shell_strcmp(name, "rm") == 0 || shell_strcmp(name, "cat") == 0 ||
             shell_strcmp(name, "edit") == 0 || shell_strcmp(name, "touch") == 0 ||
             shell_strcmp(name, "find") == 0 || shell_strcmp(name, "grep") == 0 ||
             shell_strcmp(name, "wc") == 0) {
        return "File System";
    }
    // Hardware & Debug
    else if (shell_strcmp(name, "memtest") == 0 || shell_strcmp(name, "ports") == 0 ||
             shell_strcmp(name, "interrupt") == 0 || shell_strcmp(name, "hexdump") == 0 ||
             shell_strcmp(name, "keytest") == 0 || shell_strcmp(name, "benchmark") == 0 ||
             shell_strcmp(name, "registers") == 0) {
        return "Hardware & Debug";
    }
    // System Calls
    else if (shell_strcmp(name, "syscall_test") == 0 || shell_strcmp(name, "malloc_test") == 0 ||
             shell_strcmp(name, "heap_info") == 0 || shell_strcmp(name, "syscall_info") == 0 ||
             shell_strcmp(name, "sleep") == 0) {
        return "System Calls";
    }
    // System Control
    else if (shell_strcmp(name, "reboot") == 0 || shell_strcmp(name, "panic") == 0) {
        return "System Control";
    }
    
    return "Other Commands";
}

//
// COMMAND IMPLEMENTATIONS
//

extern const ShellCommandEntry shell_commands[];

int cmd_help(int argc, char* argv[]) {
    printf("=== MiqOSoft Shell Commands ===\n\n");
    
    const char* categories[] = {
        "Basic Commands",
        "System Information", 
        "File System",
        "Hardware & Debug",
        "System Calls",
        "System Control"
    };
    
    const int LINES_PER_PAGE = 20;
    int line_count = 0;
    
    for (int cat = 0; cat < 6; cat++) {
        bool category_has_commands = false;
        
        // Check if category has commands
        for (int i = 0; shell_commands[i].name != NULL; i++) {
            if (shell_strcmp(get_command_category(shell_commands[i].name), categories[cat]) == 0) {
                category_has_commands = true;
                break;
            }
        }
        
        if (!category_has_commands) continue;
        
        printf("[%s]\n", categories[cat]);
        line_count++;
        
        for (int i = 0; shell_commands[i].name != NULL; i++) {
            if (shell_strcmp(get_command_category(shell_commands[i].name), categories[cat]) == 0) {
                printf("  %s - %s\n", shell_commands[i].name, shell_commands[i].description);
                line_count++;
                
                // Pagination
                if (line_count >= LINES_PER_PAGE) {
                    printf("\nPress any key to continue (q to quit)...");
                    char c;
                    bool key_pressed = false;
                    while (!key_pressed) {
                        if (keyboard_buffer_pop(&c)) {
                            key_pressed = true;
                            if (c == 'q' || c == 'Q') {
                                printf("\n");
                                return 0;
                            }
                        }
                    }
                    printf("\r                                           \r");
                    line_count = 0;
                }
            }
        }
        printf("\n");
        line_count++;
    }
    
    return 0;
}

int cmd_clear(int argc, char* argv[]) {
    VGA_clrscr();
    return 0;
}

int cmd_echo(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        printf("%s", argv[i]);
        if (i < argc - 1) printf(" ");
    }
    printf("\n");
    return 0;
}

int cmd_version(int argc, char* argv[]) {
    printf("MiqOSoft Kernel v1.0\n");
    printf("Architecture: i686 (32-bit)\n");
    printf("Built with: GCC cross-compiler\n");
    printf("Shell: MiqOSoft Shell v1.0\n");
    printf("Features: RAM File System, System calls, Memory management\n");
    return 0;
}

int cmd_test_all(int argc, char* argv[]) {
    printf("Running all system call tests...\n");
    syscall_run_all_tests();
    return 0;
}

int cmd_test_heap_integrity(int argc, char* argv[]) {
    printf("Testing heap integrity...\n");
    syscall_test_heap_integrity();
    return 0;
}

//
// System Information
//

int cmd_memory(int argc, char* argv[]) {
    printf("Memory Information:\n");
    printf("  Kernel Memory Range: 0x100000 - 0x200000 (1MB)\n");
    
    // Memory system constants (obtained from VFS/syscalls)
    const uint32_t HEAP_START = 0x400000;    // 4MB
    const uint32_t HEAP_SIZE = 0x100000;     // 1MB
    const uint32_t BLOCK_SIZE = 32;          // Minimum block size
    
    printf("  Heap Start: 0x%X (%dMB)\n", HEAP_START, HEAP_START / 1048576);
    printf("  Heap Size: %dKB (%d bytes)\n", HEAP_SIZE / 1024, HEAP_SIZE);
    printf("  Block Size: %d bytes\n", BLOCK_SIZE);
    
    // Perform allocation tests to verify the current heap state
    void* test_ptr1 = sys_malloc(1024);
    void* test_ptr2 = sys_malloc(2048);
    void* test_ptr3 = sys_malloc(4096);
    
    printf("  Memory Management: Simple block allocator\n");
    printf("\nHeap Status Test:\n");
    
    if (test_ptr1 && test_ptr2 && test_ptr3) {
        printf("  Test allocation 1: 0x%X (1KB)\n", (uint32_t)test_ptr1);
        printf("  Test allocation 2: 0x%X (2KB)\n", (uint32_t)test_ptr2);
        printf("  Test allocation 3: 0x%X (4KB)\n", (uint32_t)test_ptr3);
        
        // Calculate approximate space between blocks
        uint32_t block_overhead = 0;
        if (test_ptr2 > test_ptr1) {
            block_overhead = (uint32_t)test_ptr2 - (uint32_t)test_ptr1 - 1024;
        }
        
        printf("  Block overhead: ~%d bytes\n", block_overhead);
        
        // Free test memory
        sys_free(test_ptr1);
        sys_free(test_ptr2);
        sys_free(test_ptr3);
        
        printf("  Memory test successful - allocations freed\n");
    } else {
        printf("  Memory test failed - could not allocate test blocks\n");
        printf("  Heap may be full or corrupted\n");
        
        // Release any successful allocation
        if (test_ptr1) sys_free(test_ptr1);
        if (test_ptr2) sys_free(test_ptr2);
        if (test_ptr3) sys_free(test_ptr3);
    }
    
    return 0;
}

int cmd_uptime(int argc, char* argv[]) {
    uint32_t milliseconds = sys_time();
    uint32_t seconds = milliseconds / 1000;
    uint32_t minutes = seconds / 60;
    uint32_t hours = minutes / 60;

    seconds %= 60;
    minutes %= 60;
    
    printf("System has been up for: %u:%u:%u\n", hours, minutes, seconds);

    return 0;
}

int cmd_cpuinfo(int argc, char* argv[]) {
    printf("CPU Information:\n");
    printf("  Architecture: i686 (32-bit x86)\n");
    printf("  Instruction Set: x86\n");
    printf("  Protected Mode: Enabled\n");
    printf("  Paging: Disabled\n");
    printf("  Interrupts: Available\n");
    printf("  FPU: Available (if present)\n");
    printf("  For detailed info use: cpuid\n");
    return 0;
}

int cmd_cpuid(int argc, char* argv[]) {
    uint32_t eax, ebx, ecx, edx;
    
    // CPUID leaf 0 - Vendor ID
    __asm__ volatile(
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(0)
    );
    
    char vendor[13];
    *((uint32_t*)&vendor[0]) = ebx;
    *((uint32_t*)&vendor[4]) = edx;
    *((uint32_t*)&vendor[8]) = ecx;
    vendor[12] = '\0';
    
    printf("CPU Information:\n");
    printf("  Vendor: %s\n", vendor);
    printf("  Max CPUID level: %u\n", eax);
    
    if (eax >= 1) {
        // CPUID leaf 1 - Feature flags
        __asm__ volatile(
            "cpuid"
            : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
            : "a"(1)
        );
        
        printf("  Family: %u\n", (eax >> 8) & 0xF);
        printf("  Model: %u\n", (eax >> 4) & 0xF);
        printf("  Stepping: %u\n", eax & 0xF);
        
        printf("  Features (EDX): ");
        if (edx & (1 << 0)) printf("FPU ");
        if (edx & (1 << 4)) printf("TSC ");
        if (edx & (1 << 15)) printf("CMOV ");
        if (edx & (1 << 23)) printf("MMX ");
        if (edx & (1 << 25)) printf("SSE ");
        if (edx & (1 << 26)) printf("SSE2 ");
        printf("\n");
        
        printf("  Features (ECX): ");
        if (ecx & (1 << 0)) printf("SSE3 ");
        if (ecx & (1 << 9)) printf("SSSE3 ");
        if (ecx & (1 << 19)) printf("SSE4.1 ");
        if (ecx & (1 << 20)) printf("SSE4.2 ");
        printf("\n");
    }
    
    return 0;
}

int cmd_dmesg(int argc, char* argv[]) {
    if (argc == 1) {
        // Show messages
        display_kernel_messages();
    } else if (argc == 2) {
        if (shell_strcmp(argv[1], "--help") == 0 || shell_strcmp(argv[1], "-h") == 0) {
            printf("dmesg - Display kernel boot messages\n");
            printf("Usage:\n");
            printf("  dmesg           Show boot messages\n");
            printf("  dmesg --clear   Clear message buffer\n");
            printf("  dmesg --stats   Show buffer statistics\n");
            printf("  dmesg --test    Add test message\n");
            printf("  dmesg --help    Show this help\n");
        } 
        else if (shell_strcmp(argv[1], "--clear") == 0) {
            dmesg_clear();
        }
        else if (shell_strcmp(argv[1], "--stats") == 0) {
            dmesg_stats();
        }
        else if (shell_strcmp(argv[1], "--test") == 0) {
            // Add test message
            kernel_add_message('T', "test", "This is a test message from shell");
            printf("Test message added to dmesg buffer\n");
        }
        else {
            printf("dmesg: Unknown option '%s'\n", argv[1]);
            printf("Use 'dmesg --help' for help\n");
            return 1;
        }
    } else {
        printf("dmesg: Too many arguments\n");
        return 1;
    }

    return 0;
}

//
// FUNCTIONAL FILE SYSTEM COMMANDS
//

int cmd_ls(int argc, char* argv[]) {
    if (!g_ramfs.initialized) ramfs_init();
    
    char current_path[64];
    ramfs_get_current_path(current_path, sizeof(current_path));

    printf("Directory listing for %s:\n", current_path);
    printf("%s %s\n", "Type      ", "Name      ");
    printf("------------------------------------------------------------\n");

    int count = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        if (g_ramfs.files[i].in_use && g_ramfs.files[i].parent_index == g_ramfs.current_dir) {
            ram_file_t* file = &g_ramfs.files[i];
            
            printf("%s %s\n",
                   file->is_directory ? "DIR       " : "FILE      ",
                   file->name);
            count++;
        }
    }
    
    if (count == 0) {
        printf("Directory is empty\n");
    }
    
    return 0;
}

int cmd_cd(int argc, char* argv[]) {
    if (!g_ramfs.initialized) ramfs_init();
    
    if (argc < 2) {
        // Show current directory
        char current_path[64];
        ramfs_get_current_path(current_path, sizeof(current_path));
        printf("Current directory: %s\n", current_path);
        return 0;
    }
    
    const char* target = argv[1];
    
    // Handle special cases
    if (shell_strcmp(target, "..") == 0) {
        // Go to parent directory
        if (g_ramfs.current_dir != 0) {
            g_ramfs.current_dir = g_ramfs.files[g_ramfs.current_dir].parent_index;
        }
        return 0;
    } else if (shell_strcmp(target, "/") == 0) {
        // Go to root
        g_ramfs.current_dir = 0;
        return 0;
    }
    
    // Find directory
    int dir_index = ramfs_find_file(target, g_ramfs.current_dir);
    if (dir_index < 0) {
        printf("cd: Directory '%s' not found\n", target);
        return 1;
    }
    
    if (!g_ramfs.files[dir_index].is_directory) {
        printf("cd: '%s' is not a directory\n", target);
        return 1;
    }
    
    g_ramfs.current_dir = dir_index;
    return 0;
}

int cmd_pwd(int argc, char* argv[]) {
    if (!g_ramfs.initialized) ramfs_init();
    
    char current_path[64];
    ramfs_get_current_path(current_path, sizeof(current_path));
    printf("%s\n", current_path);
    
    return 0;
}

int cmd_mkdir(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: mkdir <directory_name>\n");
        return 1;
    }
    
    if (!g_ramfs.initialized) ramfs_init();
    
    int result = ramfs_create(argv[1], 1); // 1 = directory
    
    switch (result) {
        case -1:
            printf("mkdir: Directory '%s' already exists\n", argv[1]);
            return 1;
        case -2:
            printf("mkdir: No free slots available\n");
            return 1;
        case -3:
            printf("mkdir: Out of memory\n");
            return 1;
        default:
            if (result >= 0) {
                printf("Directory '%s' created successfully\n", argv[1]);
                return 0;
            } else {
                printf("mkdir: Unknown error (%d)\n", result);
                return 1;
            }
    }
}

int cmd_rm(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: rm <filename>\n");
        return 1;
    }
    
    if (!g_ramfs.initialized) ramfs_init();
    
    int result = ramfs_delete(argv[1]);
    
    switch (result) {
        case 0:
            printf("'%s' deleted successfully\n", argv[1]);
            return 0;
        case -1:
            printf("rm: '%s' not found\n", argv[1]);
            return 1;
        case -2:
            printf("rm: Directory '%s' is not empty\n", argv[1]);
            return 1;
        default:
            printf("rm: Unknown error (%d)\n", result);
            return 1;
    }
}

int cmd_cat(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: cat <filename>\n");
        return 1;
    }
    
    if (!g_ramfs.initialized) ramfs_init();
    
    char buffer[MAX_FILE_SIZE];
    int bytes_read = ramfs_read_file(argv[1], buffer, sizeof(buffer) - 1);
    
    if (bytes_read < 0) {
        printf("cat: Cannot read '%s' (error %d)\n", argv[1], bytes_read);
        return 1;
    }
    
    buffer[bytes_read] = '\0';
    printf("%s", buffer);
    
    return 0;
}

int cmd_find(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: find <pattern>\n");
        return 1;
    }
    
    if (!g_ramfs.initialized) ramfs_init();
    
    printf("Searching for files matching '%s':\n", argv[1]);
    
    bool found = false;
    for (int i = 0; i < MAX_FILES; i++) {
        if (g_ramfs.files[i].in_use) {
            if (shell_strstr(g_ramfs.files[i].name, argv[1]) != NULL) {
                printf("%s %s\n", 
                       g_ramfs.files[i].is_directory ? "[DIR] " : "[FILE] ",
                       g_ramfs.files[i].name);
                found = true;
            }
        }
    }
    
    if (!found) {
        printf("No files found matching '%s'\n", argv[1]);
    }
    
    return 0;
}

int cmd_grep(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Usage: grep <pattern> <filename>\n");
        return 1;
    }
    
    if (!g_ramfs.initialized) ramfs_init();
    
    char buffer[MAX_FILE_SIZE];
    int bytes_read = ramfs_read_file(argv[2], buffer, sizeof(buffer) - 1);
    
    if (bytes_read < 0) {
        printf("grep: Cannot read '%s'\n", argv[2]);
        return 1;
    }
    
    buffer[bytes_read] = '\0';
    
    // Search line by line
    char* line_start = buffer;
    int line_num = 1;
    bool found = false;
    
    while (*line_start) {
        char* line_end = shell_strchr(line_start, '\n');
        
        if (line_end) {
            *line_end = '\0';
        }
        
        if (shell_strstr(line_start, argv[1]) != NULL) {
            printf("%d: %s\n", line_num, line_start);
            found = true;
        }
        
        if (!line_end) break;
        
        *line_end = '\n';
        line_start = line_end + 1;
        line_num++;
    }
    
    if (!found) {
        printf("Pattern '%s' not found in '%s'\n", argv[1], argv[2]);
    }
    
    return 0;
}

int cmd_wc(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: wc <filename>\n");
        return 1;
    }
    
    if (!g_ramfs.initialized) ramfs_init();
    
    char buffer[MAX_FILE_SIZE];
    int bytes_read = ramfs_read_file(argv[1], buffer, sizeof(buffer) - 1);
    
    if (bytes_read < 0) {
        printf("wc: Cannot read '%s'\n", argv[1]);
        return 1;
    }
    
    buffer[bytes_read] = '\0';
    
    int lines = 0, words = 0, chars = bytes_read;
    bool in_word = false;
    
    for (int i = 0; i < bytes_read; i++) {
        if (buffer[i] == '\n') {
            lines++;
        }
        
        if (buffer[i] == ' ' || buffer[i] == '\t' || buffer[i] == '\n') {
            if (in_word) {
                words++;
                in_word = false;
            }
        } else {
            in_word = true;
        }
    }
    
    // Count last word if file doesn't end with whitespace
    if (in_word) {
        words++;
    }
    
    printf("  %d lines, %d words, %d characters, Filename: %s\n", lines, words, chars, argv[1]);
    
    return 0;
}

int cmd_touch(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: touch <filename>\n");
        return 1;
    }
    
    if (!g_ramfs.initialized) ramfs_init();
    
    // Check if file exists
    if (ramfs_find_file(argv[1], g_ramfs.current_dir) >= 0) {
        printf("File '%s' already exists\n", argv[1]);
        return 0;
    }
    
    // Create empty file
    int result = ramfs_write_file(argv[1], "", 0);
    if (result >= 0) {
        printf("Empty file '%s' created\n", argv[1]);
        return 0;
    } else {
        printf("touch: Cannot create '%s' (error %d)\n", argv[1], result);
        return 1;
    }
}

int cmd_edit(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: edit <filename>\n");
        return 1;
    }
    
    if (!g_ramfs.initialized) ramfs_init();
    
    char* filename = argv[1];
    
    // Try to read existing file
    char buffer[MAX_FILE_SIZE];
    int bytes_read = ramfs_read_file(filename, buffer, sizeof(buffer) - 1);
    
    if (bytes_read >= 0) {
        buffer[bytes_read] = '\0';
        printf("Editing existing file '%s' (%d bytes)\n", filename, bytes_read);
        printf("Current content:\n%s\n", buffer);
    } else {
        printf("Creating new file '%s'\n", filename);
        buffer[0] = '\0';
    }
    
    printf("\n=== Simple Editor ===\n");
    printf("Enter text (type 'SAVE' on new line to save, 'QUIT' to exit):\n\n");
    
    char edit_buffer[MAX_FILE_SIZE];
    int content_length = 0;
    char line_buffer[256];
    
    // Copy existing content if any
    if (bytes_read > 0) {
        shell_memcpy(edit_buffer, buffer, bytes_read);
        content_length = bytes_read;
    }
    
    while (1) {
    // Print prompt using regular output
    printf("> ");

        shell_memset(line_buffer, 0, sizeof(line_buffer));

        // Read input line with echo and backspace handling using local cursor
        int i = 0;
        char c;
        while (i < 255) {
            c = shell_getc();
            if (c == '\n') {
                // Move to next line visually
                putc('\n');
                break;
            }

            if (c == '\b') {
                // Backspace: remove last character if any and update display
                if (i > 0) {
                    i--;
                    // Move cursor one position left and overwrite with space
                    int cur_x = VGA_get_cursor_x();
                    int cur_y = VGA_get_cursor_y();
                    if (cur_x > 0) {
                        // Write space directly to VGA buffer at the target position
                        VGA_putchr(cur_x - 1, cur_y, ' ');
                        // Update software cursor position and hardware cursor
                        extern int g_ScreenX, g_ScreenY;
                        g_ScreenX = cur_x - 1;
                        g_ScreenY = cur_y;
                        VGA_setcursor(g_ScreenX, g_ScreenY);
                    }
                }
                continue;
            }

            // Printable characters: store and echo using putc
            line_buffer[i++] = c;
            if (c >= 32 && c <= 126) putc(c);
        }
        line_buffer[i] = '\0';
        
        if (shell_strcmp(line_buffer, "SAVE") == 0) {
            int result = ramfs_write_file(filename, edit_buffer, content_length);
            if (result >= 0) {
                printf("File '%s' saved (%d bytes)\n", filename, result);
            } else {
                printf("Error saving file: %d\n", result);
            }
            break;
        } else if (shell_strcmp(line_buffer, "QUIT") == 0) {
            printf("Editor exited without saving\n");
            break;
        }
        
        // Add line to buffer
        int line_len = shell_strlen(line_buffer);
        if (content_length + line_len + 2 < MAX_FILE_SIZE) {
            shell_memcpy(edit_buffer + content_length, line_buffer, line_len);
            content_length += line_len;
            edit_buffer[content_length++] = '\n';
            edit_buffer[content_length] = '\0';
        } else {
            printf("Error: File size limit reached (%d bytes)\n", MAX_FILE_SIZE);
            break;
        }
    }
    
    return 0;
}

//
// Hardware & Debug Commands
//

int cmd_memtest(int argc, char* argv[]) {
    printf("Basic memory test...\n");
    
    uint8_t* test_ptr = (uint8_t*)0x200000; // 2MB
    uint8_t patterns[] = {0x00, 0xFF, 0xAA, 0x55, 0xCC, 0x33};
    int pattern_count = sizeof(patterns);
    int test_size = 1024;
    bool all_passed = true;
    
    for (int p = 0; p < pattern_count; p++) {
        printf("  Testing pattern 0x%X... ", patterns[p]);
        
        // Write pattern
        for (int i = 0; i < test_size; i++) {
            test_ptr[i] = patterns[p];
        }
        
        // Verify pattern
        bool success = true;
        for (int i = 0; i < test_size; i++) {
            if (test_ptr[i] != patterns[p]) {
                success = false;
                break;
            }
        }
        
        printf("%s\n", success ? "PASS" : "FAIL");
        if (!success) all_passed = false;
    }
    
    printf("Memory test %s\n", all_passed ? "PASSED" : "FAILED");
    return all_passed ? 0 : 1;
}

int cmd_interrupt(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: interrupt <enable|disable|status>\n");
        return 1;
    }
    
    if (shell_strcmp(argv[1], "enable") == 0) {
        i686_EnableInterrupts();
        printf("Interrupts enabled\n");
    } else if (shell_strcmp(argv[1], "disable") == 0) {
        i686_DisableInterrupts();
        printf("Interrupts disabled\n");
    } else if (shell_strcmp(argv[1], "status") == 0) {
        uint32_t eflags;
        __asm__ volatile("pushfl; popl %0" : "=r"(eflags));
        bool interrupts_enabled = (eflags & 0x200) != 0;
        printf("Interrupts: %s\n", interrupts_enabled ? "ENABLED" : "DISABLED");
    } else {
        printf("Invalid option: %s\n", argv[1]);
        return 1;
    }
    
    return 0;
}

int cmd_hexdump(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: hexdump <filename_or_address> [length]\n");
        printf("Examples:\n");
        printf("  hexdump test.txt     - Dump file content\n");
        printf("  hexdump 0x100000     - Dump 256 bytes from memory\n");
        return 1;
    }
    
    if (!g_ramfs.initialized) ramfs_init();
    
    // Check if it's a memory address (starts with 0x) or filename
    if (argv[1][0] == '0' && argv[1][1] == 'x') {
        // Memory dump
        uint32_t address = hex_str_to_int(argv[1]);
        uint32_t length = 256;
        
        if (argc > 2) {
            length = dec_str_to_int(argv[2]);
        }
        
        if (length > 4096) {
            printf("Warning: Length limited to 4096 bytes\n");
            length = 4096;
        }
        
        printf("Hexdump of memory at 0x%X (%d bytes):\n\n", address, length);
        
        uint8_t* memory = (uint8_t*)address;
        uint32_t offset = 0;
        
        while (offset < length) {
            printf("%x  ", address + offset);
            
            for (int i = 0; i < 16 && offset + i < length; i++) {
                printf("%x ", memory[offset + i]);
                if (i == 7) printf(" ");
            }
            
            for (int i = offset + 16 > length ? length - offset : 16; i < 16; i++) {
                printf("   ");
                if (i == 7) printf(" ");
            }
            
            printf(" |");
            
            for (int i = 0; i < 16 && offset + i < length; i++) {
                uint8_t byte = memory[offset + i];
                printf("%c", (byte >= 32 && byte <= 126) ? byte : '.');
            }
            
            printf("|\n");
            offset += 16;
        }
    } else {
        // File dump
        char buffer[MAX_FILE_SIZE];
        int bytes_read = ramfs_read_file(argv[1], buffer, sizeof(buffer));
        
        if (bytes_read < 0) {
            printf("hexdump: Cannot read '%s'\n", argv[1]);
            return 1;
        }
        
        printf("Hexdump of file '%s' (%d bytes):\n\n", argv[1], bytes_read);
        
        uint32_t offset = 0;
        while (offset < bytes_read) {
            printf("%x  ", offset);
            
            for (int i = 0; i < 16 && offset + i < bytes_read; i++) {
                printf("%x ", (uint8_t)buffer[offset + i]);
                if (i == 7) printf(" ");
            }
            
            for (int i = offset + 16 > bytes_read ? bytes_read - offset : 16; i < 16; i++) {
                printf("   ");
                if (i == 7) printf(" ");
            }
            
            printf(" |");
            
            for (int i = 0; i < 16 && offset + i < bytes_read; i++) {
                uint8_t byte = (uint8_t)buffer[offset + i];
                printf("%c", (byte >= 32 && byte <= 126) ? byte : '.');
            }
            
            printf("|\n");
            offset += 16;
        }
    }
    
    return 0;
}

int cmd_keytest(int argc, char* argv[]) {
    printf("Keyboard test mode - press keys to see scancodes\n");
    printf("Press ESC to exit\n\n");
    
    while (1) {
        keyboard_process_buffer();
        
        if (i686_inb(0x64) & 0x01) { // Available data
            uint8_t scancode = i686_inb(0x60);
            
            if (scancode == 0x01) { // ESC
                // Clear any pending characters so they don't appear in the shell
                keyboard_clear_buffer();
                keyboard_reset_dead_state();
                printf("\nExiting keyboard test\n");
                break;
            }
            
            printf("Scancode: 0x%X", scancode);
            if (scancode & 0x80) {
                printf(" (key released)");
            } else {
                printf(" (key pressed)");
            }
            
            // Show key if known
            if (!(scancode & 0x80)) {
                char ascii = keyboard_scancode_to_ascii(scancode);
                if (ascii >= 32 && ascii <= 126) {
                    printf(" = '%c'", ascii);
                }
            }
            printf("\n");
        }
    }
    
    return 0;
}

int cmd_benchmark(int argc, char* argv[]) {
    printf("Running CPU benchmark suite...\n\n");
    
    // Arithmetic Test
    printf("1. Arithmetic operations test: ");
    volatile uint32_t result = 0;
    for (uint32_t i = 0; i < 1000000; i++) {
        result += i * 2 + 1;
    }
    printf("DONE (result: %u)\n", result);
    
    // Memory Test
    printf("2. Memory access test: ");
    volatile uint8_t* mem = (volatile uint8_t*)0x200000;
    for (uint32_t i = 0; i < 100000; i++) {
        mem[i % 1024] = (uint8_t)(i & 0xFF);
        result += mem[i % 1024];
    }
    printf("DONE\n");
    
    // Function Call Testing
    printf("3. Function call test: ");
    for (uint32_t i = 0; i < 50000; i++) {
        VGA_get_cursor_x();
    }
    printf("DONE\n");
    
    // Logical Operations Test
    printf("4. Logical operations test: ");
    for (uint32_t i = 0; i < 500000; i++) {
        result ^= (i << 1) | (i >> 1);
        result &= 0xAAAAAAAA;
        result |= 0x55555555;
    }
    printf("DONE\n");
    
    printf("\nBenchmark completed successfully\n");
    return 0;
}

int cmd_registers(int argc, char* argv[]) {
    uint32_t eax, ebx, ecx, edx, esp, ebp, esi, edi, eflags;
    
    __asm__ volatile(
        "movl %%eax, %0\n"
        "movl %%ebx, %1\n"
        "movl %%ecx, %2\n"
        "movl %%edx, %3\n"
        "movl %%esp, %4\n"
        "movl %%ebp, %5\n"
        "movl %%esi, %6\n"
        "movl %%edi, %7\n"
        "pushfl\n"
        "popl %8\n"
        : "=m"(eax), "=m"(ebx), "=m"(ecx), "=m"(edx),
          "=m"(esp), "=m"(ebp), "=m"(esi), "=m"(edi), "=m"(eflags)
        :
        : "memory"
    );
    
    printf("CPU Registers:\n");
    printf("  General Purpose:\n");
    printf("    EAX: 0x%X    EBX: 0x%X\n", eax, ebx);
    printf("    ECX: 0x%X    EDX: 0x%X\n", ecx, edx);
    printf("  Index Registers:\n");
    printf("    ESI: 0x%X    EDI: 0x%X\n", esi, edi);
    printf("  Stack Pointers:\n");
    printf("    ESP: 0x%X    EBP: 0x%X\n", esp, ebp);
    printf("  Flags:\n");
    printf("    EFLAGS: 0x%X\n", eflags);
    
    printf("  Flag Bits: ");
    if (eflags & 0x001) printf("CF ");
    if (eflags & 0x004) printf("PF ");
    if (eflags & 0x010) printf("AF ");
    if (eflags & 0x040) printf("ZF ");
    if (eflags & 0x080) printf("SF ");
    if (eflags & 0x200) printf("IF ");
    if (eflags & 0x400) printf("DF ");
    if (eflags & 0x800) printf("OF ");
    printf("\n");
    
    return 0;
}

//
// System Call Commands
//

int cmd_syscall_test(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: syscall_test <test_type>\n");
        printf("Available tests:\n");
        printf("  all       - Run comprehensive test suite\n");
        printf("  basic     - Test basic syscalls (print, getpid, time)\n");
        printf("  memory    - Test malloc/free operations\n");
        printf("  heap      - Test heap integrity\n");
        printf("  stress    - Run stress tests\n");
        return 1;
    }
    
    if (shell_strcmp(argv[1], "all") == 0) {
        syscall_run_all_tests();
    } else if (shell_strcmp(argv[1], "basic") == 0) {
        printf("=== Basic Syscall Test ===\n\n");
        
        printf("Testing sys_print: ");
        int result = sys_print("Hello from syscall!\n");
        printf("Result: %d\n\n", result);
        
        int32_t pid = sys_getpid();
        printf("Current PID: %d\n\n", pid);
        
        uint32_t time = sys_time();
        printf("Current time: %u\n\n", time);
        
    } else if (shell_strcmp(argv[1], "memory") == 0) {
        printf("=== Memory Syscall Test ===\n\n");
        
        printf("Testing malloc:\n");
        void* ptr1 = sys_malloc(256);
        void* ptr2 = sys_malloc(512);
        printf("  Allocated 256 bytes at: 0x%X\n", (uint32_t)ptr1);
        printf("  Allocated 512 bytes at: 0x%X\n", (uint32_t)ptr2);
        
        if (ptr1 && ptr2) {
            strcpy((char*)ptr1, "Test data 1");
            strcpy((char*)ptr2, "Test data 2 - longer string");
            
            printf("  Data in ptr1: %s\n", (char*)ptr1);
            printf("  Data in ptr2: %s\n", (char*)ptr2);
            
            printf("\nTesting free:\n");
            int free1 = sys_free(ptr1);
            int free2 = sys_free(ptr2);
            printf("  Free ptr1 result: %d\n", free1);
            printf("  Free ptr2 result: %d\n", free2);
        }
        
    } else if (shell_strcmp(argv[1], "heap") == 0) {
        syscall_test_heap_integrity();
        
    } else if (shell_strcmp(argv[1], "stress") == 0) {
        printf("=== Stress Test ===\n\n");
        
        printf("Memory stress test (100 allocations):\n");
        int success = 0, failed = 0;
        for (int i = 0; i < 100; i++) {
            void* ptr = sys_malloc(64 + (i % 200));
            if (ptr) {
                success++;
                sys_free(ptr);
            } else {
                failed++;
            }
        }
        printf("  Success: %d, Failed: %d\n\n", success, failed);
        
    } else {
        printf("Unknown test type: %s\n", argv[1]);
        return 1;
    }
    
    return 0;
}

int cmd_malloc_test(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: malloc_test <size_in_bytes>\n");
        printf("Example: malloc_test 1024\n");
        return 1;
    }
    
    int size = dec_str_to_int(argv[1]);
    
    if (size <= 0) {
        printf("Invalid size: %s\n", argv[1]);
        return 1;
    }
    
    printf("Allocating %d bytes...\n", size);
    void* ptr = sys_malloc(size);
    
    if (ptr) {
        printf("Success! Allocated at address: 0x%X\n", (uint32_t)ptr);
        
        // Write test data
        char* data = (char*)ptr;
        for (int i = 0; i < size && i < 100; i++) {
            data[i] = 'A' + (i % 26);
        }
        data[size < 100 ? size - 1 : 99] = '\0';
        
        printf("Test data written: %.50s%s\n", data, size > 50 ? "..." : "");
        
        printf("Freeing memory...\n");
        int result = sys_free(ptr);
        printf("Free result: %d\n", result);
        
    } else {
        printf("Allocation failed - out of memory\n");
    }
    
    return 0;
}

int cmd_heap_info(int argc, char* argv[]) {
    printf("=== Heap Information ===\n\n");
    
    printf("Heap configuration:\n");
    printf("  Start address: 0x%X\n", 0x400000);
    printf("  Size: %d KB (%d bytes)\n", 0x100000 / 1024, 0x100000);
    printf("  Block size: %d bytes\n", 32);
    printf("  Management: Simple block allocator\n");
    
    printf("\nTesting current heap state:\n");
    
    void* test_ptrs[5];
    int sizes[] = {64, 128, 256, 512, 1024};
    
    for (int i = 0; i < 5; i++) {
        test_ptrs[i] = sys_malloc(sizes[i]);
        if (test_ptrs[i]) {
            printf("  Allocated %d bytes at 0x%X\n", sizes[i], (uint32_t)test_ptrs[i]);
        } else {
            printf("  Failed to allocate %d bytes\n", sizes[i]);
        }
    }
    
    printf("\nCleaning up test allocations:\n");
    for (int i = 0; i < 5; i++) {
        if (test_ptrs[i]) {
            sys_free(test_ptrs[i]);
            printf("  Freed block at 0x%X\n", (uint32_t)test_ptrs[i]);
        }
    }
    
    return 0;
}

int cmd_sleep_test(int argc, char* argv[]) {
    int milliseconds = 1000; // Default 1 second
    
    if (argc > 1) {
        milliseconds = dec_str_to_int(argv[1]);
        if (milliseconds <= 0) {
            milliseconds = 1000;
        }
    }
    
    printf("Sleeping for %d milliseconds...\n", milliseconds);
    uint32_t start_time = sys_time();
    
    int result = sys_sleep(milliseconds);
    
    uint32_t end_time = sys_time();
    printf("Sleep completed (result: %d)\n", result);
    printf("Time difference: %u units\n", end_time - start_time);
    
    return 0;
}

//
// System Control Commands
//

int cmd_reboot(int argc, char* argv[]) {
    printf("Rebooting system...\n");
    printf("Goodbye!\n");
    
    // Use the keyboard port for reboot
    i686_outb(0x64, 0xFE);
    
    // If reboot failed, use alternative method
    printf("Primary reboot method failed, trying alternative...\n");
    
    // Triple fault method
    __asm__ volatile(
        "cli\n"
        "lidt %0\n"
        "int $0x03\n"
        :
        : "m"(*((void**)0))
        : "memory"
    );
    
    return 0;
}

int cmd_panic(int argc, char* argv[]) {
    const char* message = "Manual kernel panic triggered from shell";
    
    if (argc > 1) {
        message = argv[1];
    }
    
    printf("Triggering kernel panic: %s\n", message);
    printf("This will halt the system...\n");
    
    // Trigger panic
    i686_Panic();
    
    // Should never reach here
    return 0;
}

//
// COMMAND TABLE - ALL COMMANDS RESTORED
//

const ShellCommandEntry shell_commands[] = {
    // Basic Commands
    {"help",            "Show available commands",                          cmd_help},
    {"clear",           "Clear the screen",                                 cmd_clear},
    {"echo",            "Display a line of text",                           cmd_echo},
    {"version",         "Show OS version information",                      cmd_version},
    {"test",            "Run all syscall tests",                            cmd_test_all},
    {"heap_test",       "Run heap integrity test",                          cmd_test_heap_integrity},
    
    // System Information
    {"memory",          "Show memory information",                          cmd_memory},
    {"uptime",          "Show system uptime",                               cmd_uptime},
    {"cpuinfo",         "Show CPU information",                             cmd_cpuinfo},
    {"cpuid",           "Show detailed CPU information via CPUID",          cmd_cpuid},
    {"dmesg",           "Show kernel messages",                             cmd_dmesg},
    
    // File System (RAM-based)
    {"ls",              "List directory contents",                          cmd_ls},
    {"cd",              "Change directory",                                 cmd_cd},
    {"pwd",             "Print working directory",                          cmd_pwd},
    {"mkdir",           "Create directory",                                 cmd_mkdir},
    {"rm",              "Remove file or directory",                         cmd_rm},
    {"cat",             "Display file contents",                            cmd_cat},
    {"edit",            "Edit text file",                                   cmd_edit},
    {"touch",           "Create empty file",                                cmd_touch},
    {"find",            "Find files by name pattern",                       cmd_find},
    {"grep",            "Search text in files",                             cmd_grep},
    {"wc",              "Count lines, words and characters",                cmd_wc},
    
    // Hardware & Debug
    {"memtest",         "Run basic memory test",                            cmd_memtest},
    {"interrupt",       "Control interrupt state",                          cmd_interrupt},
    {"hexdump",         "Display file/memory in hexadecimal",               cmd_hexdump},
    {"keytest",         "Test keyboard input (shows scancodes)",            cmd_keytest},
    {"benchmark",       "Run CPU benchmark",                                cmd_benchmark},
    {"registers",       "Show CPU register values",                         cmd_registers},
    
    // System Calls
    {"syscall_test",    "Test system call functionality",                   cmd_syscall_test},
    {"malloc_test",     "Test memory allocation via syscall",               cmd_malloc_test},
    {"heap_info",       "Show heap information and test",                   cmd_heap_info},
    {"sleep",           "Test sleep syscall",                               cmd_sleep_test},
    
    // System Control
    {"reboot",          "Restart the system",                               cmd_reboot},
    {"panic",           "Trigger a kernel panic (for testing)",             cmd_panic},
    {"exit",            "Power off / halt the system",                     cmd_exit},
    
    // Terminator
    {NULL, NULL, NULL}
};

//
// COMMAND MANAGEMENT FUNCTIONS
//

int get_shell_command_count(void) {
    int count = 0;
    while (shell_commands[count].name != NULL) count++;
    return count;
}

const ShellCommandEntry* find_shell_command(const char* name) {
    for (int i = 0; shell_commands[i].name != NULL; i++) {
        if (shell_strcmp(shell_commands[i].name, name) == 0) {
            return &shell_commands[i];
        }
    }
    return NULL;
}

int cmd_exit(int argc, char* argv[]) {
    printf("Shutting down...\n");

    i686_outb(0x604, 0x00);
    i686_outb(0x605, 0x20);

    i686_DisableInterrupts();
    while (1) {
        i686_hlt();
    }

    return 0;
}