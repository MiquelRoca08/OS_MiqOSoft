#pragma once

#include <stdint.h>

void time_init(void);
uint64_t time_get_ticks(void);
uint32_t sys_time(void);