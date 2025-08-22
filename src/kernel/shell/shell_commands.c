#include "shell.h"
#include "syscall.h"
#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <io.h>
#include <vga_text.h>
#include <keyboard.h>
#include <debug.h>
#include <shell_utils.h>
#include <shell_commands.h>

// ============================================================================
// DECLARACIONES EXTERNAS
// ============================================================================

// Funciones de shell.c que necesitamos
extern int shell_strlen(const char* str);
extern int shell_strcmp(const char* s1, const char* s2);
extern char* shell_strchr(const char* str, int c);
extern char* shell_strstr(const char* haystack, const char* needle);
extern char* shell_strncpy(char* dest, const char* src, int n);

// Sistema de archivos virtual de shell.c
extern VirtualFile virtual_fs[];

// Funciones de syscall_test.c
extern void syscall_run_all_tests(void);
extern void syscall_test_heap_integrity(void);

// ============================================================================
// UTILIDADES AUXILIARES
// ============================================================================

// Función auxiliar para conversión hexadecimal
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

// Función auxiliar para conversión decimal
uint32_t dec_str_to_int(const char* dec_str) {
    uint32_t result = 0;
    while (*dec_str >= '0' && *dec_str <= '9') {
        result = result * 10 + (*dec_str - '0');
        dec_str++;
    }
    return result;
}

// ============================================================================
// COMANDOS BÁSICOS DE SISTEMA
// ============================================================================

