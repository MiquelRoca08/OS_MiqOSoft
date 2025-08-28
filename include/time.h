#pragma once

#include <stdint.h>

/* Inicializa el subsistema de tiempo (PIT + IRQ handler) */
void time_init(void);

/* Devuelve ticks desde arranque */
uint64_t time_get_ticks(void);

/* Devuelve milisegundos desde arranque */
uint32_t sys_time(void);