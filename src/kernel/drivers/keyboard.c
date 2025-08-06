#include "keyboard.h"
#include "../arch/i686/io.h"
#include "../arch/i686/isr.h"
#include "../stdio.h"
#include <stdint.h>
#include <stdbool.h>

/*
┌──────┐      ┌──────┬──────┬──────┬──────┐  ┌──────┬──────┬──────┬──────┐  ┌──────┬──────┬─────┬─────┐   ┌──────┐
│ Esc  │      │ F1   │ F2   │ F3   │ F4   │  │ F5   │ F6   │ F7   │ F8   │  │ F9   │ F10  │ F11 │ F12 │   │ DEL  │
│01    │      │3B    │3C    │3D    │3E    │  │3F    │40    │41    │42    │  │43    │44    │57   │58   │   │E0 53 │
├──────┼──────┼──────┼──────┼──────┼──────┼──┴───┬──┴───┬──┴───┬──┴───┬──┴──┴┬─────┴┬─────┴┬────┴─────┤   └──────┘
│  º   │ 1    │ 2    │ 3    │ 4    │ 5    │ 6    │ 7    │ 8    │ 9    │ 0    │ '    │ ¡    │ Backspace│
│ 29   │02    │03    │04    │05    │06    │07    │08    │09    │0A    │0B    │0C    │0D    │0E        │
├──────┴───┬──┴───┬──┴───┬──┴───┬──┴───┬──┴───┬──┴───┬──┴───┬──┴───┬──┴───┬──┴───┬──┴───┬──┴───┬──────┤
│ Tab      │ Q    │ W    │ E    │ R    │ T    │ Y    │ U    │ I    │ O    │ P    │ `    │ +    │ Ç    │
│0F        │10    │11    │12    │13    │14    │15    │16    │17    │18    │19    │1A    │1B    │2B    │
├──────────┴┬─────┴┬─────┴┬─────┴┬─────┴┬─────┴┬─────┴┬─────┴┬─────┴┬─────┴┬─────┴┬─────┴┬─────┴──────┤
│ Caps      │ A    │ S    │ D    │ F    │ G    │ H    │ J    │ K    │ L    │ Ñ    │ ´    │ Enter      │
│3A         │1E    │1F    │20    │21    │22    │23    │24    │25    │26    │27    │28    │1C          │
├────────┬──┴───┬──┴───┬──┴───┬──┴───┬──┴───┬──┴───┬──┴───┬──┴───┬──┴───┬──┴───┬──┴───┬──┴────────────┤
│ Shift  │ <    │ Z    │ X    │ C    │ V    │ B    │ N    │ M    │ ,    │ .    │ -    │ Right Shift   │
│2A      │56    │2C    │2D    │2E    │2F    │30    │31    │32    │33    │34    │35    │36             │
├────────┼──────┼──────┼──────┼──────┴──────┴──────┴──────┴──────┼──────┼──────┴┬─────┴┬───────┬──────┤
│ Ctrl   │ FN   │ Win  │ Alt  │ Space                            │ AltG │ Ctrl  │      │↑ E0 48│      │
│1D      │      │E0    │38    │39                                │E0 38 │1D     │──────┼───────┼──────┤
└────────┴──────┴──────┴──────┴──────────────────────────────────┴──────┴───────┤←E0 4B│↓ E0 50│→E0 4D│
                                                                                └──────┴───────┴──────┘
*/

#define KEY_RELEASE  0x80
#define KEY_SHIFT    0x2A
#define KEY_RSHIFT   0x36
#define KEY_ALTGR    0x38
#define KEY_CAPS     0x3A
#define KEY_ESCAPE   0x01
#define KEY_F1       0x3B
#define KEY_F2       0x3C
#define KEY_F3       0x3D
#define KEY_F4       0x3E
#define KEY_F5       0x3F
#define KEY_F6       0x40
#define KEY_F7       0x41
#define KEY_F8       0x42
#define KEY_F9       0x43
#define KEY_F10      0x44

