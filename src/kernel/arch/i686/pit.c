#include <pit.h>
#include <io.h>

#define PIT_CHANNEL0 0x40
#define PIT_COMMAND  0x43
#define PIT_INPUT_HZ 1193182

void pit_init(uint32_t frequency) {
    uint16_t divisor = (uint16_t)(PIT_INPUT_HZ / frequency);

    /* Channel 0, acceso low+high byte, modo 3 (square wave), binario */
    outb(PIT_COMMAND, 0x36);

    /* Divisor en low y high byte */
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF));
}
