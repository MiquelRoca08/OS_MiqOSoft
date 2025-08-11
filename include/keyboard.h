#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <isr.h>
#include <io.h>

// Puertos del teclado PS/2
#define KEYBOARD_DATA_PORT    0x60
#define KEYBOARD_STATUS_PORT  0x64
#define KEYBOARD_COMMAND_PORT 0x64

// Bits de estado
#define KEYBOARD_OUTPUT_BUFFER_FULL 0x01
#define KEYBOARD_INPUT_BUFFER_FULL  0x02

// Teclas especiales
#define KEY_RELEASE 0x80
#define KEY_SHIFT   0x2A
#define KEY_ALTGR   0x38

#define KEY_DEAD_KEY 0xFF

// Funciones públicas
void redraw_input_line(void);
void keyboard_init(void);
void keyboard_handler(Registers* regs);
void keyboard_process_buffer(void);
char keyboard_get_scancode(void);
char keyboard_scancode_to_ascii(uint8_t scancode);
void keyboard_reset_dead_state(void);
int keyboard_is_caps_lock_on(void);
void keyboard_set_caps_lock(int state);