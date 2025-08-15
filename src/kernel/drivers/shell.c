#include "shell.h"
#include <stdio.h>
#include <memory.h>
#include <vga_text.h>
#include <keyboard.h>
#include <io.h>
#include <debug.h>
#include <hal.h>

// Buffer para la línea de comandos actual
static char command_buffer[SHELL_BUFFER_SIZE];
static int command_pos = 0;
static bool shell_running = false;

// Implementaciones simples de funciones de string para evitar conflictos
int shell_strlen(const char* str) {
    int len = 0;
    while (str[len]) len++;
    return len;
}

int shell_strcmp(const char* s1, const char* s2) {
    while (*s1 && *s2 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return *s1 - *s2;
}

char* shell_strchr(const char* str, int c) {
    while (*str) {
        if (*str == c) return (char*)str;
        str++;
    }
    return (*str == c) ? (char*)str : 0;
}

char* shell_strstr(const char* haystack, const char* needle) {
    if (!*needle) return (char*)haystack;
    
    while (*haystack) {
        const char* h = haystack;
        const char* n = needle;
        
        while (*h && *n && *h == *n) {
            h++;
            n++;
        }
        
        if (!*n) return (char*)haystack;
        haystack++;
    }
    
    return 0;
}

char* shell_strncpy(char* dest, const char* src, int n) {
    char* d = dest;
    while (n && (*d++ = *src++)) n--;
    while (n--) *d++ = '\0';
    return dest;
}

// Tabla de comandos disponibles
static ShellCommand shell_commands[] = {
    {"help",     "Show available commands",                    cmd_help},
    {"clear",    "Clear the screen",                          cmd_clear},
    {"echo",     "Display a line of text",                    cmd_echo},
    {"version",  "Show OS version information",               cmd_version},
    {"reboot",   "Restart the system",                        cmd_reboot},
    {"panic",    "Trigger a kernel panic (for testing)",      cmd_panic},
    {"memory",   "Show memory information",                   cmd_memory},
    {"uptime",   "Show system uptime",                        cmd_uptime},
    
    // Sistema de archivos
    {"ls",       "List directory contents",                   cmd_ls},
    {"cat",      "Display file contents",                     cmd_cat},
    {"mkdir",    "Create directory",                          cmd_mkdir},
    {"rm",       "Remove file or directory",                  cmd_rm},
    {"find",     "Find files by name pattern",               cmd_find},
    {"grep",     "Search text in files",                      cmd_grep},
    {"wc",       "Count lines, words and characters",         cmd_wc},
    
    // Información del sistema
    {"lsmod",    "List loaded kernel modules",                cmd_lsmod},
    {"dmesg",    "Show kernel messages",                      cmd_dmesg},
    {"ps",       "Show running processes",                    cmd_ps},
    {"lspci",    "List PCI devices",                          cmd_lspci},
    {"cpuinfo",  "Show CPU information",                      cmd_cpuinfo},
    {"cpuid",    "Show detailed CPU information via CPUID",   cmd_cpuid},
    
    // Hardware y debugging
    {"memtest",  "Run basic memory test",                     cmd_memtest},
    {"ports",    "Read/write I/O ports",                      cmd_ports},
    {"interrupt", "Control interrupt state",                  cmd_interrupt},
    {"hexdump",  "Display memory in hexadecimal",             cmd_hexdump},
    {"keytest",  "Test keyboard input (shows scancodes)",     cmd_keytest},
    {"benchmark", "Run CPU benchmark",                        cmd_benchmark},
    {"registers", "Show CPU register values",                cmd_registers},
    {"stack",    "Show stack contents",                       cmd_stack},
    
    // Red (no implementado)
    {"ping",     "Send ICMP echo requests",                   cmd_ping},
    {"netstat",  "Show network statistics",                  cmd_netstat},
    
    {NULL, NULL, NULL} // Terminador
};

void shell_init(void) {
    memset(command_buffer, 0, sizeof(command_buffer));
    command_pos = 0;
    shell_running = true;
    
    printf("MiqOSoft Shell v1.0\n");
    printf("Type 'help' for a list of available commands.\n\n");
    
    // Sincronizar el sistema de teclado con la posición actual del cursor VGA
    extern int cursor_line, cursor_pos, total_lines_used;
    extern int line_lengths[];
    
    cursor_line = VGA_get_cursor_y();
    cursor_pos = VGA_get_cursor_x();
    total_lines_used = cursor_line + 1;
    line_lengths[cursor_line] = 0;
    
    shell_print_prompt();
}

void shell_print_prompt(void) {
    // Sincronizar cursor del teclado con la posición VGA actual
    extern int cursor_line, cursor_pos, total_lines_used;
    extern int line_lengths[];
    
    printf(SHELL_PROMPT);
    
    // Actualizar variables del sistema de teclado
    cursor_line = VGA_get_cursor_y();
    cursor_pos = VGA_get_cursor_x();
    total_lines_used = cursor_line + 1;
    line_lengths[cursor_line] = 0;
}

void shell_run(void) {
    char c;
    
    // Leer caracteres directamente del buffer circular de keyboard.c
    while (keyboard_buffer_pop(&c)) {
        if (c == '\n') {
            // Enter presionado - procesar comando
            printf("\n");
            command_buffer[command_pos] = '\0';
            
            if (command_pos > 0) {
                shell_process_command(command_buffer);
            }
            
            // Reset buffer
            memset(command_buffer, 0, sizeof(command_buffer));
            command_pos = 0;
            shell_print_prompt();
            
        } else if (c == '\b') {
            // Backspace - CORREGIDO
            if (command_pos > 0) {
                command_pos--;
                command_buffer[command_pos] = '\0';
                
                // Mover cursor hacia atrás, escribir espacio, mover cursor hacia atrás otra vez
                // Esto borra visualmente el carácter en pantalla
                extern int g_ScreenX, g_ScreenY;
                if (g_ScreenX > 0) {
                    g_ScreenX--;
                    VGA_putchr(g_ScreenX, g_ScreenY, ' ');  // Escribir espacio
                    VGA_setcursor(g_ScreenX, g_ScreenY);    // Reposicionar cursor
                }
            }
            
        } else if (c >= 32 && c <= 126) {
            // Carácter imprimible
            if (command_pos < SHELL_BUFFER_SIZE - 1) {
                command_buffer[command_pos] = c;
                command_pos++;
                printf("%c", c); // Mostrar el carácter
            }
        }
        // Nota: Otros caracteres especiales (como caracteres acentuados) 
        // también deberían manejarse aquí si es necesario
        else if (c != 0) {
            // Manejar caracteres especiales (acentos, ñ, etc.)
            if (command_pos < SHELL_BUFFER_SIZE - 1) {
                command_buffer[command_pos] = c;
                command_pos++;
                printf("%c", c);
            }
        }
    }
}

int shell_parse_command(const char* input, char* argv[]) {
    static char local_buffer[SHELL_BUFFER_SIZE];
    int argc = 0;
    int i = 0, start = 0;
    bool in_word = false;
    
    // Copiar input a buffer local
    while (input[i] && i < SHELL_BUFFER_SIZE - 1) {
        local_buffer[i] = input[i];
        i++;
    }
    local_buffer[i] = '\0';
    
    // Parsear argumentos
    i = 0;
    while (local_buffer[i] && argc < SHELL_MAX_ARGS - 1) {
        if (local_buffer[i] != ' ' && local_buffer[i] != '\t') {
            if (!in_word) {
                argv[argc] = &local_buffer[i];
                argc++;
                in_word = true;
            }
        } else {
            if (in_word) {
                local_buffer[i] = '\0';
                in_word = false;
            }
        }
        i++;
    }
    
    argv[argc] = NULL;
    return argc;
}

ShellCommand* shell_find_command(const char* name) {
    for (int i = 0; shell_commands[i].name != 0; i++) {
        if (shell_strcmp(shell_commands[i].name, name) == 0) {
            return &shell_commands[i];
        }
    }
    return 0;
}

void shell_process_command(const char* input) {
    char* argv[SHELL_MAX_ARGS];
    int argc = shell_parse_command(input, argv);
    
    if (argc == 0) return;
    
    ShellCommand* cmd = shell_find_command(argv[0]);
    if (cmd) {
        int result = cmd->function(argc, argv);
        if (result != 0) {
            printf("Command '%s' returned error code %d\n", argv[0], result);
        }
    } else {
        printf("Unknown command: %s\n", argv[0]);
        printf("Type 'help' for a list of available commands.\n");
    }
}

// ============================================================================
// COMANDOS BUILT-IN
// ============================================================================

int cmd_help(int argc, char* argv[]) {
    printf("Available commands:\n\n");
    
    for (int i = 0; shell_commands[i].name != 0; i++) {
        printf("  %-12s - %s\n", 
               shell_commands[i].name, 
               shell_commands[i].description);
    }
    printf("\n");
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
    printf("MiqOSoft Kernel v0.16.1\n");
    printf("Architecture: i686 (32-bit)\n");
    printf("Built with: GCC cross-compiler\n");
    printf("Shell: MiqOSoft Shell v1.0\n");
    return 0;
}

int cmd_reboot(int argc, char* argv[]) {
    printf("Rebooting system...\n");
    // Usar el puerto del teclado para reboot
    i686_outb(0x64, 0xFE);
    return 0;
}

int cmd_panic(int argc, char* argv[]) {
    printf("Triggering kernel panic for testing purposes...\n");
    i686_Panic();
    return 0;
}

int cmd_memory(int argc, char* argv[]) {
    printf("Memory Information:\n");
    printf("  Physical Memory: Not implemented yet\n");
    printf("  Available Memory: Not implemented yet\n");
    printf("  Kernel Memory: Not implemented yet\n");
    printf("  User Memory: Not implemented yet\n");
    return 0;
}

int cmd_uptime(int argc, char* argv[]) {
    printf("Uptime: Not implemented yet\n");
    printf("(Timer functionality needs to be added)\n");
    return 0;
}

int cmd_lspci(int argc, char* argv[]) {
    printf("PCI device enumeration not implemented yet.\n");
    printf("This would show connected PCI devices.\n");
    return 0;
}

int cmd_cpuinfo(int argc, char* argv[]) {
    printf("CPU Information:\n");
    printf("  Architecture: i686 (32-bit x86)\n");
    printf("  Vendor: (Detection not implemented)\n");
    printf("  Model: (Detection not implemented)\n");
    printf("  Features: (Detection not implemented)\n");
    return 0;
}

// ============================================================================
// SISTEMA DE ARCHIVOS SIMULADO
// ============================================================================

// Sistema de archivos virtual global
VirtualFile virtual_fs[] = {
    {"README.txt", "Welcome to MiqOSoft!\nThis is a simple operating system written in C.", false, true},
    {"kernel.log", "[BOOT] Kernel initialized\n[BOOT] HAL initialized\n[BOOT] Shell started", false, true},
    {"test.txt", "This is a test file.\nHello, World!", false, true},
    {"config.sys", "# MiqOSoft Configuration\nmemory=640\nvideo=vga\nkeyboard=us", false, true},
    {"autoexec.bat", "@echo off\necho Welcome to MiqOSoft!\necho Type 'help' for commands", false, true},
    {"bin", "", true, true},
    {"dev", "", true, true},
    {"tmp", "", true, true},
    {"etc", "", true, true},
    {"", "", false, false} // Terminador
};

int cmd_ls(int argc, char* argv[]) {
    printf("Directory listing:\n");
    
    for (int i = 0; virtual_fs[i].exists; i++) {
        if (virtual_fs[i].is_directory) {
            printf("  [DIR]  %s\n", virtual_fs[i].name);
        } else {
            printf("  [FILE] %s\n", virtual_fs[i].name);
        }
    }
    return 0;
}

int cmd_cat(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: cat <filename>\n");
        return 1;
    }
    
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
    
    printf("mkdir: Directory creation not fully implemented\n");
    printf("Would create directory: %s\n", argv[1]);
    return 0;
}

