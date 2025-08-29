#include <stdint.h>
#include <stdio.h>
#include <memory.h>
#include <hal.h>
#include <irq.h>
#include <io.h>
#include <debug.h>
#include <boot/bootparams.h>
#include <keyboard.h>
#include <vga_text.h>
#include <shell.h>
#include <syscall.h>
#include <shell_commands.h>
#include <time.h>

extern void _init();

void start(BootParams* bootParams)
{
    // call global constructors
    _init();

    HAL_Initialize();
    
    log_debug("Main", "Initializing timer system...");
    // time_init();  // Removed duplicate - already called by HAL_Initialize()
    
    log_debug("Main", "Initializing syscall system...");
    syscall_initialize();

    keyboard_init();
    i686_IRQ_RegisterHandler(1, keyboard_handler);  // IRQ 1 = Keyboard
    
    i8259_Unmask(0);  // IRQ 0 = PIT
    
    i8259_Unmask(1);  // IRQ 1 = Keyboard
    
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
    
    printf("OS MiqOSoft v0.18.5\n");
    printf("This operating system is under construction.\n");
    printf("\n");
    
    // Inicializar y ejecutar la shell
    log_info("Main", "Entering main shell loop...");
    shell_init();
    
    // Bucle principal del sistema operativo
    while(1)
    {
        shell_run();
    }
}
