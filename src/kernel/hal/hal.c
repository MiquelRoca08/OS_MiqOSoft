#include "hal.h"
#include <gdt.h>
#include <idt.h>
#include <isr.h>
#include <irq.h>
#include <vga_text.h>

void HAL_Initialize()
{
    VGA_clrscr();
    i686_GDT_Initialize();
    i686_IDT_Initialize();
    i686_ISR_Initialize();
    i686_IRQ_Initialize();
}