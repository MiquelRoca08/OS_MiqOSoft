#include <stdint.h>
#include "stdio.h"
#include "memory.h"
#include <hal/hal.h>
#include <arch/i686/irq.h>
#include <arch/i686/io.h>
#include <debug.h>
#include <boot/bootparams.h>
#include "drivers/keyboard.h"
#include "arch/i686/vga_text.h"

extern void _init();

void crash_me();

void timer(Registers* regs)
{
    printf(".");
}

void start(BootParams* bootParams)
{   
    // call global constructors
    _init();

    HAL_Initialize();

    keyboard_init();
    screen_init();
    
    i686_IRQ_RegisterHandler(1, keyboard_handler); // IRQ 1 es para el teclado PS/2
    i686_EnableInterrupts(); // Habilitar interrupciones explícitamente
    
    log_debug("Main", "Boot device: %x", bootParams->BootDevice);
    log_debug("Main", "Memory region count: %d", bootParams->Memory.RegionCount);
    for (int i = 0; i < bootParams->Memory.RegionCount; i++) 
    {
        log_debug("Main", "MEM: start=0x%llx length=0x%llx type=%x", 
            bootParams->Memory.Regions[i].Begin,
            bootParams->Memory.Regions[i].Length,
            bootParams->Memory.Regions[i].Type);
    }


    log_info("Main", "This is an info msg!");
    log_warn("Main", "This is a warning msg!");
    log_err("Main", "This is an error msg!");
    log_crit("Main", "This is a critical msg!");
    printf("OS MiqOSoft v0.16.1\n");
    printf("This operating system is under construction.\n");

    g_ScreenY = 2;  // Línea 2 para input
    g_ScreenX = 0;
    redraw_input_line(); // limpia solo esa línea para la entrada

    while (1) {
        keyboard_process_buffer();
        // Aquí puedes añadir más cosas como scheduling, otras tareas, etc.
    }
    //i686_IRQ_RegisterHandler(0, timer);

    //crash_me();

end:
    for (;;);
}