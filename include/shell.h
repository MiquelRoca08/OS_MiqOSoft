#pragma once

#include <stdint.h>
#include <stdbool.h>

//
// Shell Configuration
//

#define SHELL_BUFFER_SIZE 256
#define SHELL_MAX_ARGS 16
#define SHELL_PROMPT "MiqOs> "

//
// Main Functions Of The Shell
//

// Initialization and control
void shell_init(void);
void shell_run(void);
void shell_print_prompt(void);

// Command processing
void shell_process_command(const char* input);
int shell_parse_command(const char* input, char* argv[]);

// Command line management
char* shell_get_current_line(void);
void shell_display_command(const char* cmd);
void shell_clear_line(void);
void shell_redraw_screen(void);

// Navigation and cursor
void shell_move_cursor_left(void);
void shell_move_cursor_right(void);

//
// String Functions For The Shell
//

// Shell internal string functions (avoids conflicts with libc)
int shell_strlen(const char* str);
int shell_strcmp(const char* s1, const char* s2);
char* shell_strchr(const char* str, int c);
char* shell_strstr(const char* haystack, const char* needle);
char* shell_strncpy(char* dest, const char* src, int n);

//
// Keyboard Integration
//

// Function to control shell mode in keyboard.c
void keyboard_set_shell_mode(bool enabled);

//
// Compatibility Features (for commands that need them)
//

// Helper function for formatting numbers (used by some commands)
void format_number(char* buffer, int num);