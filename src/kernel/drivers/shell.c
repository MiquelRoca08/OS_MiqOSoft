#include "shell.h"
#include <shell_commands.h>
#include <stdio.h>
#include <memory.h>
#include <vga_text.h>
#include <keyboard.h>
#include <io.h>
#include <debug.h>
#include <hal.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>

#define HISTORY_SIZE 50
#define COMMAND_MAX_LEN 256

// Definición de size_t si no está disponible
#ifndef SIZE_T_DEFINED
#define SIZE_T_DEFINED
typedef unsigned long size_t;
#endif

// Buffer para la línea de comandos actual
static char command_buffer[SHELL_BUFFER_SIZE];
static int command_pos = 0;
static int cursor_position = 0;  // Posición del cursor en la línea de comandos
static bool shell_running = false;

static char command_history[HISTORY_SIZE][COMMAND_MAX_LEN];
static int history_count = 0;
static int history_index = -1;  // -1 significa comando actual (no en historial)
static char current_command_backup[COMMAND_MAX_LEN];  // Backup del comando actual

void shell_redraw_screen(void) {
    extern int g_ScreenX, g_ScreenY;
    VGA_clrscr();
    shell_print_prompt();
    printf("%s", command_buffer);
    g_ScreenX = shell_strlen(SHELL_PROMPT) + cursor_position;
    g_ScreenY = SCREEN_HEIGHT - 1;
    VGA_setcursor(g_ScreenX, g_ScreenY);
}

void history_add_command(const char* command) {
    if (!command || shell_strlen(command) == 0) return;
    
    // No agregar comandos duplicados consecutivos
    if (history_count > 0 && 
        shell_strcmp(command_history[(history_count - 1) % HISTORY_SIZE], command) == 0) {
        return;
    }
    
    // Copiar comando al historial
    shell_strncpy(command_history[history_count % HISTORY_SIZE], command, COMMAND_MAX_LEN - 1);
    command_history[history_count % HISTORY_SIZE][COMMAND_MAX_LEN - 1] = '\0';
    
    history_count++;
    history_index = -1;  // Reset índice de navegación
}

const char* history_get_previous(void) {
    if (history_count == 0) return NULL;
    
    if (history_index == -1) {
        // Primera vez navegando hacia atrás - guardar comando actual
        shell_strncpy(current_command_backup, command_buffer, COMMAND_MAX_LEN - 1);
        history_index = (history_count - 1) % HISTORY_SIZE;
    } else {
        // Navegar hacia atrás en el historial
        if (history_count < HISTORY_SIZE) {
            if (history_index > 0) history_index--;
        } else {
            history_index = (history_index - 1 + HISTORY_SIZE) % HISTORY_SIZE;
            if (history_index == (history_count % HISTORY_SIZE)) {
                // Hemos llegado al comando más antiguo
                history_index = (history_index + 1) % HISTORY_SIZE;
                return command_history[history_index];
            }
        }
    }
    
    return command_history[history_index];
}

const char* history_get_next(void) {
    if (history_count == 0 || history_index == -1) return NULL;
    
    if (history_count < HISTORY_SIZE) {
        if (history_index < history_count - 1) {
            history_index++;
            return command_history[history_index];
        } else {
            // Volver al comando actual
            history_index = -1;
            return current_command_backup;
        }
    } else {
        history_index = (history_index + 1) % HISTORY_SIZE;
        if (history_index == (history_count % HISTORY_SIZE)) {
            // Volver al comando actual
            history_index = -1;
            return current_command_backup;
        }
        return command_history[history_index];
    }
}
void shell_clear_line(void) {
    // Borrar visualmente la línea actual
    extern int g_ScreenX, g_ScreenY;
    int prompt_len = shell_strlen(SHELL_PROMPT);  // YA ESTÁ CORRECTO
    
    // Ir al inicio de la línea (después del prompt)
    g_ScreenX = prompt_len;
    VGA_setcursor(g_ScreenX, g_ScreenY);
    
    // Borrar hasta el final de la línea
    for (int i = 0; i < command_pos; i++) {
        printf(" ");
    }
    
    // Volver al inicio
    g_ScreenX = prompt_len;
    VGA_setcursor(g_ScreenX, g_ScreenY);
}

void shell_display_command(const char* cmd) {
    // Limpiar línea actual
    shell_clear_line();
    
    // Copiar nuevo comando
    shell_strncpy(command_buffer, cmd, SHELL_BUFFER_SIZE - 1);
    command_buffer[SHELL_BUFFER_SIZE - 1] = '\0';
    command_pos = shell_strlen(command_buffer);  // CAMBIADO: strlen -> shell_strlen
    
    // Mostrar el comando
    printf("%s", command_buffer);
}

void shell_move_cursor_left(void) {
    if (cursor_position > 0) {
        cursor_position--;
        extern int g_ScreenX, g_ScreenY;
        if (g_ScreenX > shell_strlen(SHELL_PROMPT)) {
            g_ScreenX--;
            VGA_setcursor(g_ScreenX, g_ScreenY);
        }
    }
}

