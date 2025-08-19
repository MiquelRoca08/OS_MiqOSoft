#include "shell.h"
#include <stdio.h>
#include <memory.h>
#include <vga_text.h>
#include <keyboard.h>
#include <mouse.h>
#include <io.h>
#include <debug.h>
#include <hal.h>
#include <string.h>
#include <stdio.h>

#define HISTORY_SIZE 50
#define COMMAND_MAX_LEN 256

// Buffer para la línea de comandos actual
static char command_buffer[SHELL_BUFFER_SIZE];
static int command_pos = 0;
static bool shell_running = false;

static char command_history[HISTORY_SIZE][COMMAND_MAX_LEN];
static int history_count = 0;
static int history_index = -1;  // -1 significa comando actual (no en historial)
static char current_command_backup[COMMAND_MAX_LEN];  // Backup del comando actual

// Buffer de scroll
static ScrollBuffer scroll_buffer = {0};

// Función para añadir texto al buffer de scroll
void shell_add_to_scroll_buffer(const char* text) {
    if (!text) return;
    
    // Dividir el texto en líneas
    const char* line_start = text;
    while (*line_start) {
        const char* line_end = shell_strchr(line_start, '\n');
        int line_length = line_end ? (line_end - line_start) : shell_strlen(line_start);
        
        // Limitar longitud de línea a 80 caracteres
        if (line_length > 80) line_length = 80;
        
        // Si el buffer está lleno, desplazar hacia arriba
        if (scroll_buffer.line_count >= SHELL_MAX_SCROLL_BUFFER) {
            for (int i = 1; i < SHELL_MAX_SCROLL_BUFFER; i++) {
                memcpy(scroll_buffer.lines[i-1], scroll_buffer.lines[i], 81);
            }
            scroll_buffer.line_count = SHELL_MAX_SCROLL_BUFFER - 1;
        }
        
        // Añadir la nueva línea
        shell_strncpy(scroll_buffer.lines[scroll_buffer.line_count], line_start, line_length);
        scroll_buffer.lines[scroll_buffer.line_count][line_length] = '\0';
        scroll_buffer.line_count++;
        
        if (!line_end) break;
        line_start = line_end + 1;
    }
    
    // Ajustar current_top para mostrar siempre el final
    if (!scroll_buffer.scroll_mode) {
        if (scroll_buffer.line_count > SCREEN_HEIGHT - 1) {
            scroll_buffer.current_top = scroll_buffer.line_count - (SCREEN_HEIGHT - 1);
        } else {
            scroll_buffer.current_top = 0;
        }
    }
}

// Función para redibujar la pantalla desde el buffer de scroll
void shell_redraw_screen(void) {
    VGA_clrscr();
    
    int lines_to_show = SCREEN_HEIGHT - 1; // Dejar una línea para el prompt
    int start_line = scroll_buffer.current_top;
    
    for (int i = 0; i < lines_to_show && (start_line + i) < scroll_buffer.line_count; i++) {
        printf("%s\n", scroll_buffer.lines[start_line + i]);
    }
    
    // Si no estamos en modo scroll, mostrar el prompt
    if (!scroll_buffer.scroll_mode) {
        shell_print_prompt();
        printf("%s", command_buffer);
    } else {
        // En modo scroll, mostrar indicador
        extern int g_ScreenX, g_ScreenY;
        g_ScreenY = SCREEN_HEIGHT - 1;
        g_ScreenX = 0;
        VGA_setcursor(g_ScreenX, g_ScreenY);
        printf("-- SCROLL MODE -- (ESC to exit) Lines: %d-%d of %d", 
               scroll_buffer.current_top + 1, 
               scroll_buffer.current_top + lines_to_show, 
               scroll_buffer.line_count);
    }
}

