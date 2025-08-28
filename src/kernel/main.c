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

extern void _init();

void start(BootParams* bootParams)
{
    // call global constructors
    _init();

    HAL_Initialize();
    
    // Inicializar sistema de syscalls ANTES de habilitar interrupciones
    syscall_initialize();

    keyboard_init();
    i686_IRQ_RegisterHandler(1, keyboard_handler);  // IRQ 1 = teclado
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

    printf("OS MiqOSoft v0.18.4\n");
    printf("This operating system is under construction.\n");
    printf("System calls are now available!\n\n");
    
    // Inicializar y ejecutar la shell para que el SO no termine.
    // Esto reemplaza las pruebas de syscalls para que el sistema avance.
    log_info("Main", "Entering main shell loop...");
    shell_init();
    
    // Bucle principal del sistema operativo.
    // Esto mantiene el programa en ejecución y permite al usuario interactuar con la shell.
    while(1)
    {
        shell_run();
    }
}