void shell_move_cursor_right(void) {
    if (cursor_position < command_pos) {
        cursor_position++;
        extern int g_ScreenX, g_ScreenY;
        g_ScreenX++;
        VGA_setcursor(g_ScreenX, g_ScreenY);
    }
}

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

// Función auxiliar simple para formatear números
void format_number(char* buffer, int num) {
    if (num == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }
    
    char temp[12];
    int temp_len = 0;
    bool negative = num < 0;
    if (negative) num = -num;
    
    while (num > 0) {
        temp[temp_len++] = '0' + (num % 10);
        num /= 10;
    }
    
    int pos = 0;
    if (negative) buffer[pos++] = '-';
    
    for (int i = temp_len - 1; i >= 0; i--) {
        buffer[pos++] = temp[i];
    }
    buffer[pos] = '\0';
}

// Tabla de comandos disponibles
static ShellCommand shell_commands[] = {
    {"help",      "Show available commands",                   cmd_help},
    {"clear",     "Clear the screen",                          cmd_clear},
    {"echo",      "Display a line of text",                    cmd_echo},
    {"version",   "Show OS version information",               cmd_version},
    {"reboot",    "Restart the system",                        cmd_reboot},
    {"panic",     "Trigger a kernel panic (for testing)",      cmd_panic},
    {"memory",    "Show memory information",                   cmd_memory},
    {"uptime",    "Show system uptime",                        cmd_uptime},
    {"history",   "Show command history",                      cmd_history},
    
    // Sistema de archivos
    {"ls",        "List directory contents",                   cmd_ls},
    {"cat",       "Display file contents",                     cmd_cat},
    {"mkdir",     "Create directory",                          cmd_mkdir},
    {"rm",        "Remove file or directory",                  cmd_rm},
    {"find",      "Find files by name pattern",                cmd_find},
    {"grep",      "Search text in files",                      cmd_grep},
    {"wc",        "Count lines, words and characters",         cmd_wc},
    
    // Información del sistema
    {"lsmod",     "List loaded kernel modules",                cmd_lsmod},
    {"dmesg",     "Show kernel messages",                      cmd_dmesg},
    {"ps",        "Show running processes",                    cmd_ps},
    {"lspci",     "List PCI devices",                          cmd_lspci},
    {"cpuinfo",   "Show CPU information",                      cmd_cpuinfo},
    {"cpuid",     "Show detailed CPU information via CPUID",   cmd_cpuid},
    
    // Hardware y debugging
    {"memtest",   "Run basic memory test",                     cmd_memtest},
    {"ports",     "Read/write I/O ports",                      cmd_ports},
    {"interrupt", "Control interrupt state",                   cmd_interrupt},
    {"hexdump",   "Display memory in hexadecimal",             cmd_hexdump},
    {"keytest",   "Test keyboard input (shows scancodes)",     cmd_keytest},
    {"benchmark", "Run CPU benchmark",                         cmd_benchmark},
    {"registers", "Show CPU register values",                  cmd_registers},
    {"stack",     "Show stack contents",                       cmd_stack},
    {"ping",      "Send ICMP echo requests",                   cmd_ping},
    {"netstat",   "Show network statistics",                   cmd_netstat},
    {"exit",      "Exit the shell",                            NULL},  // Comando adicional para mejor organización
    
    {NULL, NULL, NULL}  // Terminador
};