// Función para hacer scroll hacia arriba
void shell_scroll_up(int lines) {
    if (scroll_buffer.line_count <= SCREEN_HEIGHT - 1) return; // No hay suficientes líneas
    
    if (!scroll_buffer.scroll_mode) {
        scroll_buffer.scroll_mode = true;
        // Capturar el estado actual del comando
        // (ya está en command_buffer)
    }
    
    scroll_buffer.current_top -= lines;
    if (scroll_buffer.current_top < 0) {
        scroll_buffer.current_top = 0;
    }
    
    shell_redraw_screen();
}

// Función para hacer scroll hacia abajo
void shell_scroll_down(int lines) {
    if (scroll_buffer.line_count <= SCREEN_HEIGHT - 1) return; // No hay suficientes líneas
    
    scroll_buffer.current_top += lines;
    
    int max_top = scroll_buffer.line_count - (SCREEN_HEIGHT - 1);
    if (scroll_buffer.current_top >= max_top) {
        scroll_buffer.current_top = max_top;
        // Si llegamos al final, salir del modo scroll
        shell_exit_scroll_mode();
        return;
    }
    
    shell_redraw_screen();
}

// Función para salir del modo scroll
void shell_exit_scroll_mode(void) {
    if (!scroll_buffer.scroll_mode) return;
    
    scroll_buffer.scroll_mode = false;
    
    // Volver a la vista normal (final del buffer)
    if (scroll_buffer.line_count > SCREEN_HEIGHT - 1) {
        scroll_buffer.current_top = scroll_buffer.line_count - (SCREEN_HEIGHT - 1);
    } else {
        scroll_buffer.current_top = 0;
    }
    
    shell_redraw_screen();
}

// Callback para el scroll del ratón
void shell_handle_mouse_scroll(int8_t scroll_delta) {
    if (scroll_delta > 0) {
        // Scroll hacia arriba (rueda hacia arriba)
        shell_scroll_up(3);
    } else if (scroll_delta < 0) {
        // Scroll hacia abajo (rueda hacia abajo)
        shell_scroll_down(3);
    }
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
    int prompt_len = shell_strlen(SHELL_PROMPT);
    
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
    command_pos = shell_strlen(command_buffer);
    
    // Mostrar el comando
    printf("%s", command_buffer);
}

// Cursor position within the command line (for LEFT/RIGHT arrows)
static int cursor_position = 0;

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

// Declaraciones de comandos adicionales
int cmd_scroll_info(int argc, char* argv[]);
int cmd_scroll_test(int argc, char* argv[]);

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
    
    // Scroll testing commands
    {"scrollinfo", "Show scroll buffer information",           cmd_scroll_info},
    {"scrolltest", "Generate test lines for scroll testing",   cmd_scroll_test},
    
    // Red (no implementado)
    {"ping",      "Send ICMP echo requests",                   cmd_ping},
    {"netstat",   "Show network statistics",                   cmd_netstat},
    
    {NULL, NULL, NULL} // Terminador
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
    
    // Inicializar buffer de scroll - usar bucle para evitar overflow
    scroll_buffer.line_count = 0;
    scroll_buffer.current_top = 0;
    scroll_buffer.scroll_mode = false;
    for (int i = 0; i < SHELL_MAX_SCROLL_BUFFER; i++) {
        memset(scroll_buffer.lines[i], 0, 81);
    }
    
    // IMPORTANTE: Activar el modo shell en el sistema de teclado
    keyboard_set_shell_mode(true);
    
    // Configurar callback de scroll del ratón
    mouse_set_scroll_callback(shell_handle_mouse_scroll);
    
    printf("MiqOSoft Shell v1.0\n");
    shell_add_to_scroll_buffer("MiqOSoft Shell v1.0");
    
    printf("Type 'help' for a list of available commands.\n");
    shell_add_to_scroll_buffer("Type 'help' for a list of available commands.");
    
    printf("Use UP/DOWN arrows to navigate command history.\n");
    shell_add_to_scroll_buffer("Use UP/DOWN arrows to navigate command history.");
    
    if (mouse_has_wheel()) {
        printf("Use mouse wheel to scroll through output history.\n");
        shell_add_to_scroll_buffer("Use mouse wheel to scroll through output history.");
    }
    
    printf("\n");
    shell_add_to_scroll_buffer("");
    
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
    shell_add_to_scroll_buffer(SHELL_PROMPT);
    
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
        // Si estamos en modo scroll, manejar teclas especiales
        if (scroll_buffer.scroll_mode) {
            if (c == 0x01) {  // ESC (usando código UP como ESC por simplicidad)
                shell_exit_scroll_mode();
                continue;
            }
            // En modo scroll, ignorar otras teclas
            continue;
        }
        
        if (c == '\n') {
            // Enter presionado - procesar comando
            printf("\n");
            shell_add_to_scroll_buffer("");
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
                int prompt_len = shell_strlen(SHELL_PROMPT);
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
            printf("Command '%s' returned error code ", argv[0]);
            char num_str[12];
            format_number(num_str, result);
            printf("%s\n", num_str);
            shell_add_to_scroll_buffer("Command returned error");
        }
    } else {
        printf("Unknown command: %s\n", argv[0]);
        shell_add_to_scroll_buffer("Unknown command");
        printf("Type 'help' for a list of available commands.\n");
        shell_add_to_scroll_buffer("Type 'help' for a list of available commands.");
    }
}