// Extended keys (preceded by 0xE0)
#define KEY_UP       0x48
#define KEY_DOWN     0x50
#define KEY_LEFT     0x4B
#define KEY_RIGHT    0x4D
#define KEY_DELETE   0x53

// Dead key states
#define DEAD_NONE        0x00
#define DEAD_ACUTE       0x01  // ´
#define DEAD_DIERESIS    0x02  // ¨
#define DEAD_GRAVE       0x03  // `
#define DEAD_TILDE       0x04  // ~
#define DEAD_CIRCUMFLEX  0x05  // ^

// Special Characters
#define CHAR_a_ACUTE       0xA0
#define CHAR_e_ACUTE       0x82
#define CHAR_i_ACUTE       0xA1
#define CHAR_o_ACUTE       0xA2
#define CHAR_u_ACUTE       0xA3
#define CHAR_u_DIERESIS    0x81
#define CHAR_A_ACUTE       0xB5
#define CHAR_E_ACUTE       0x90
#define CHAR_I_ACUTE       0xD6
#define CHAR_O_ACUTE       0xE0
#define CHAR_U_ACUTE       0xE9
#define CHAR_U_DIERESIS    0x9A
#define CHAR_n_TILDE       0xA4
#define CHAR_N_TILDE       0xA5
#define CHAR_INVERTED_EXCL 0xAD
#define CHAR_INVERTED_QUEST 0xA8
#define CHAR_DEGREE        0xF8
#define CHAR_FEMININE      0xA6
#define CHAR_MIDDLE_DOT    0xFA
#define CHAR_EURO          0xEE

static int shift_pressed = 0;
static int altgr_pressed = 0;
static int caps_lock = 0;
static int extended_key = 0;  // For E0 prefixed keys
static uint8_t dead_key_state = DEAD_NONE;

typedef struct {
    uint8_t scancode;
    char normal;
    char shift;
    char altgr;
    bool is_dead_key;
} KeyMapping;

static const KeyMapping keymap[] = {
    {0x02, '1', '!', '|', false},
    {0x03, '2', '"', '@', false},
    {0x04, '3', CHAR_MIDDLE_DOT, '#', false},
    {0x05, '4', '$', '~', false},
    {0x06, '5', '%', CHAR_EURO, false},
    {0x07, '6', '&', '\xAA', false}, // ¬
    {0x08, '7', '/', 0, false},
    {0x09, '8', '(', 0, false},
    {0x0A, '9', ')', 0, false},
    {0x0B, '0', '=', 0, false},
    {0x0C, '\'', '?', 0, false},
    {0x0D, CHAR_INVERTED_EXCL, CHAR_INVERTED_QUEST, 0, false},
    {0x0E, '\b', '\b', '\b', false},  // Backspace
    {0x0F, '\t', '\t', '\t', false},
    {0x10, 'q', 'Q', 0, false},
    {0x11, 'w', 'W', 0, false},
    {0x12, 'e', 'E', CHAR_EURO, false},
    {0x13, 'r', 'R', 0, false},
    {0x14, 't', 'T', 0, false},
    {0x15, 'y', 'Y', 0, false},
    {0x16, 'u', 'U', 0, false},
    {0x17, 'i', 'I', 0, false},
    {0x18, 'o', 'O', 0, false},
    {0x19, 'p', 'P', 0, false},
    {0x1A, '`', '^', '[', true},   // Dead key: ` (grave) y ^ (circumflex), AltGr = [
    {0x1B, '+', '*', ']', false},  // AltGr = ]
    {0x1C, '\n', '\n', '\n', false},
    {0x1E, 'a', 'A', 0, false},
    {0x1F, 's', 'S', 0, false},
    {0x20, 'd', 'D', 0, false},
    {0x21, 'f', 'F', 0, false},
    {0x22, 'g', 'G', 0, false},
    {0x23, 'h', 'H', 0, false},
    {0x24, 'j', 'J', 0, false},
    {0x25, 'k', 'K', 0, false},
    {0x26, 'l', 'L', 0, false},
    {0x27, CHAR_n_TILDE, CHAR_N_TILDE, 0, false},
    {0x28, 0, 0, '{', true},       // Dead key: ´ (acute) y ¨ (dieresis), AltGr = {
    {0x29, CHAR_DEGREE, CHAR_FEMININE, '\\', false},
    {0x2B, '\x87', '\x80', '}', false}, // ç y Ç, AltGr = }
    {0x56, '<', '>', 0, false},
    {0x2C, 'z', 'Z', 0, false},
    {0x2D, 'x', 'X', 0, false},
    {0x2E, 'c', 'C', 0, false},
    {0x2F, 'v', 'V', 0, false},
    {0x30, 'b', 'B', 0, false},
    {0x31, 'n', 'N', 0, false},
    {0x32, 'm', 'M', 0, false},
    {0x33, ',', ';', 0, false},
    {0x34, '.', ':', 0, false},
    {0x35, '-', '_', 0, false},
    {0x37, '*', '*', '*', false},
    {0x39, ' ', ' ', ' ', false}
};

