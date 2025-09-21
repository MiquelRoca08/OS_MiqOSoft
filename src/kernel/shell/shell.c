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

#define COMMAND_MAX_LEN 256

// Definition of size_t if not available
#ifndef SIZE_T_DEFINED
#define SIZE_T_DEFINED
typedef unsigned long size_t;
#endif

// Buffer for the current command line
static char command_buffer[SHELL_BUFFER_SIZE];
static int command_pos = 0;
static int cursor_position = 0;  // Cursor position in the command line
static bool shell_running = false;

//
// String Functions
//

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

//
// Shell Interface Functions
//

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
    
    // Go to the beginning of the line (after the prompt)
    g_ScreenX = prompt_len;
    VGA_setcursor(g_ScreenX, g_ScreenY);
    
    // Erase to the end of the line
    for (int i = 0; i < command_pos; i++) {
        printf(" ");
    }
    
    // Go back to the beginning
    g_ScreenX = prompt_len;
    VGA_setcursor(g_ScreenX, g_ScreenY);
}

void shell_display_command(const char* cmd) {
    // Clear current line
    shell_clear_line();
    
    // Copy new command
    shell_strncpy(command_buffer, cmd, SHELL_BUFFER_SIZE - 1);
    command_buffer[SHELL_BUFFER_SIZE - 1] = '\0';
    command_pos = shell_strlen(command_buffer);
    
    // Display the command
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

//
// Main Shell Functions
//

void shell_init(void) {
    memset(command_buffer, 0, sizeof(command_buffer));
    command_pos = 0;
    cursor_position = 0;
    shell_running = true;
    
    keyboard_set_shell_mode(true);
    
    printf("Type 'help' for a list of available commands.\n");
    printf("Total commands available: %d\n", get_shell_command_count());
    
    printf("\n");
    
    // Synchronize the keyboard system with the current VGA cursor position
    extern int cursor_line, cursor_pos, total_lines_used;
    extern int line_lengths[];
    
    cursor_line = VGA_get_cursor_y();
    cursor_pos = VGA_get_cursor_x();
    total_lines_used = cursor_line + 1;
    line_lengths[cursor_line] = 0;
    
    shell_print_prompt();
}

void shell_print_prompt(void) {
    // Synchronize keyboard cursor with current VGA position
    extern int cursor_line, cursor_pos, total_lines_used;
    extern int line_lengths[];
    
    printf(SHELL_PROMPT);
    
    // Update keyboard system variables
    cursor_line = VGA_get_cursor_y();
    cursor_pos = VGA_get_cursor_x();
    total_lines_used = cursor_line + 1;
    line_lengths[cursor_line] = 0;
}

void shell_run(void) {
    char c;
    
    // Read characters directly from the circular buffer of keyboard.c
    while (keyboard_buffer_pop(&c)) {
        if (c == '\n') {
            // Enter pressed - process command
            printf("\n");
            command_buffer[command_pos] = '\0';

            if (command_pos > 0) {
                // Add to history before processing
                shell_process_command(command_buffer);
            }

            // Reset buffer and cursor
            memset(command_buffer, 0, sizeof(command_buffer));
            command_pos = 0;
            cursor_position = 0;
            shell_print_prompt();
            
        } else if (c == '\b') {
            // Backspace
            if (cursor_position > 0 && command_pos > 0) {
                // If the cursor is not at the end, move characters
                if (cursor_position < command_pos) {
                    for (int i = cursor_position - 1; i < command_pos - 1; i++) {
                        command_buffer[i] = command_buffer[i + 1];
                    }
                }

                command_pos--;
                cursor_position--;
                command_buffer[command_pos] = '\0';

                // Redraw line
                extern int g_ScreenX, g_ScreenY;
                int prompt_len = shell_strlen(SHELL_PROMPT);
                g_ScreenX = prompt_len;
                VGA_setcursor(g_ScreenX, g_ScreenY);

                // Clear line and redraw
                for (int i = 0; i <= command_pos + 1; i++) printf(" ");
                g_ScreenX = prompt_len;
                VGA_setcursor(g_ScreenX, g_ScreenY);
                printf("%s", command_buffer);

                // Position cursor
                g_ScreenX = prompt_len + cursor_position;
                VGA_setcursor(g_ScreenX, g_ScreenY);
            }
            
        } else if (c == 0x01) { // TODO:
            // UP - previous history

        } else if (c == 0x02) { // TODO:
            // DOWN - next history

        } else if (c == 0x03) {
            // LEFT - move cursor left
            shell_move_cursor_left();
            
        } else if (c == 0x04) {
            // RIGHT - move cursor right
            shell_move_cursor_right();
            
        } else if (c >= 32 && c <= 126) {
            // Printable character
            if (command_pos < SHELL_BUFFER_SIZE - 1) {
                // If cursor is not at the end, insert character
                if (cursor_position < command_pos) {
                    for (int i = command_pos; i > cursor_position; i--) {
                        command_buffer[i] = command_buffer[i - 1];
                    }
                }

                command_buffer[cursor_position] = c;
                command_pos++;
                cursor_position++;

                // Redraw from cursor to end
                extern int g_ScreenX, g_ScreenY;
                int start_x = g_ScreenX;
                printf("%s", &command_buffer[cursor_position - 1]);
                g_ScreenX = start_x + 1;
                VGA_setcursor(g_ScreenX, g_ScreenY);
            }
        } else if (c != 0) {
            // Special characters (accents, ñ, etc.)
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
    
    // Copy input to local buffer
    while (input[i] && i < SHELL_BUFFER_SIZE - 1) {
        local_buffer[i] = input[i];
        i++;
    }
    local_buffer[i] = '\0';
    
    // Parse arguments
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
    
    // Search command in the unified table
    const ShellCommandEntry* cmd = find_shell_command(argv[0]);
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

// Function to get the current line (required by shell.h header)
char* shell_get_current_line(void) {
    return command_buffer;
}