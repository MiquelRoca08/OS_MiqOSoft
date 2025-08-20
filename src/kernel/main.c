#include <stdint.h>
#include "stdio.h"
#include "memory.h"
#include <hal.h>
#include <irq.h>
#include <io.h>
#include <debug.h>
#include "../libs/boot/bootparams.h"
#include <keyboard.h>
#include <mouse.h>
#include <vga_text.h>
#include "shell.h"

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
    mouse_init(); // Inicializar el ratón
    
    i686_IRQ_RegisterHandler(1, keyboard_handler);  // IRQ 1 = teclado
    i686_IRQ_RegisterHandler(12, mouse_handler);    // IRQ 12 = ratón
    i686_EnableInterrupts();
    
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
    
    printf("OS MiqOSoft v0.17.3\n");
    printf("This operating system is under construction.\n\n");
    
    // Mostrar información sobre el ratón
    if (mouse_has_wheel()) {
        printf("Mouse with wheel detected - scroll support enabled!\n\n");
    } else {
        printf("Standard mouse detected.\n\n");
    }

    // Inicializar la shell
    shell_init();

    // Loop principal - ahora manejado por la shell
    while (1) {
        shell_run();
    }

end:
    for (;;);
}