// Composition table for dead keys
typedef struct {
    uint8_t dead_type;
    char base_char;
    char result;
} DeadKeyComposition;

static const DeadKeyComposition dead_compositions[] = {
    {DEAD_ACUTE, 'a', CHAR_a_ACUTE},
    {DEAD_ACUTE, 'A', CHAR_A_ACUTE},
    {DEAD_ACUTE, 'e', CHAR_e_ACUTE},
    {DEAD_ACUTE, 'E', CHAR_E_ACUTE},
    {DEAD_ACUTE, 'i', CHAR_i_ACUTE},
    {DEAD_ACUTE, 'I', CHAR_I_ACUTE},
    {DEAD_ACUTE, 'o', CHAR_o_ACUTE},
    {DEAD_ACUTE, 'O', CHAR_O_ACUTE},
    {DEAD_ACUTE, 'u', CHAR_u_ACUTE},
    {DEAD_ACUTE, 'U', CHAR_U_ACUTE},
    
    {DEAD_DIERESIS, 'u', CHAR_u_DIERESIS},
    {DEAD_DIERESIS, 'U', CHAR_U_DIERESIS},
    
    {DEAD_GRAVE, 'a', 0x85},  // à
    {DEAD_GRAVE, 'A', 0x85},  // À
    {DEAD_GRAVE, 'e', 0x8A},  // è
    {DEAD_GRAVE, 'E', 0x8A},  // È
    {DEAD_GRAVE, 'i', 0x8D},  // ì
    {DEAD_GRAVE, 'I', 0x8D},  // Ì
    {DEAD_GRAVE, 'o', 0x93},  // ò
    {DEAD_GRAVE, 'O', 0x93},  // Ò
    {DEAD_GRAVE, 'u', 0x97},  // ù
    {DEAD_GRAVE, 'U', 0x97},  // Ù
    
    {DEAD_TILDE, 'n', CHAR_n_TILDE},
    {DEAD_TILDE, 'N', CHAR_N_TILDE},
    {DEAD_TILDE, 'a', 0xC6},  // ã
    {DEAD_TILDE, 'A', 0xC6},  // Ã
    {DEAD_TILDE, 'o', 0xE4},  // õ
    {DEAD_TILDE, 'O', 0xE4},  // Õ
    
    {DEAD_CIRCUMFLEX, 'a', 0x83},  // â
    {DEAD_CIRCUMFLEX, 'A', 0x83},  // Â
    {DEAD_CIRCUMFLEX, 'e', 0x88},  // ê
    {DEAD_CIRCUMFLEX, 'E', 0x88},  // Ê
    {DEAD_CIRCUMFLEX, 'i', 0x8C},  // î
    {DEAD_CIRCUMFLEX, 'I', 0x8C},  // Î
    {DEAD_CIRCUMFLEX, 'o', 0x93},  // ô
    {DEAD_CIRCUMFLEX, 'O', 0x93},  // Ô
    {DEAD_CIRCUMFLEX, 'u', 0x96},  // û
    {DEAD_CIRCUMFLEX, 'U', 0x96},  // Û
};