void shell_init(void) {
    memset(command_buffer, 0, sizeof(command_buffer));
    command_pos = 0;
    cursor_position = 0;
    shell_running = true;
    
    // Inicializar historial
    memset(command_history, 0, sizeof(command_history));
    history_count = 0;
    history_index = -1;
    
    // IMPORTANTE: Activar el modo shell en el sistema de teclado
    keyboard_set_shell_mode(true);
    
    printf("MiqOSoft Shell v1.0\n");
    printf("Type 'help' for a list of available commands.\n");
    
    printf("\n");
    
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
                // Agregar al historial antes de procesar
                history_add_command(command_buffer);
                shell_process_command(command_buffer);
            }
            
            // Reset buffer y cursor
            memset(command_buffer, 0, sizeof(command_buffer));
            command_pos = 0;
            cursor_position = 0;
            shell_print_prompt();
            
        } else if (c == '\b') {
            // Backspace
            if (cursor_position > 0 && command_pos > 0) {
                // Si el cursor no está al final, mover caracteres
                if (cursor_position < command_pos) {
                    for (int i = cursor_position - 1; i < command_pos - 1; i++) {
                        command_buffer[i] = command_buffer[i + 1];
                    }
                }
                
                command_pos--;
                cursor_position--;
                command_buffer[command_pos] = '\0';
                
                // Redibujar línea
                extern int g_ScreenX, g_ScreenY;
                int prompt_len = shell_strlen(SHELL_PROMPT);  // YA ESTÁ CORRECTO
                g_ScreenX = prompt_len;
                VGA_setcursor(g_ScreenX, g_ScreenY);
                
                // Limpiar línea y redibujar
                for (int i = 0; i <= command_pos + 1; i++) printf(" ");
                g_ScreenX = prompt_len;
                VGA_setcursor(g_ScreenX, g_ScreenY);
                printf("%s", command_buffer);
                
                // Posicionar cursor
                g_ScreenX = prompt_len + cursor_position;
                VGA_setcursor(g_ScreenX, g_ScreenY);
            }
            
        } else if (c == 0x01) {
            // UP - historial anterior
            const char* prev_cmd = history_get_previous();
            if (prev_cmd) {
                shell_display_command(prev_cmd);
                cursor_position = command_pos;
            }
            
        } else if (c == 0x02) {
            // DOWN - historial siguiente
            const char* next_cmd = history_get_next();
            if (next_cmd) {
                shell_display_command(next_cmd);
                cursor_position = command_pos;
            }
            
        } else if (c == 0x03) {
            // LEFT - mover cursor izquierda
            shell_move_cursor_left();
            
        } else if (c == 0x04) {
            // RIGHT - mover cursor derecha
            shell_move_cursor_right();
            
        } else if (c >= 32 && c <= 126) {
            // Carácter imprimible
            if (command_pos < SHELL_BUFFER_SIZE - 1) {
                // Si cursor no está al final, insertar carácter
                if (cursor_position < command_pos) {
                    for (int i = command_pos; i > cursor_position; i--) {
                        command_buffer[i] = command_buffer[i - 1];
                    }
                }
                
                command_buffer[cursor_position] = c;
                command_pos++;
                cursor_position++;
                
                // Redibujar desde cursor hasta final
                extern int g_ScreenX, g_ScreenY;
                int start_x = g_ScreenX;
                printf("%s", &command_buffer[cursor_position - 1]);
                g_ScreenX = start_x + 1;
                VGA_setcursor(g_ScreenX, g_ScreenY);
            }
        } else if (c != 0) {
            // Caracteres especiales (acentos, ñ, etc.)
            if (command_pos < SHELL_BUFFER_SIZE - 1) {
                if (cursor_position < command_pos) {
                    for (int i = command_pos; i > cursor_position; i--) {
                        command_buffer[i] = command_buffer[i - 1];
                    }
                }
                
                command_buffer[cursor_position] = c;
                command_pos++;
                cursor_position++;
                printf("%c", c);
            }
        }
    }
}

int shell_parse_command(const char* input, char* argv[]) {
    static char local_buffer[SHELL_BUFFER_SIZE];
    int argc = 0;
    int i = 0;
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
    printf("=== MiqOSoft Shell Commands ===\n\n");
    
    // Primero encontrar la longitud del comando más largo y contar total de comandos
    int max_length = 0;
    int total_commands = 0;
    for (int i = 0; shell_commands[i].name != 0; i++) {
        int len = shell_strlen(shell_commands[i].name);
        if (len > max_length) {
            max_length = len;
        }
        total_commands++;
    }
    
    // Añadir margen para la alineación
    max_length += 4;

    // Número de líneas por página (ajustar según el tamaño de la pantalla)
    const int LINES_PER_PAGE = 15;
    int current_page = 0;
    int total_pages = (total_commands + LINES_PER_PAGE - 1) / LINES_PER_PAGE;
    
    // Mostrar cada comando con el espaciado correcto
    for (int i = 0; shell_commands[i].name != 0; i++) {
        // Si es el inicio de una nueva página (excepto la primera)
        if (i > 0 && (i % LINES_PER_PAGE) == 0) {
            printf("\nPress any key to continue...");
            
            // Esperar una tecla
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
            
            // Limpiar línea y continuar
            printf("\r                                           \r");
            VGA_clrscr();  // Limpiar pantalla para la siguiente página
            printf("=== MiqOSoft Shell Commands (Page %d/%d) ===\n\n", 
                   current_page + 2, total_pages);
            current_page++;
        }

        // Imprimir comando y descripción
        printf("%s", shell_commands[i].name);
        
        // Calcular y añadir espacios necesarios
        int spaces = max_length - shell_strlen(shell_commands[i].name);
        for (int s = 0; s < spaces; s++) {
            printf(" ");
        }
        
        printf("%s\n", shell_commands[i].description);
    }
    
    printf("\n");
    return 0;
}

int cmd_history(int argc, char* argv[]) {
    printf("Command history:\n");
    
    if (history_count == 0) {
        printf("  (empty)\n");
        return 0;
    }
    
    int start = (history_count < HISTORY_SIZE) ? 0 : (history_count % HISTORY_SIZE);
    int count = (history_count < HISTORY_SIZE) ? history_count : HISTORY_SIZE;
    
    for (int i = 0; i < count; i++) {
        int idx = (start + i) % HISTORY_SIZE;
        printf("  %3d: %s\n", i + 1, command_history[idx]);
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
    printf("MiqOSoft Kernel v0.17.5\n");
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