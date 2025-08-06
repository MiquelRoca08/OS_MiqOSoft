#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
#include <stdbool.h>
#include "../arch/i686/isr.h"
#include "../arch/i686/io.h"

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
void keyboard_init(void);
void keyboard_handler(Registers* regs);
char keyboard_get_scancode(void);
char keyboard_scancode_to_ascii(uint8_t scancode);

#endif // KEYBOARD_H
