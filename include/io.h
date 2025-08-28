#pragma once
#include <stdint.h>

void __attribute__((cdecl)) i686_outb(uint16_t port, uint8_t value);
uint8_t __attribute__((cdecl)) i686_inb(uint16_t port);
uint8_t __attribute__((cdecl)) i686_EnableInterrupts();
uint8_t __attribute__((cdecl)) i686_DisableInterrupts();

void outb(uint16_t port, uint8_t val);
uint8_t inb(uint16_t port);

void i686_iowait();
void __attribute__((cdecl)) i686_Panic();