// ============================================================================
// COMANDOS BUILT-IN
// ============================================================================

int cmd_help(int argc, char* argv[]) {
    printf("Available commands:\n\n");
    shell_add_to_scroll_buffer("Available commands:\n\n");
    
    for (int i = 0; shell_commands[i].name != 0; i++) {
        char line[256];
        snprintf(line, sizeof(line), "  %-12s - %s\n", 
               shell_commands[i].name, 
               shell_commands[i].description);
        printf("%s", line);
        shell_add_to_scroll_buffer(line);
    }
    printf("\n");
    shell_add_to_scroll_buffer("\n");
    return 0;
}

int cmd_history(int argc, char* argv[]) {
    printf("Command history:\n");
    shell_add_to_scroll_buffer("Command history:\n");
    
    if (history_count == 0) {
        printf("  (empty)\n");
        shell_add_to_scroll_buffer("  (empty)\n");
        return 0;
    }
    
    int start = (history_count < HISTORY_SIZE) ? 0 : (history_count % HISTORY_SIZE);
    int count = (history_count < HISTORY_SIZE) ? history_count : HISTORY_SIZE;
    
    for (int i = 0; i < count; i++) {
        int idx = (start + i) % HISTORY_SIZE;
        char line[256];
        snprintf(line, sizeof(line), "  %3d: %s\n", i + 1, command_history[idx]);
        printf("%s", line);
        shell_add_to_scroll_buffer(line);
    }
    
    return 0;
}

int cmd_clear(int argc, char* argv[]) {
    VGA_clrscr();
    // Limpiar también el buffer de scroll
    memset(&scroll_buffer, 0, sizeof(scroll_buffer));
    return 0;
}

int cmd_echo(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        printf("%s", argv[i]);
        shell_add_to_scroll_buffer(argv[i]);
        if (i < argc - 1) {
            printf(" ");
            shell_add_to_scroll_buffer(" ");
        }
    }
    printf("\n");
    shell_add_to_scroll_buffer("\n");
    return 0;
}

int cmd_version(int argc, char* argv[]) {
    char msg[256];
    snprintf(msg, sizeof(msg), "MiqOSoft Kernel v0.17.2\n");
    printf("%s", msg);
    shell_add_to_scroll_buffer(msg);
    
    snprintf(msg, sizeof(msg), "Architecture: i686 (32-bit)\n");
    printf("%s", msg);
    shell_add_to_scroll_buffer(msg);
    
    snprintf(msg, sizeof(msg), "Built with: GCC cross-compiler\n");
    printf("%s", msg);
    shell_add_to_scroll_buffer(msg);
    
    snprintf(msg, sizeof(msg), "Shell: MiqOSoft Shell v1.0\n");
    printf("%s", msg);
    shell_add_to_scroll_buffer(msg);
    
    if (mouse_has_wheel()) {
        snprintf(msg, sizeof(msg), "Mouse: IntelliMouse with wheel support\n");
        printf("%s", msg);
        shell_add_to_scroll_buffer(msg);
    }
    
    return 0;
}