int cmd_rm(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: rm <file>\n");
        return 1;
    }
    
    printf("rm: File deletion not fully implemented\n");
    printf("Would remove: %s\n", argv[1]);
    return 0;
}

// ============================================================================
// COMANDOS DEL KERNEL
// ============================================================================

int cmd_lsmod(int argc, char* argv[]) {
    printf("Loaded kernel modules:\n");
    printf("  core         - Core kernel functionality\n");
    printf("  hal          - Hardware Abstraction Layer\n");
    printf("  vga_text     - VGA text mode driver\n");
    printf("  keyboard     - PS/2 keyboard driver\n");
    printf("  i8259        - PIC interrupt controller\n");
    return 0;
}

int cmd_dmesg(int argc, char* argv[]) {
    printf("Kernel messages (last few):\n");
    printf("[0.000] Kernel started\n");
    printf("[0.001] GDT initialized\n");
    printf("[0.002] IDT initialized\n");
    printf("[0.003] ISR handlers installed\n");
    printf("[0.004] IRQ handlers installed\n");
    printf("[0.005] Keyboard driver loaded\n");
    printf("[0.006] Shell initialized\n");
    return 0;
}

int cmd_ps(int argc, char* argv[]) {
    printf("Running processes:\n");
    printf("  PID  PPID  STATE    NAME\n");
    printf("    0     -  running  kernel\n");
    printf("    1     0  running  shell\n");
    printf("\nNote: Process management not fully implemented\n");
    return 0;
}