void keyboard_init(void) {
    i686_outb(0x21, i686_inb(0x21) & ~(1 << 1));
    while(i686_inb(KEYBOARD_STATUS_PORT) & KEYBOARD_OUTPUT_BUFFER_FULL) {
        i686_inb(KEYBOARD_DATA_PORT);
    }
}

static uint8_t get_dead_key_type(uint8_t scancode) {
    switch (scancode) {
        case 0x1A:  // ` y ^
            return shift_pressed ? DEAD_CIRCUMFLEX : DEAD_GRAVE;
        case 0x28:  // ´ y ¨ 
            return shift_pressed ? DEAD_DIERESIS : DEAD_ACUTE;
        default:
            return DEAD_NONE;
    }
}

static char find_dead_key_composition(uint8_t dead_type, char base_char) {
    for (int i = 0; i < sizeof(dead_compositions)/sizeof(DeadKeyComposition); i++) {
        if (dead_compositions[i].dead_type == dead_type && 
            dead_compositions[i].base_char == base_char) {
            return dead_compositions[i].result;
        }
    }
    return 0;
}

static void output_dead_key_symbol(uint8_t dead_type) {
    switch (dead_type) {
        case DEAD_ACUTE:     putc(0xEF); break;  // ´
        case DEAD_DIERESIS:  putc(0xF9); break;  // ¨
        case DEAD_GRAVE:     putc('`'); break;   // `
        case DEAD_TILDE:     putc('~'); break;   // ~
        case DEAD_CIRCUMFLEX: putc('^'); break;  // ^
    }
}

// Function to apply caps lock logic
static char apply_caps_lock(char ascii) {
    if (caps_lock && ascii >= 'a' && ascii <= 'z') {
        return ascii - 'a' + 'A';  // Convert to uppercase
    } else if (caps_lock && ascii >= 'A' && ascii <= 'Z') {
        return ascii - 'A' + 'a';  // Convert to lowercase (if shift is pressed)
    }
    return ascii;
}

// Function to handle arrow keys
static void handle_arrow_key(uint8_t scancode) {
    switch (scancode) {
        case KEY_UP:
            printf("[UP]");
            // You can implement cursor movement here
            break;
        case KEY_DOWN:
            printf("[DOWN]");
            // You can implement cursor movement here
            break;
        case KEY_LEFT:
            printf("[LEFT]");
            // You can implement cursor movement here
            break;
        case KEY_RIGHT:
            printf("[RIGHT]");
            // You can implement cursor movement here
            break;
    }
}