int cmd_reboot(int argc, char* argv[]) {
    printf("Rebooting system...\n");
    shell_add_to_scroll_buffer("Rebooting system...\n");
    // Usar el puerto del teclado para reboot
    i686_outb(0x64, 0xFE);
    return 0;
}

int cmd_panic(int argc, char* argv[]) {
    printf("Triggering kernel panic for testing purposes...\n");
    shell_add_to_scroll_buffer("Triggering kernel panic for testing purposes...\n");
    i686_Panic();
    return 0;
}

int cmd_memory(int argc, char* argv[]) {
    char msg[256];
    snprintf(msg, sizeof(msg), "Memory Information:\n");
    printf("%s", msg);
    shell_add_to_scroll_buffer(msg);
    
    snprintf(msg, sizeof(msg), "  Physical Memory: Not implemented yet\n");
    printf("%s", msg);
    shell_add_to_scroll_buffer(msg);
    
    snprintf(msg, sizeof(msg), "  Available Memory: Not implemented yet\n");
    printf("%s", msg);
    shell_add_to_scroll_buffer(msg);
    
    snprintf(msg, sizeof(msg), "  Kernel Memory: Not implemented yet\n");
    printf("%s", msg);
    shell_add_to_scroll_buffer(msg);
    
    snprintf(msg, sizeof(msg), "  User Memory: Not implemented yet\n");
    printf("%s", msg);
    shell_add_to_scroll_buffer(msg);
    
    // Mostrar información del buffer de scroll
    snprintf(msg, sizeof(msg), "  Scroll Buffer: %d lines used of %d maximum\n", 
             scroll_buffer.line_count, SHELL_MAX_SCROLL_BUFFER);
    printf("%s", msg);
    shell_add_to_scroll_buffer(msg);
    
    return 0;
}

int cmd_uptime(int argc, char* argv[]) {
    printf("Uptime: Not implemented yet\n");
    shell_add_to_scroll_buffer("Uptime: Not implemented yet\n");
    printf("(Timer functionality needs to be added)\n");
    shell_add_to_scroll_buffer("(Timer functionality needs to be added)\n");
    return 0;
}

int cmd_lspci(int argc, char* argv[]) {
    printf("PCI device enumeration not implemented yet.\n");
    shell_add_to_scroll_buffer("PCI device enumeration not implemented yet.\n");
    printf("This would show connected PCI devices.\n");
    shell_add_to_scroll_buffer("This would show connected PCI devices.\n");
    return 0;
}

int cmd_cpuinfo(int argc, char* argv[]) {
    char msg[256];
    snprintf(msg, sizeof(msg), "CPU Information:\n");
    printf("%s", msg);
    shell_add_to_scroll_buffer(msg);
    
    snprintf(msg, sizeof(msg), "  Architecture: i686 (32-bit x86)\n");
    printf("%s", msg);
    shell_add_to_scroll_buffer(msg);
    
    snprintf(msg, sizeof(msg), "  Vendor: (Detection not implemented)\n");
    printf("%s", msg);
    shell_add_to_scroll_buffer(msg);
    
    snprintf(msg, sizeof(msg), "  Model: (Detection not implemented)\n");
    printf("%s", msg);
    shell_add_to_scroll_buffer(msg);
    
    snprintf(msg, sizeof(msg), "  Features: (Detection not implemented)\n");
    printf("%s", msg);
    shell_add_to_scroll_buffer(msg);
    
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
    shell_add_to_scroll_buffer("Directory listing:\n");
    
    for (int i = 0; virtual_fs[i].exists; i++) {
        char line[256];
        if (virtual_fs[i].is_directory) {
            snprintf(line, sizeof(line), "  [DIR]  %s\n", virtual_fs[i].name);
        } else {
            snprintf(line, sizeof(line), "  [FILE] %s\n", virtual_fs[i].name);
        }
        printf("%s", line);
        shell_add_to_scroll_buffer(line);
    }
    return 0;
}

