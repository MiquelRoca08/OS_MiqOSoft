#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <isr.h>

// Puertos del ratón PS/2
#define MOUSE_DATA_PORT     0x60
#define MOUSE_STATUS_PORT   0x64
#define MOUSE_COMMAND_PORT  0x64

// Comandos del controlador
#define MOUSE_CMD_ENABLE_AUX    0xA8
#define MOUSE_CMD_DISABLE_AUX   0xA7
#define MOUSE_CMD_WRITE_AUX     0xD4

// Comandos del ratón
#define MOUSE_CMD_RESET         0xFF
#define MOUSE_CMD_SET_DEFAULTS  0xF6
#define MOUSE_CMD_ENABLE        0xF4
#define MOUSE_CMD_DISABLE       0xF5
#define MOUSE_CMD_SET_SAMPLE    0xF3
#define MOUSE_CMD_GET_ID        0xF2

// Respuestas del ratón
#define MOUSE_ACK               0xFA
#define MOUSE_NACK              0xFE

// Bits del paquete del ratón
#define MOUSE_BUTTON_LEFT       0x01
#define MOUSE_BUTTON_RIGHT      0x02
#define MOUSE_BUTTON_MIDDLE     0x04
#define MOUSE_X_OVERFLOW        0x40
#define MOUSE_Y_OVERFLOW        0x80

// Estructura para los datos del ratón
typedef struct {
    uint8_t flags;
    int16_t x_delta;
    int16_t y_delta;
    int8_t z_delta;  // Para la rueda
    bool left_button;
    bool right_button;
    bool middle_button;
} MousePacket;

// Funciones públicas
void mouse_init(void);
void mouse_handler(Registers* regs);
bool mouse_has_wheel(void);
MousePacket* mouse_get_packet(void);
void mouse_set_scroll_callback(void (*callback)(int8_t scroll_delta));

// Callback para el scroll
extern void (*mouse_scroll_callback)(int8_t scroll_delta);