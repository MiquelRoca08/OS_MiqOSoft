#include <pit.h>
#include <io.h>
#include <debug.h>

#define PIT_CHANNEL0 0x40
#define PIT_COMMAND  0x43
#define PIT_INPUT_HZ 1193182

void pit_init(uint32_t frequency) {
    // Calculate divisor - ensure it's within valid range
    if (frequency < 19 || frequency > PIT_INPUT_HZ) {
        log_debug("PIT", "Invalid frequency %d Hz (must be between 19 and %d)", frequency, PIT_INPUT_HZ);
        return;
    }
    
    uint16_t divisor = (uint16_t)(PIT_INPUT_HZ / frequency);
    
    log_debug("PIT", "Input frequency: %d Hz", PIT_INPUT_HZ);
    log_debug("PIT", "Requested frequency: %d Hz", frequency);
    log_debug("PIT", "Calculated divisor: %d (0x%X)", divisor, divisor);
    log_debug("PIT", "Actual frequency will be: %d Hz", PIT_INPUT_HZ / divisor);
    
    // Debug: Check if divisor is correct for 1000Hz
    if (frequency == 1000) {
        log_debug("PIT", "DEBUG: For 1000Hz, expected divisor ~1193, got %d", divisor);
    }

    /* 
     * Command byte: 0x36
     * - Channel 0 (00)
     * - Access mode: lobyte/hibyte (11) 
     * - Operating mode: square wave generator (011)
     * - Binary mode (0)
     */
    log_debug("PIT", "Command byte: 0x36 (channel 0, lobyte/hibyte, mode 3, binary)");
    outb(PIT_COMMAND, 0x36);

    /* Divisor en low y high byte */
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF));
    
    log_debug("PIT", "Configuration complete");
}