int cmd_cat(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: cat <filename>\n");
        shell_add_to_scroll_buffer("Usage: cat <filename>\n");
        return 1;
    }
    
    for (int i = 0; virtual_fs[i].exists; i++) {
        if (!virtual_fs[i].is_directory && 
            shell_strcmp(virtual_fs[i].name, argv[1]) == 0) {
            printf("%s\n", virtual_fs[i].content);
            shell_add_to_scroll_buffer(virtual_fs[i].content);
            shell_add_to_scroll_buffer("\n");
            return 0;
        }
    }
    
    char error_msg[256];
    snprintf(error_msg, sizeof(error_msg), "cat: %s: No such file\n", argv[1]);
    printf("%s", error_msg);
    shell_add_to_scroll_buffer(error_msg);
    return 1;
}

int cmd_mkdir(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: mkdir <directory>\n");
        shell_add_to_scroll_buffer("Usage: mkdir <directory>\n");
        return 1;
    }
    
    char msg[256];
    snprintf(msg, sizeof(msg), "mkdir: Directory creation not fully implemented\n");
    printf("%s", msg);
    shell_add_to_scroll_buffer(msg);
    
    snprintf(msg, sizeof(msg), "Would create directory: %s\n", argv[1]);
    printf("%s", msg);
    shell_add_to_scroll_buffer(msg);
    return 0;
}

int cmd_rm(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: rm <file>\n");
        shell_add_to_scroll_buffer("Usage: rm <file>\n");
        return 1;
    }
    
    char msg[256];
    snprintf(msg, sizeof(msg), "rm: File deletion not fully implemented\n");
    printf("%s", msg);
    shell_add_to_scroll_buffer(msg);
    
    snprintf(msg, sizeof(msg), "Would remove: %s\n", argv[1]);
    printf("%s", msg);
    shell_add_to_scroll_buffer(msg);
    return 0;
}

// ============================================================================
// COMANDOS DEL KERNEL
// ============================================================================

int cmd_lsmod(int argc, char* argv[]) {
    printf("Loaded kernel modules:\n");
    shell_add_to_scroll_buffer("Loaded kernel modules:\n");
    
    char modules[][256] = {
        "  core         - Core kernel functionality\n",
        "  hal          - Hardware Abstraction Layer\n",
        "  vga_text     - VGA text mode driver\n",
        "  keyboard     - PS/2 keyboard driver\n",
        "  mouse        - PS/2 mouse driver\n",
        "  i8259        - PIC interrupt controller\n"
    };
    
    for (int i = 0; i < 6; i++) {
        printf("%s", modules[i]);
        shell_add_to_scroll_buffer(modules[i]);
    }
    
    return 0;
}

int cmd_dmesg(int argc, char* argv[]) {
    printf("Kernel messages (last few):\n");
    shell_add_to_scroll_buffer("Kernel messages (last few):\n");
    
    char messages[][256] = {
        "[0.000] Kernel started\n",
        "[0.001] GDT initialized\n",
        "[0.002] IDT initialized\n",
        "[0.003] ISR handlers installed\n",
        "[0.004] IRQ handlers installed\n",
        "[0.005] Keyboard driver loaded\n",
        "[0.006] Mouse driver loaded\n",
        "[0.007] Shell initialized\n"
    };
    
    for (int i = 0; i < 8; i++) {
        printf("%s", messages[i]);
        shell_add_to_scroll_buffer(messages[i]);
    }
    
    return 0;
}

