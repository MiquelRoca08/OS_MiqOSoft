#include "time.h"
#include <stdint.h>
#include <io.h>
#include <debug.h>
#include <irq.h>
#include <isr.h>
#include <pit.h>
#include <stdio.h>
#include <io.h>

/* Frecuencia del PIT (Hz). 1000 → un tick = 1ms */
#define HZ 1000

static volatile uint64_t s_ticks = 0;

static void pit_tick_handler(Registers* regs) {
    (void)regs;
    s_ticks++;
}

/* Inicializa el temporizador */
void time_init(void) {
    pit_init(HZ);
    i686_IRQ_RegisterHandler(0, pit_tick_handler); /* IRQ0 = PIT */
    log_info("Time", "Timer initialized with HZ=%d", HZ);
}

uint64_t time_get_ticks(void) {
    return s_ticks;
}

uint32_t sys_time(void) {
    /* ticks → milisegundos */
    return (uint32_t)(s_ticks * (1000 / HZ));
}
