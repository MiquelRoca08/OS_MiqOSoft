#include <time.h>
#include <stdint.h>
#include <io.h>
#include <debug.h>
#include <irq.h>
#include <isr.h>
#include <pit.h>
#include <stdio.h>

/* Frecuencia del PIT (Hz). 1000 → un tick = 1ms */
#define HZ 1000

static volatile uint64_t s_ticks = 0;

static void pit_tick_handler(Registers* regs) {
    (void)regs;
    s_ticks++;
    
    // Debug: log every 1000 ticks to monitor actual timer speed
    // if (s_ticks % 1000 == 0) {
    //     log_debug("Time", "PIT tick: %llu", s_ticks);
    // }
}

/* Inicializa el temporizador */
void time_init(void) {
    log_debug("Time", "Initializing PIT with frequency %d Hz", HZ);
    pit_init(HZ);
    
    log_debug("Time", "Registering PIT handler for IRQ 0");
    i686_IRQ_RegisterHandler(0, pit_tick_handler); /* IRQ0 = PIT */
    
    log_debug("Time", "Timer initialized with HZ=%d", HZ);
    log_debug("Time", "Timer initialization completed");
}

uint64_t time_get_ticks(void) {
    return s_ticks;
}

uint32_t sys_time(void) {
    /* Con HZ=1000, cada tick = 1ms, así que ticks == milisegundos */
    return (uint32_t)s_ticks;
}