int cmd_ps(int argc, char* argv[]) {
    printf("Running processes:\n");
    shell_add_to_scroll_buffer("Running processes:\n");
    
    char processes[][256] = {
        "  PID  PPID  STATE    NAME\n",
        "    0     -  running  kernel\n",
        "    1     0  running  shell\n",
        "\nNote: Process management not fully implemented\n"
    };
    
    for (int i = 0; i < 4; i++) {
        printf("%s", processes[i]);
        shell_add_to_scroll_buffer(processes[i]);
    }
    
    return 0;
}

// Comando para mostrar información del scroll
int cmd_scroll_info(int argc, char* argv[]) {
    char msg[256];
    snprintf(msg, sizeof(msg), "Scroll Buffer Information:\n");
    printf("%s", msg);
    shell_add_to_scroll_buffer(msg);
    
    snprintf(msg, sizeof(msg), "  Total lines: %d\n", scroll_buffer.line_count);
    printf("%s", msg);
    shell_add_to_scroll_buffer(msg);
    
    snprintf(msg, sizeof(msg), "  Current top: %d\n", scroll_buffer.current_top);
    printf("%s", msg);
    shell_add_to_scroll_buffer(msg);
    
    snprintf(msg, sizeof(msg), "  Scroll mode: %s\n", scroll_buffer.scroll_mode ? "ON" : "OFF");
    printf("%s", msg);
    shell_add_to_scroll_buffer(msg);
    
    snprintf(msg, sizeof(msg), "  Max buffer size: %d lines\n", SHELL_MAX_SCROLL_BUFFER);
    printf("%s", msg);
    shell_add_to_scroll_buffer(msg);
    
    if (mouse_has_wheel()) {
        snprintf(msg, sizeof(msg), "  Mouse wheel: Available\n");
    } else {
        snprintf(msg, sizeof(msg), "  Mouse wheel: Not available\n");
    }
    printf("%s", msg);
    shell_add_to_scroll_buffer(msg);
    
    snprintf(msg, sizeof(msg), "\nUse mouse wheel to scroll up/down through history.\n");
    printf("%s", msg);
    shell_add_to_scroll_buffer(msg);
    
    snprintf(msg, sizeof(msg), "Press ESC to exit scroll mode.\n");
    printf("%s", msg);
    shell_add_to_scroll_buffer(msg);
    
    return 0;
}

// Comando para generar texto de prueba para el scroll
int cmd_scroll_test(int argc, char* argv[]) {
    int lines = 30; // Por defecto
    
    if (argc > 1) {
        // Convertir argumento a número
        lines = 0;
        char* num = argv[1];
        while (*num >= '0' && *num <= '9') {
            lines = lines * 10 + (*num - '0');
            num++;
        }
        if (lines <= 0 || lines > 100) lines = 30;
    }
    
    char msg[256];
    snprintf(msg, sizeof(msg), "Generating %d test lines for scroll testing:\n", lines);
    printf("%s", msg);
    shell_add_to_scroll_buffer(msg);
    
    for (int i = 1; i <= lines; i++) {
        snprintf(msg, sizeof(msg), "Line %03d: This is a test line for scroll functionality - Lorem ipsum dolor sit amet\n", i);
        printf("%s", msg);
        shell_add_to_scroll_buffer(msg);
    }
    
    snprintf(msg, sizeof(msg), "\nTest completed. Use mouse wheel to scroll through the output.\n");
    printf("%s", msg);
    shell_add_to_scroll_buffer(msg);
    
    return 0;
}

