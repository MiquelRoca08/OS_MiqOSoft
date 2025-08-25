#pragma once

#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// CONFIGURACIÓN DE LA SHELL
// ============================================================================

#define SHELL_BUFFER_SIZE 256
#define SHELL_MAX_ARGS 16
#define SHELL_PROMPT "MiqOs> "

// ============================================================================
// FUNCIONES PRINCIPALES DE LA SHELL
// ============================================================================

// Inicialización y control
void shell_init(void);
void shell_run(void);
void shell_print_prompt(void);

// Procesamiento de comandos
void shell_process_command(const char* input);
int shell_parse_command(const char* input, char* argv[]);

// Gestión de línea de comandos
char* shell_get_current_line(void);
void shell_display_command(const char* cmd);
void shell_clear_line(void);
void shell_redraw_screen(void);

// Navegación y cursor
void shell_move_cursor_left(void);
void shell_move_cursor_right(void);

// Historial
void history_add_command(const char* command);
const char* history_get_previous(void);
const char* history_get_next(void);
int shell_show_history(void);

// ============================================================================
// FUNCIONES DE STRING PARA LA SHELL
// ============================================================================

// Funciones de string internas de la shell (evita conflictos con libc)
int shell_strlen(const char* str);
int shell_strcmp(const char* s1, const char* s2);
char* shell_strchr(const char* str, int c);
char* shell_strstr(const char* haystack, const char* needle);
char* shell_strncpy(char* dest, const char* src, int n);

// ============================================================================
// INTEGRACIÓN CON EL TECLADO
// ============================================================================

// Función para controlar el modo shell en keyboard.c
void keyboard_set_shell_mode(bool enabled);

// ============================================================================
// FUNCIONES DE COMPATIBILIDAD (para comandos que las necesiten)
// ============================================================================

// Función auxiliar para formatear números (utilizada por algunos comandos)
void format_number(char* buffer, int num);