void keyboard_handler(Registers* regs) {
    uint8_t scancode = keyboard_get_scancode();

    // Handle extended keys (arrow keys, etc.)
    if (extended_key) {
        extended_key = 0;  // Reset flag
        switch (scancode) {
            case 0x38:  // AltGr presionado
                altgr_pressed = 1;
                return;
            case 0xB8:  // AltGr soltado
                altgr_pressed = 0;
                return;
            case KEY_UP:
            case KEY_DOWN:
            case KEY_LEFT:
            case KEY_RIGHT:
                handle_arrow_key(scancode);
                break;
            case KEY_DELETE:
                printf("[DEL]");
                break;
        }
        return;
    }

    // Handle key releases
    if (scancode & KEY_RELEASE) {
        uint8_t key = scancode & ~KEY_RELEASE;
        
        if (extended_key) {
            extended_key = 0;  // Reset extended key flag
            // Handle extended key releases if needed
            return;
        }
        
        if (key == KEY_SHIFT || key == KEY_RSHIFT) {
            shift_pressed = 0;
        } else if (key == KEY_ALTGR) {
            altgr_pressed = 0;
        }
        return;
    }

    // Handle extended keys (arrow keys, etc.)
    if (extended_key) {
        extended_key = 0;  // Reset flag
        switch (scancode) {
            case KEY_UP:
            case KEY_DOWN:
            case KEY_LEFT:
            case KEY_RIGHT:
                handle_arrow_key(scancode);
                break;
            case KEY_DELETE:
                printf("[DEL]");
                // Implement delete functionality here
                break;
        }
        return;
    }

    // Handle modifier keys
    if (scancode == KEY_SHIFT || scancode == KEY_RSHIFT) {
        shift_pressed = 1;
        return;
    } else if (scancode == KEY_ALTGR) {
        altgr_pressed = 1;
        return;
    } else if (scancode == KEY_CAPS) {
        caps_lock = !caps_lock;  // Toggle caps lock
        return;
    }

    // Find the key mapping
    const KeyMapping* mapping = NULL;
    for (int i = 0; i < sizeof(keymap)/sizeof(KeyMapping); i++) {
        if (keymap[i].scancode == scancode) {
            mapping = &keymap[i];
            break;
        }
    }

    if (!mapping) return;

    // Handle dead keys
    if (mapping->is_dead_key) {
        // Check if AltGr is pressed and there's an altgr character
        if (altgr_pressed && mapping->altgr) {
            char ascii = mapping->altgr;
            putc(ascii);  // Output [ or { directly
            return;
        }
        
        uint8_t new_dead_state = get_dead_key_type(scancode);
        
        if (dead_key_state != DEAD_NONE) {
            output_dead_key_symbol(dead_key_state);
        }
        
        dead_key_state = new_dead_state;
        return;
    }

    // Determine which character to output
    char ascii = 0;
    if (altgr_pressed && mapping->altgr) {
        ascii = mapping->altgr;
    } else if (shift_pressed && mapping->shift) {
        ascii = mapping->shift;
    } else {
        ascii = mapping->normal;
    }
    
    if (!ascii) return;

    // Apply caps lock for letters (but not if shift or altgr is pressed)
    if (!shift_pressed && !altgr_pressed) {
        ascii = apply_caps_lock(ascii);
    }

    // Handle dead key composition
    if (dead_key_state != DEAD_NONE) {
        char composed = find_dead_key_composition(dead_key_state, ascii);
        
        if (composed) {
            putc(composed);
        } else {
            output_dead_key_symbol(dead_key_state);
            putc(ascii);
        }
        
        dead_key_state = DEAD_NONE;
        return;
    }

    // Output the character
    putc(ascii);
}

char keyboard_get_scancode(void) {
    while (!(i686_inb(KEYBOARD_STATUS_PORT) & KEYBOARD_OUTPUT_BUFFER_FULL));
    return i686_inb(KEYBOARD_DATA_PORT);
}

char keyboard_scancode_to_ascii(uint8_t scancode) {
    for (int i = 0; i < sizeof(keymap)/sizeof(KeyMapping); i++) {
        if (keymap[i].scancode == scancode) {
            char ascii = 0;
            if (altgr_pressed && keymap[i].altgr) {
                ascii = keymap[i].altgr;
            } else if (shift_pressed && keymap[i].shift) {
                ascii = keymap[i].shift;
            } else {
                ascii = keymap[i].normal;
            }
            
            // Apply caps lock if applicable
            if (!shift_pressed && !altgr_pressed) {
                ascii = apply_caps_lock(ascii);
            }
            
            return ascii;
        }
    }
    return 0;
}

void keyboard_reset_dead_state(void) {
    dead_key_state = DEAD_NONE;
}

// Additional utility functions
int keyboard_is_caps_lock_on(void) {
    return caps_lock;
}

void keyboard_set_caps_lock(int state) {
    caps_lock = state;
}