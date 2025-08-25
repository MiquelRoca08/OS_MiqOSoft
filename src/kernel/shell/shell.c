#include <shell.h>
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

// Sistema de historial
static char command_history[HISTORY_SIZE][COMMAND_MAX_LEN];
static int history_count = 0;
static int history_index = -1;  // -1 significa comando actual (no en historial)
static char current_command_backup[COMMAND_MAX_LEN];  // Backup del comando actual

// ============================================================================
// FUNCIONES DE STRING PARA LA SHELL
// ============================================================================

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

// ============================================================================
// GESTIÓN DEL HISTORIAL
// ============================================================================

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

int shell_show_history(void) {
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

// ============================================================================
// FUNCIONES DE INTERFACE DE LA SHELL
// ============================================================================

void shell_redraw_screen(void) {
    extern int g_ScreenX, g_ScreenY;
    VGA_clrscr();
    shell_print_prompt();
    printf("%s", command_buffer);
    g_ScreenX = shell_strlen(SHELL_PROMPT) + cursor_position;
    g_ScreenY = SCREEN_HEIGHT - 1;
    VGA_setcursor(g_ScreenX, g_ScreenY);
}

void shell_clear_line(void) {
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

// ============================================================================
// FUNCIONES PRINCIPALES DE LA SHELL
// ============================================================================

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
    printf("Total commands available: %d\n", get_unified_command_count());
    
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

void shell_process_command(const char* input) {
    char* argv[SHELL_MAX_ARGS];
    int argc = shell_parse_command(input, argv);
    
    if (argc == 0) return;
    
    // Buscar comando en la tabla unificada
    const ShellCommandEntry* cmd = find_unified_command(argv[0]);
    if (cmd && cmd->function) {
        int result = cmd->function(argc, argv);
        if (result != 0) {
            printf("Command '%s' returned error code %d\n", argv[0], result);
        }
    } else {
        printf("Unknown command: %s\n", argv[0]);
        printf("Type 'help' for a list of available commands.\n");
    }
}

// Función para obtener la línea actual (requerida por el header shell.h)
char* shell_get_current_line(void) {
    return command_buffer;
}