int cmd_help(int argc, char* argv[]) {
    printf("=== MiqOSoft Shell Commands ===\n\n");
    
    // Definir comandos con categorías
    struct {
        const char* category;
        const char* commands[20];
    } categories[] = {
        {
            "Basic Commands",
            {"help - Show this help", "clear - Clear screen", "echo <text> - Display text",
             "version - Show OS version", "exit - Exit shell", "history - Show command history", NULL}
        },
        {
            "System Information", 
            {"memory - Memory information", "uptime - System uptime", "cpuinfo - CPU information",
             "cpuid - Detailed CPU info", "lsmod - List kernel modules", "dmesg - Kernel messages",
             "ps - Show processes", NULL}
        },
        {
            "File System",
            {"ls - List files", "cat <file> - Show file content", "mkdir <dir> - Create directory",
             "rm <file> - Remove file", "find <pattern> - Find files", "grep <pattern> <file> - Search in file",
             "wc <file> - Count lines/words", NULL}
        },
        {
            "Hardware & Debug",
            {"ports <read|write> - I/O port access", "interrupt <enable|disable> - Control interrupts",
             "hexdump <addr> <len> - Memory dump", "keytest - Keyboard test", "benchmark - CPU benchmark",
             "registers - Show CPU registers", "stack - Show stack dump", "memtest - Memory test", NULL}
        },
        {
            "System Calls",
            {"syscall_test <type> - Test syscalls", "malloc_test <size> - Test malloc",
             "file_create <name> - Create file", "file_read <name> - Read file", "heap_info - Heap status",
             "sleep_test <ms> - Test sleep", "syscall_info - Syscall documentation", NULL}
        },
        {
            "System Control",
            {"reboot - Restart system", "panic - Trigger kernel panic", NULL}
        }
    };
    
    const int LINES_PER_PAGE = 20;
    int line_count = 0;
    
    for (int cat = 0; cat < 6; cat++) {
        printf("[%s]\n", categories[cat].category);
        line_count++;
        
        for (int cmd = 0; categories[cat].commands[cmd] != NULL; cmd++) {
            printf("  %s\n", categories[cat].commands[cmd]);
            line_count++;
            
            // Paginación
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
    printf("MiqOSoft Kernel v0.18\n");
    printf("Architecture: i686 (32-bit)\n");
    printf("Built with: GCC cross-compiler\n");
    printf("Shell: MiqOSoft Shell v1.0\n");
    printf("Features: System calls, VFS, Memory management\n");
    return 0;
}

int cmd_history(int argc, char* argv[]) {
    // Esta función será implementada por shell.c ya que accede a variables estáticas
    extern int shell_show_history(void);
    return shell_show_history();
}

// ============================================================================
// INFORMACIÓN DEL SISTEMA
// ============================================================================

int cmd_memory(int argc, char* argv[]) {
    printf("Memory Information:\n");
    printf("  Kernel Memory Range: 0x100000 - 0x200000 (1MB)\n");
    printf("  Heap Start: 0x400000 (4MB)\n");
    printf("  Heap Size: 1MB\n");
    printf("  Block Size: 32 bytes\n");
    printf("  Available Memory: Variable (depends on allocation)\n");
    printf("  Memory Management: Simple block allocator\n");
    return 0;
}

int cmd_uptime(int argc, char* argv[]) {
    uint32_t time = sys_time();
    printf("System uptime: %u time units\n", time);
    printf("Note: Real-time clock not implemented\n");
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

int cmd_lsmod(int argc, char* argv[]) {
    printf("Loaded kernel modules:\n");
    printf("  %-15s %-10s %s\n", "Module", "Size", "Description");
    printf("  %-15s %-10s %s\n", "------", "----", "-----------");
    printf("  %-15s %-10s %s\n", "core", "32KB", "Core kernel functionality");
    printf("  %-15s %-10s %s\n", "hal", "8KB", "Hardware Abstraction Layer");
    printf("  %-15s %-10s %s\n", "vga_text", "4KB", "VGA text mode driver");
    printf("  %-15s %-10s %s\n", "keyboard", "6KB", "PS/2 keyboard driver");
    printf("  %-15s %-10s %s\n", "i8259", "2KB", "PIC interrupt controller");
    printf("  %-15s %-10s %s\n", "syscall", "12KB", "System call interface");
    printf("  %-15s %-10s %s\n", "shell", "16KB", "Command shell");
    return 0;
}

int cmd_dmesg(int argc, char* argv[]) {
    printf("Kernel messages (recent):\n");
    printf("[0.000] Kernel started\n");
    printf("[0.001] GDT initialized\n");
    printf("[0.002] IDT initialized\n");
    printf("[0.003] ISR handlers installed\n");
    printf("[0.004] IRQ handlers installed\n");
    printf("[0.005] PIC initialized\n");
    printf("[0.006] Keyboard driver loaded\n");
    printf("[0.007] System calls initialized\n");
    printf("[0.008] Shell initialized\n");
    printf("[0.009] System ready\n");
    return 0;
}

int cmd_ps(int argc, char* argv[]) {
    printf("Process information:\n");
    printf("  %-5s %-5s %-10s %-15s %s\n", "PID", "PPID", "STATE", "NAME", "DESCRIPTION");
    printf("  %-5s %-5s %-10s %-15s %s\n", "---", "----", "-----", "----", "-----------");
    printf("  %-5s %-5s %-10s %-15s %s\n", "0", "-", "running", "kernel", "Kernel process");
    printf("  %-5s %-5s %-10s %-15s %s\n", "1", "0", "running", "shell", "Command shell");
    printf("\nNote: Full process management not implemented\n");
    return 0;
}

int cmd_lspci(int argc, char* argv[]) {
    printf("PCI device enumeration not implemented.\n");
    printf("In a full implementation, this would show:\n");
    printf("  - Host bridge\n");
    printf("  - VGA controller\n");
    printf("  - Network controllers\n");
    printf("  - Storage controllers\n");
    printf("  - USB controllers\n");
    return 0;
}

// ============================================================================
// SISTEMA DE ARCHIVOS VIRTUAL
// ============================================================================

int cmd_ls(int argc, char* argv[]) {
    printf("Directory listing:\n");
    printf("  %-20s %-8s %s\n", "Name", "Type", "Size");
    printf("  %-20s %-8s %s\n", "----", "----", "----");
    
    extern VirtualFile virtual_fs[];
    for (int i = 0; virtual_fs[i].exists; i++) {
        printf("  %-20s %-8s %s\n", 
               virtual_fs[i].name,
               virtual_fs[i].is_directory ? "[DIR]" : "[FILE]",
               virtual_fs[i].is_directory ? "-" : "varies");
    }
    return 0;
}

int cmd_cat(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: cat <filename>\n");
        return 1;
    }
    
    extern VirtualFile virtual_fs[];
    for (int i = 0; virtual_fs[i].exists; i++) {
        if (!virtual_fs[i].is_directory && 
            shell_strcmp(virtual_fs[i].name, argv[1]) == 0) {
            printf("%s\n", virtual_fs[i].content);
            return 0;
        }
    }
    
    printf("cat: %s: No such file\n", argv[1]);
    return 1;
}

int cmd_mkdir(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: mkdir <directory>\n");
        return 1;
    }
    
    printf("mkdir: Directory creation simulated\n");
    printf("Would create directory: %s\n", argv[1]);
    printf("Note: Full filesystem not implemented\n");
    return 0;
}

int cmd_rm(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: rm <file>\n");
        return 1;
    }
    
    printf("rm: File deletion simulated\n");
    printf("Would remove: %s\n", argv[1]);
    printf("Note: Full filesystem not implemented\n");
    return 0;
}