// Función auxiliar para snprintf (implementación simple)
int snprintf(char* buffer, size_t size, const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    // Implementación muy básica - solo soporta %s, %d, %x
    int written = 0;
    const char* fmt = format;
    
    while (*fmt && written < size - 1) {
        if (*fmt == '%') {
            fmt++;
            switch (*fmt) {
                case 's': {
                    char* str = va_arg(args, char*);
                    while (*str && written < size - 1) {
                        buffer[written++] = *str++;
                    }
                    break;
                }
                case 'd': {
                    int num = va_arg(args, int);
                    if (num == 0) {
                        buffer[written++] = '0';
                    } else {
                        char temp[12];
                        int temp_len = 0;
                        bool negative = num < 0;
                        if (negative) num = -num;
                        
                        while (num > 0) {
                            temp[temp_len++] = '0' + (num % 10);
                            num /= 10;
                        }
                        
                        if (negative && written < size - 1) {
                            buffer[written++] = '-';
                        }
                        
                        for (int i = temp_len - 1; i >= 0 && written < size - 1; i--) {
                            buffer[written++] = temp[i];
                        }
                    }
                    break;
                }
                case 'x': {
                    uint32_t num = va_arg(args, uint32_t);
                    char hex_chars[] = "0123456789abcdef";
                    char temp[9];
                    int temp_len = 0;
                    
                    if (num == 0) {
                        buffer[written++] = '0';
                    } else {
                        while (num > 0) {
                            temp[temp_len++] = hex_chars[num % 16];
                            num /= 16;
                        }
                        
                        for (int i = temp_len - 1; i >= 0 && written < size - 1; i--) {
                            buffer[written++] = temp[i];
                        }
                    }
                    break;
                }
                case '%':
                    buffer[written++] = '%';
                    break;
            }
        } else {
            buffer[written++] = *fmt;
        }
        fmt++;
    }
    
    buffer[written] = '\0';
    va_end(args);
    return written;
}

// Función auxiliar para vsnprintf
int vsnprintf(char* buffer, size_t size, const char* format, va_list args) {
    int written = 0;
    const char* fmt = format;
    
    while (*fmt && written < size - 1) {
        if (*fmt == '%') {
            fmt++;
            switch (*fmt) {
                case 's': {
                    char* str = va_arg(args, char*);
                    while (*str && written < size - 1) {
                        buffer[written++] = *str++;
                    }
                    break;
                }
                case 'd': {
                    int num = va_arg(args, int);
                    if (num == 0) {
                        buffer[written++] = '0';
                    } else {
                        char temp[12];
                        int temp_len = 0;
                        bool negative = num < 0;
                        if (negative) num = -num;
                        
                        while (num > 0) {
                            temp[temp_len++] = '0' + (num % 10);
                            num /= 10;
                        }
                        
                        if (negative && written < size - 1) {
                            buffer[written++] = '-';
                        }
                        
                        for (int i = temp_len - 1; i >= 0 && written < size - 1; i--) {
                            buffer[written++] = temp[i];
                        }
                    }
                    break;
                }
                case 'x': {
                    uint32_t num = va_arg(args, uint32_t);
                    char hex_chars[] = "0123456789abcdef";
                    char temp[9];
                    int temp_len = 0;
                    
                    if (num == 0) {
                        buffer[written++] = '0';
                    } else {
                        while (num > 0) {
                            temp[temp_len++] = hex_chars[num % 16];
                            num /= 16;
                        }
                        
                        for (int i = temp_len - 1; i >= 0 && written < size - 1; i--) {
                            buffer[written++] = temp[i];
                        }
                    }
                    break;
                }
                case '%':
                    buffer[written++] = '%';
                    break;
            }
        } else {
            buffer[written++] = *fmt;
        }
        fmt++;
    }
    
    buffer[written] = '\0';
    return written;
}

// Función auxiliar para vprintf 
int vprintf(const char* format, va_list args) {
    // Esta es una implementación básica que redirige a printf
    // En un sistema real, esto sería más complejo
    char buffer[1024];
    int result = vsnprintf(buffer, sizeof(buffer), format, args);
    
    // Imprimir carácter por carácter usando la función VGA
    for (int i = 0; i < result && buffer[i]; i++) {
        VGA_putc(buffer[i]);
    }
    
    return result;
}