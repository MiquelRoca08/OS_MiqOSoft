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

// ============================================================================
// SISTEMA DMESG ROBUSTO CON INICIALIZACIÓN SEGURA
// ============================================================================

#define DMESG_MAX_ENTRIES 32
#define DMESG_MAX_MSG_LEN 60
#define DMESG_MAX_COMP_LEN 10

typedef struct {
    uint32_t step;       // Número de paso del boot
    char level;          // Nivel del mensaje
    char component[DMESG_MAX_COMP_LEN];
    char message[DMESG_MAX_MSG_LEN];
} boot_message_t;

static boot_message_t dmesg_entries[DMESG_MAX_ENTRIES];
static int dmesg_entry_count = 0;
static uint32_t boot_sequence = 0;

// Función para limpiar memoria de forma segura
void safe_memset(void* ptr, int value, int size) {
    char* p = (char*)ptr;
    for (int i = 0; i < size; i++) {
        p[i] = (char)value;
    }
}

// Función para copiar strings de forma segura
void safe_string_copy(char* dest, const char* src, int max_len) {
    int i = 0;
    
    // Asegurar que dest esté limpio
    for (int j = 0; j < max_len; j++) {
        dest[j] = '\0';
    }
    
    // Copiar solo si src es válido
    if (src) {
        while (i < max_len - 1 && src[i] != '\0' && src[i] >= 32 && src[i] <= 126) {
            dest[i] = src[i];
            i++;
        }
    }
    dest[i] = '\0';
}

// Función segura para agregar mensajes al log del kernel
void kernel_add_message(char level, const char* component, const char* message) {
    if (dmesg_entry_count >= DMESG_MAX_ENTRIES) {
        return; // Buffer lleno
    }
    
    boot_message_t* entry = &dmesg_entries[dmesg_entry_count];
    
    // Limpiar la entrada completamente
    safe_memset(entry, 0, sizeof(boot_message_t));
    
    entry->step = boot_sequence++;
    entry->level = level;
    
    // Copiar strings de forma ultra-segura
    safe_string_copy(entry->component, component, DMESG_MAX_COMP_LEN);
    safe_string_copy(entry->message, message, DMESG_MAX_MSG_LEN);
    
    dmesg_entry_count++;
}

// Función para mostrar los mensajes del kernel
void display_kernel_messages(void) {
    if (dmesg_entry_count == 0) {
        printf("No kernel boot messages recorded.\n");
        return;
    }
    
    printf("Kernel boot messages (%d entries):\n", dmesg_entry_count);
    printf("Step Level Component  Message\n");
    printf("---- ----- --------- -------\n");
    
    for (int i = 0; i < dmesg_entry_count; i++) {
        boot_message_t* entry = &dmesg_entries[i];
        
        // Validar que los datos son seguros para imprimir
        if (entry->level >= 32 && entry->level <= 126) {
            printf("%04u   %c   %-9s %s\n", 
                   entry->step, 
                   entry->level, 
                   entry->component, 
                   entry->message);
        }
    }
}

// ============================================================================
// FUNCIÓN PRINCIPAL DEL KERNEL
// ============================================================================

void start(BootParams* bootParams)
{
    // call global constructors
    _init();

    // PASO 1: Inicializar completamente el sistema dmesg
    dmesg_entry_count = 0;
    boot_sequence = 0;
    safe_memset(dmesg_entries, 0, sizeof(dmesg_entries));
    
    kernel_add_message('I', "boot", "MiqOSoft v0.18.5 kernel starting");

    // PASO 2: HAL
    kernel_add_message('I', "hal", "Hardware layer initializing");
    HAL_Initialize();
    kernel_add_message('I', "hal", "Hardware layer ready");
    
    log_debug("Main", "Initializing timer system...");
    kernel_add_message('I', "timer", "Timer subsystem ready");
    // time_init();  // Removed duplicate - already called by HAL_Initialize()
    
    // PASO 3: System calls
    log_debug("Main", "Initializing syscall system...");
    kernel_add_message('I', "syscall", "System calls initializing");
    syscall_initialize();
    kernel_add_message('I', "syscall", "27 syscalls registered");

    // PASO 4: Keyboard
    kernel_add_message('I', "keyboard", "PS/2 keyboard initializing");
    keyboard_init();
    kernel_add_message('D', "keyboard", "Keyboard driver loaded");
    
    i686_IRQ_RegisterHandler(1, keyboard_handler);  // IRQ 1 = Keyboard
    kernel_add_message('D', "irq", "Keyboard IRQ handler set");
    
    // PASO 5: Interrupciones
    i8259_Unmask(0);  // IRQ 0 = PIT
    kernel_add_message('D', "pic", "Timer interrupt unmasked");
    
    i8259_Unmask(1);  // IRQ 1 = Keyboard
    kernel_add_message('D', "pic", "Keyboard interrupt unmasked");
    
    kernel_add_message('I', "cpu", "Enabling interrupts globally");
    i686_EnableInterrupts();
    kernel_add_message('I', "cpu", "System ready for interrupts");
    
    // PASO 6: Información de boot
    kernel_add_message('D', "boot", "Processing boot parameters");
    log_debug("Main", "Boot device: %x", bootParams->BootDevice);
    
    kernel_add_message('I', "memory", "Scanning memory regions");
    log_debug("Main", "Memory region count: %d", bootParams->Memory.RegionCount);
    for (int i = 0; i < bootParams->Memory.RegionCount; i++) 
    {
        log_debug("Main", "MEM: start=0x%llx length=0x%llx type=%x", 
            bootParams->Memory.Regions[i].Begin,
            bootParams->Memory.Regions[i].Length,
            bootParams->Memory.Regions[i].Type);
    }
    kernel_add_message('I', "memory", "Memory map analysis complete");

    // PASO 7: Sistema de logging de demostración
    log_info("Main", "This is an info msg!");
    log_warn("Main", "This is a warning msg!");
    log_err("Main", "This is an error msg!");
    log_crit("Main", "This is a critical msg!");
    
    kernel_add_message('I', "demo", "System logging test complete");
    
    // PASO 8: Interfaz de usuario
    printf("OS MiqOSoft v0.18.5\n");
    printf("This operating system is under construction.\n");
    printf("\n");
    
    kernel_add_message('I', "ui", "Console interface ready");
    
    // PASO 9: Shell
    log_info("Main", "Entering main shell loop...");
    kernel_add_message('I', "shell", "Command shell starting");
    shell_init();
    kernel_add_message('I', "shell", "Shell ready for input");
    
    // PASO 10: Finalización
    kernel_add_message('I', "boot", "System initialization complete");
    
    // Bucle principal del sistema operativo
    while(1)
    {
        shell_run();
    }
}