int cmd_find(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: find <pattern>\n");
        return 1;
    }
    
    printf("Searching for files matching '%s':\n", argv[1]);
    
    extern VirtualFile virtual_fs[];
    bool found = false;
    
    for (int i = 0; virtual_fs[i].exists; i++) {
        if (shell_strstr(virtual_fs[i].name, argv[1]) != 0) {
            printf("  %s %s\n", 
                   virtual_fs[i].is_directory ? "[DIR] " : "[FILE]",
                   virtual_fs[i].name);
            found = true;
        }
    }
    
    if (!found) {
        printf("  No files found matching '%s'\n", argv[1]);
    }
    
    return 0;
}

int cmd_grep(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Usage: grep <pattern> <file>\n");
        return 1;
    }
    
    extern VirtualFile virtual_fs[];
    
    for (int i = 0; virtual_fs[i].exists; i++) {
        if (!virtual_fs[i].is_directory && 
            shell_strcmp(virtual_fs[i].name, argv[2]) == 0) {
            
            char* content = virtual_fs[i].content;
            char* line_start = content;
            int line_number = 1;
            bool found = false;
            
            while (*line_start) {
                char* line_end = shell_strchr(line_start, '\n');
                int line_length = line_end ? (line_end - line_start) : shell_strlen(line_start);
                
                char line_buffer[256];
                shell_strncpy(line_buffer, line_start, line_length);
                line_buffer[line_length] = '\0';
                
                if (shell_strstr(line_buffer, argv[1]) != 0) {
                    if (!found) {
                        printf("Matches in '%s':\n", argv[2]);
                        found = true;
                    }
                    printf("  %d: %s\n", line_number, line_buffer);
                }
                
                line_number++;
                line_start = line_end ? line_end + 1 : line_start + line_length;
                if (!line_end) break;
            }
            
            if (!found) {
                printf("No matches found for '%s' in '%s'\n", argv[1], argv[2]);
            }
            return 0;
        }
    }
    
    printf("grep: %s: No such file\n", argv[2]);
    return 1;
}

int cmd_wc(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: wc <file>\n");
        return 1;
    }
    
    extern VirtualFile virtual_fs[];
    
    for (int i = 0; virtual_fs[i].exists; i++) {
        if (!virtual_fs[i].is_directory && 
            shell_strcmp(virtual_fs[i].name, argv[1]) == 0) {
            
            char* content = virtual_fs[i].content;
            int lines = 0, words = 0, chars = 0;
            bool in_word = false;
            
            while (*content) {
                chars++;
                
                if (*content == '\n') {
                    lines++;
                }
                
                if (*content == ' ' || *content == '\t' || *content == '\n') {
                    if (in_word) {
                        words++;
                        in_word = false;
                    }
                } else {
                    in_word = true;
                }
                
                content++;
            }
            
            if (in_word) words++;
            
            printf("  %4d %4d %4d %s\n", lines, words, chars, argv[1]);
            return 0;
        }
    }
    
    printf("wc: %s: No such file\n", argv[1]);
    return 1;
}

// ============================================================================
// COMANDOS DE HARDWARE Y DEBUGGING
// ============================================================================

int cmd_memtest(int argc, char* argv[]) {
    printf("Basic memory test...\n");
    
    uint8_t* test_ptr = (uint8_t*)0x200000; // 2MB
    uint8_t patterns[] = {0x00, 0xFF, 0xAA, 0x55, 0xCC, 0x33};
    int pattern_count = sizeof(patterns);
    int test_size = 1024;
    bool all_passed = true;
    
    for (int p = 0; p < pattern_count; p++) {
        printf("  Testing pattern 0x%02X... ", patterns[p]);
        
        // Escribir patrón
        for (int i = 0; i < test_size; i++) {
            test_ptr[i] = patterns[p];
        }
        
        // Verificar patrón
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

int cmd_ports(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: ports <read|write> [port] [value]\n");
        printf("  ports read 0x3f8      - Read from port 0x3f8\n");
        printf("  ports write 0x3f8 65  - Write 65 to port 0x3f8\n");
        return 1;
    }
    
    if (shell_strcmp(argv[1], "read") == 0) {
        if (argc < 3) {
            printf("Usage: ports read <port>\n");
            return 1;
        }
        
        uint16_t port = hex_str_to_int(argv[2]);
        uint8_t value = i686_inb(port);
        printf("Port 0x%04X = 0x%02X (%d)\n", port, value, value);
        
    } else if (shell_strcmp(argv[1], "write") == 0) {
        if (argc < 4) {
            printf("Usage: ports write <port> <value>\n");
            return 1;
        }
        
        uint16_t port = hex_str_to_int(argv[2]);
        uint8_t value = dec_str_to_int(argv[3]);
        
        i686_outb(port, value);
        printf("Wrote 0x%02X (%d) to port 0x%04X\n", value, value, port);
        
    } else {
        printf("Invalid operation: %s\n", argv[1]);
        return 1;
    }
    
    return 0;
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
    if (argc < 3) {
        printf("Usage: hexdump <address> <length>\n");
        printf("  hexdump 0x100000 256  - Dump 256 bytes from 0x100000\n");
        return 1;
    }
    
    uint32_t addr = hex_str_to_int(argv[1]);
    uint32_t len = dec_str_to_int(argv[2]);
    
    if (len > 1024) {
        printf("Length limited to 1024 bytes\n");
        len = 1024;
    }
    
    uint8_t* ptr = (uint8_t*)addr;
    
    printf("Hex dump of 0x%08X (%d bytes):\n", addr, len);
    printf("Address   00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F  ASCII\n");
    printf("--------  -----------------------------------------------  -----\n");
    
    for (uint32_t i = 0; i < len; i += 16) {
        printf("%08X: ", addr + i);
        
        // Mostrar hex
        for (int j = 0; j < 16; j++) {
            if (i + j < len) {
                printf("%02X ", ptr[i + j]);
            } else {
                printf("   ");
            }
        }
        
        printf(" |");
        
        // Mostrar ASCII
        for (int j = 0; j < 16 && i + j < len; j++) {
            char c = ptr[i + j];
            printf("%c", (c >= 32 && c <= 126) ? c : '.');
        }
        
        printf("|\n");
    }
    
    return 0;
}

int cmd_keytest(int argc, char* argv[]) {
    printf("Keyboard test mode - press keys to see scancodes\n");
    printf("Press ESC to exit\n\n");
    
    while (1) {
        keyboard_process_buffer();
        
        if (i686_inb(0x64) & 0x01) { // Datos disponibles
            uint8_t scancode = i686_inb(0x60);
            
            if (scancode == 0x01) { // ESC
                printf("\nExiting keyboard test\n");
                break;
            }
            
            printf("Scancode: 0x%02X", scancode);
            if (scancode & 0x80) {
                printf(" (key released)");
            } else {
                printf(" (key pressed)");
            }
            
            // Mostrar tecla si es conocida
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
    
    // Test aritmético
    printf("1. Arithmetic operations test: ");
    volatile uint32_t result = 0;
    for (uint32_t i = 0; i < 1000000; i++) {
        result += i * 2 + 1;
    }
    printf("DONE (result: %u)\n", result);
    
    // Test de memoria
    printf("2. Memory access test: ");
    volatile uint8_t* mem = (volatile uint8_t*)0x200000;
    for (uint32_t i = 0; i < 100000; i++) {
        mem[i % 1024] = (uint8_t)(i & 0xFF);
        result += mem[i % 1024];
    }
    printf("DONE\n");
    
    // Test de llamadas a función
    printf("3. Function call test: ");
    for (uint32_t i = 0; i < 50000; i++) {
        VGA_get_cursor_x();
    }
    printf("DONE\n");
    
    // Test de operaciones lógicas
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
    printf("    EAX: 0x%08X    EBX: 0x%08X\n", eax, ebx);
    printf("    ECX: 0x%08X    EDX: 0x%08X\n", ecx, edx);
    printf("  Index Registers:\n");
    printf("    ESI: 0x%08X    EDI: 0x%08X\n", esi, edi);
    printf("  Stack Pointers:\n");
    printf("    ESP: 0x%08X    EBP: 0x%08X\n", esp, ebp);
    printf("  Flags:\n");
    printf("    EFLAGS: 0x%08X\n", eflags);
    
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

int cmd_stack(int argc, char* argv[]) {
    uint32_t* stack_ptr;
    __asm__ volatile("movl %%esp, %0" : "=r"(stack_ptr));
    
    int entries = 16;
    if (argc > 1) {
        entries = dec_str_to_int(argv[1]);
        if (entries > 32) entries = 32;
        if (entries < 1) entries = 16;
    }
    
    printf("Stack dump (ESP: 0x%08X, showing %d entries):\n", (uint32_t)stack_ptr, entries);
    printf("Address   Value      ASCII\n");
    printf("--------  --------   -----\n");
    
    for (int i = 0; i < entries; i++) {
        printf("%08X: %08X   ", (uint32_t)(stack_ptr + i), stack_ptr[i]);
        
        // Mostrar como ASCII si es válido
        uint8_t* bytes = (uint8_t*)&stack_ptr[i];
        for (int j = 0; j < 4; j++) {
            char c = bytes[j];
            printf("%c", (c >= 32 && c <= 126) ? c : '.');
        }
        printf("\n");
    }
    
    return 0;
}

// ============================================================================
// COMANDOS DE RED (STUBS)
// ============================================================================

int cmd_ping(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: ping <host>\n");
        return 1;
    }
    
    printf("PING %s: Network stack not implemented\n", argv[1]);
    printf("This would send ICMP echo requests when networking is available\n");
    printf("Simulating ping...\n");
    printf("PING %s: 64 bytes from %s: icmp_seq=1 ttl=64 time=0.1 ms (simulated)\n", argv[1], argv[1]);
    printf("PING %s: 64 bytes from %s: icmp_seq=2 ttl=64 time=0.2 ms (simulated)\n", argv[1], argv[1]);
    printf("PING %s: 64 bytes from %s: icmp_seq=3 ttl=64 time=0.1 ms (simulated)\n", argv[1], argv[1]);
    return 0;
}

int cmd_netstat(int argc, char* argv[]) {
    printf("Network Statistics:\n");
    printf("  Network Stack: Not implemented\n");
    printf("  Active Connections: None\n");
    printf("  Interfaces:\n");
    printf("    lo0: 127.0.0.1 (loopback) - simulated\n");
    printf("  Routing Table: Empty\n");
    printf("  Network Buffers: N/A\n");
    return 0;
}

// ============================================================================
// COMANDOS DE SYSTEM CALLS
// ============================================================================

int cmd_syscall_test(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: syscall_test <test_type>\n");
        printf("Available tests:\n");
        printf("  all       - Run comprehensive test suite\n");
        printf("  basic     - Test basic syscalls (print, getpid, time)\n");
        printf("  memory    - Test malloc/free operations\n");
        printf("  files     - Test file I/O operations\n");
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
        printf("  Allocated 256 bytes at: 0x%08X\n", (uint32_t)ptr1);
        printf("  Allocated 512 bytes at: 0x%08X\n", (uint32_t)ptr2);
        
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
        
    } else if (shell_strcmp(argv[1], "files") == 0) {
        printf("=== File Syscall Test ===\n\n");
        
        printf("Opening existing file 'welcome.txt':\n");
        int32_t fd = sys_open("welcome.txt", OPEN_READ);
        printf("  File descriptor: %d\n", fd);
        
        if (fd >= 0) {
            char buffer[200];
            memset(buffer, 0, sizeof(buffer));
            int32_t bytes = sys_read(fd, buffer, sizeof(buffer) - 1);
            printf("  Read %d bytes:\n%s\n", bytes, buffer);
            sys_close(fd);
        }
        
        printf("Creating new file 'shell_test.txt':\n");
        fd = sys_open("shell_test.txt", OPEN_CREATE | OPEN_WRITE);
        printf("  File descriptor: %d\n", fd);
        
        if (fd >= 0) {
            const char* data = "This file was created from the shell!\nUsing syscalls for file operations.\n";
            int32_t written = sys_write(fd, data, strlen(data));
            printf("  Wrote %d bytes\n", written);
            sys_close(fd);
            
            fd = sys_open("shell_test.txt", OPEN_READ);
            if (fd >= 0) {
                char buffer[200];
                memset(buffer, 0, sizeof(buffer));
                int32_t bytes = sys_read(fd, buffer, sizeof(buffer) - 1);
                printf("  Reading back file (%d bytes):\n%s\n", bytes, buffer);
                sys_close(fd);
            }
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
        
        printf("File stress test (creating 10 files):\n");
        char filename[32];
        success = 0;
        failed = 0;
        for (int i = 0; i < 10; i++) {
            snprintf(filename, sizeof(filename), "stress_%d.txt", i);
            int32_t fd = sys_open(filename, OPEN_CREATE | OPEN_WRITE);
            if (fd >= 0) {
                success++;
                char data[64];
                snprintf(data, sizeof(data), "Stress test file #%d\n", i);
                sys_write(fd, data, strlen(data));
                sys_close(fd);
            } else {
                failed++;
            }
        }
        printf("  Success: %d, Failed: %d\n", success, failed);
        
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
        printf("Success! Allocated at address: 0x%08X\n", (uint32_t)ptr);
        
        // Escribir datos de prueba
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

int cmd_file_create(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: file_create <filename> [content]\n");
        printf("Example: file_create myfile.txt \"Hello World\"\n");
        return 1;
    }
    
    const char* filename = argv[1];
    const char* content = argc > 2 ? argv[2] : "Default file content\n";
    
    printf("Creating file: %s\n", filename);
    
    int32_t fd = sys_open(filename, OPEN_CREATE | OPEN_WRITE);
    if (fd < 0) {
        printf("Failed to create file: %s (error: %d)\n", filename, fd);
        return 1;
    }
    
    printf("File descriptor: %d\n", fd);
    
    int32_t bytes_written = sys_write(fd, content, strlen(content));
    printf("Wrote %d bytes\n", bytes_written);
    
    sys_close(fd);
    printf("File closed successfully\n");
    
    // Verificar leyendo el archivo
    printf("\nVerifying by reading back:\n");
    fd = sys_open(filename, OPEN_READ);
    if (fd >= 0) {
        char buffer[256];
        memset(buffer, 0, sizeof(buffer));
        int32_t bytes_read = sys_read(fd, buffer, sizeof(buffer) - 1);
        printf("Read %d bytes: %s\n", bytes_read, buffer);
        sys_close(fd);
    }
    
    return 0;
}

int cmd_file_read(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: file_read <filename>\n");
        printf("Example: file_read welcome.txt\n");
        return 1;
    }
    
    const char* filename = argv[1];
    
    printf("Reading file: %s\n", filename);
    
    int32_t fd = sys_open(filename, OPEN_READ);
    if (fd < 0) {
        printf("Failed to open file: %s (error: %d)\n", filename, fd);
        return 1;
    }
    
    printf("File descriptor: %d\n", fd);
    
    char buffer[512];
    memset(buffer, 0, sizeof(buffer));
    int32_t bytes_read = sys_read(fd, buffer, sizeof(buffer) - 1);
    
    printf("Read %d bytes:\n", bytes_read);
    printf("--- File Content ---\n");
    printf("%s", buffer);
    if (bytes_read == sizeof(buffer) - 1) {
        printf("... (file may be larger)\n");
    }
    printf("--- End Content ---\n");
    
    sys_close(fd);
    return 0;
}

int cmd_heap_info(int argc, char* argv[]) {
    printf("=== Heap Information ===\n\n");
    
    printf("Heap configuration:\n");
    printf("  Start address: 0x%08X\n", 0x400000);
    printf("  Size: %d KB (%d bytes)\n", 0x100000 / 1024, 0x100000);
    printf("  Block size: %d bytes\n", 32);
    printf("  Management: Simple block allocator\n");
    
    printf("\nTesting current heap state:\n");
    
    void* test_ptrs[5];
    int sizes[] = {64, 128, 256, 512, 1024};
    
    for (int i = 0; i < 5; i++) {
        test_ptrs[i] = sys_malloc(sizes[i]);
        if (test_ptrs[i]) {
            printf("  Allocated %d bytes at 0x%08X\n", sizes[i], (uint32_t)test_ptrs[i]);
        } else {
            printf("  Failed to allocate %d bytes\n", sizes[i]);
        }
    }
    
    printf("\nCleaning up test allocations:\n");
    for (int i = 0; i < 5; i++) {
        if (test_ptrs[i]) {
            sys_free(test_ptrs[i]);
            printf("  Freed block at 0x%08X\n", (uint32_t)test_ptrs[i]);
        }
    }
    
    return 0;
}

int cmd_syscall_info(int argc, char* argv[]) {
    printf("=== System Call Information ===\n\n");
    
    printf("Syscall Interface:\n");
    printf("  Interrupt vector: 0x80\n");
    printf("  Total syscalls: %d\n", SYSCALL_COUNT);
    printf("  Calling convention: EAX=syscall_num, EBX=arg1, ECX=arg2, EDX=arg3, ESI=arg4\n");
    
    printf("\nImplemented System Calls:\n");
    printf("  %-2s %-15s %s\n", "ID", "Name", "Description");
    printf("  %-2s %-15s %s\n", "--", "----", "-----------");
    printf("  %-2d %-15s %s\n", 0, "exit", "Exit process with code");
    printf("  %-2d %-15s %s\n", 1, "print", "Print message to console");
    printf("  %-2d %-15s %s\n", 2, "read", "Read from file descriptor");
    printf("  %-2d %-15s %s\n", 3, "malloc", "Allocate memory");
    printf("  %-2d %-15s %s\n", 4, "free", "Free allocated memory");
    printf("  %-2d %-15s %s\n", 5, "open", "Open file");
    printf("  %-2d %-15s %s\n", 6, "close", "Close file descriptor");
    printf("  %-2d %-15s %s\n", 7, "write", "Write to file descriptor");
    printf("  %-2d %-15s %s\n", 8, "seek", "Seek in file (placeholder)");
    printf("  %-2d %-15s %s\n", 9, "getpid", "Get process ID");
    printf("  %-2d %-15s %s\n", 10, "time", "Get current time");
    printf("  %-2d %-15s %s\n", 11, "sleep", "Sleep for milliseconds");
    printf("  %-2d %-15s %s\n", 12, "yield", "Yield to scheduler");
    
    printf("\nError Codes:\n");
    printf("  %2d - SYSCALL_OK\n", 0);
    printf("  %2d - SYSCALL_ERROR\n", -1);
    printf("  %2d - SYSCALL_INVALID_SYSCALL\n", -2);
    printf("  %2d - SYSCALL_INVALID_PARAMS\n", -3);
    printf("  %2d - SYSCALL_PERMISSION_DENIED\n", -4);
    printf("  %2d - SYSCALL_NOT_FOUND\n", -5);
    printf("  %2d - SYSCALL_OUT_OF_MEMORY\n", -6);
    
    printf("\nUsage examples:\n");
    printf("  syscall_test basic     - Test basic functionality\n");
    printf("  malloc_test 1024       - Test memory allocation\n");
    printf("  file_create test.txt   - Create a test file\n");
    printf("  file_read test.txt     - Read a file\n");
    
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

// ============================================================================
// COMANDOS DE CONTROL DEL SISTEMA
// ============================================================================

int cmd_reboot(int argc, char* argv[]) {
    printf("Rebooting system...\n");
    printf("Goodbye!\n");
    
    // Usar el puerto del teclado para reboot
    i686_outb(0x64, 0xFE);
    
    // Si el reboot falló, usar método alternativo
    printf("Primary reboot method failed, trying alternative...\n");
    
    // Método de triple fault
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

// ============================================================================
// TABLA DE COMANDOS UNIFICADA
// ============================================================================

const ShellCommandEntry unified_shell_commands[] = {
    // Comandos básicos
    {"help",            "Show available commands",                          cmd_help},
    {"clear",           "Clear the screen",                                 cmd_clear},
    {"echo",            "Display a line of text",                           cmd_echo},
    {"version",         "Show OS version information",                      cmd_version},
    {"history",         "Show command history",                             cmd_history},
    
    // Información del sistema
    {"memory",          "Show memory information",                          cmd_memory},
    {"uptime",          "Show system uptime",                               cmd_uptime},
    {"cpuinfo",         "Show CPU information",                             cmd_cpuinfo},
    {"cpuid",           "Show detailed CPU information via CPUID",          cmd_cpuid},
    {"lsmod",           "List loaded kernel modules",                       cmd_lsmod},
    {"dmesg",           "Show kernel messages",                             cmd_dmesg},
    {"ps",              "Show running processes",                           cmd_ps},
    {"lspci",           "List PCI devices",                                 cmd_lspci},
    
    // Sistema de archivos
    {"ls",              "List directory contents",                          cmd_ls},
    {"cat",             "Display file contents",                            cmd_cat},
    {"mkdir",           "Create directory",                                 cmd_mkdir},
    {"rm",              "Remove file or directory",                         cmd_rm},
    {"find",            "Find files by name pattern",                       cmd_find},
    {"grep",            "Search text in files",                             cmd_grep},
    {"wc",              "Count lines, words and characters",                cmd_wc},
    
    // Hardware y debugging
    {"memtest",         "Run basic memory test",                            cmd_memtest},
    {"ports",           "Read/write I/O ports",                             cmd_ports},
    {"interrupt",       "Control interrupt state",                          cmd_interrupt},
    {"hexdump",         "Display memory in hexadecimal",                    cmd_hexdump},
    {"keytest",         "Test keyboard input (shows scancodes)",            cmd_keytest},
    {"benchmark",       "Run CPU benchmark",                                cmd_benchmark},
    {"registers",       "Show CPU register values",                         cmd_registers},
    {"stack",           "Show stack contents",                              cmd_stack},
    
    // Red (stubs)
    {"ping",            "Send ICMP echo requests",                          cmd_ping},
    {"netstat",         "Show network statistics",                          cmd_netstat},
    
    // System calls
    {"syscall_test",    "Test system call functionality",                   cmd_syscall_test},
    {"malloc_test",     "Test memory allocation via syscall",               cmd_malloc_test},
    {"file_create",     "Create file via syscall",                          cmd_file_create},
    {"file_read",       "Read file via syscall",                            cmd_file_read},
    {"heap_info",       "Show heap information and test",                   cmd_heap_info},
    {"syscall_info",    "Show syscall information and usage",               cmd_syscall_info},
    {"sleep_test",      "Test sleep syscall",                               cmd_sleep_test},
    
    // Control del sistema
    {"reboot",          "Restart the system",                               cmd_reboot},
    {"panic",           "Trigger a kernel panic (for testing)",             cmd_panic},
    
    // Terminador
    {NULL, NULL, NULL}
};

// ============================================================================
// FUNCIONES AUXILIARES PARA MANEJO DE COMANDOS
// ============================================================================

int get_unified_command_count(void) {
    int count = 0;
    while (unified_shell_commands[count].name != NULL) {
        count++;
    }
    return count;
}

const ShellCommandEntry* find_unified_command(const char* name) {
    if (!name) return NULL;
    
    for (int i = 0; unified_shell_commands[i].name != NULL; i++) {
        if (shell_strcmp(unified_shell_commands[i].name, name) == 0) {
            return &unified_shell_commands[i];
        }
    }
    
    return NULL;
}

/*
TODO:
- Arreglar cmd_history
- Hacer que cmd_memory muestre memoria real y libre correctamente
- cmd uptime no muestra nada
- cmd_dmesg sea real
- cmd_ps no muestra nada
- cmd_ls no funciona bien y sea real
- cmd_cat sea real
- cmd_mkdir sea real
- cmd_rm sea real
- cmd_find sea real
- cmd_grep sea real
- cmd_wc sea real
- cmd_ports da error 1
- cmd_stack no funciona bien
- cmd_file_create no funciona bien
- cmd_syscall_info se corta
- cmd_syscall_sleep no funciona bien
*/