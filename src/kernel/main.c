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
// SISTEMA DMESG MEJORADO - VERSION QUE FUNCIONA
// ============================================================================

#define SIMPLE_MSG_COUNT 20  // Incrementado de 16 a 20
#define SIMPLE_MSG_LEN 50    // Incrementado de 32 a 50 para mensajes más largos

// Estructura ultra-simple
typedef struct {
    char msg[SIMPLE_MSG_LEN];
    int valid;
} ultra_simple_msg_t;

static ultra_simple_msg_t ultra_messages[SIMPLE_MSG_COUNT];
static int ultra_msg_idx = 0;
static int ultra_initialized = 0;

// Función mejorada para crear mensajes más completos
void kernel_add_message(char level, const char* component, const char* message) {
    if (!ultra_initialized) {
        // Inicializar ultra-simple
        for (int i = 0; i < SIMPLE_MSG_COUNT; i++) {
            for (int j = 0; j < SIMPLE_MSG_LEN; j++) {
                ultra_messages[i].msg[j] = 0;
            }
            ultra_messages[i].valid = 0;
        }
        ultra_initialized = 1;
        ultra_msg_idx = 0;
    }
    
    if (ultra_msg_idx >= SIMPLE_MSG_COUNT) {
        return;  // Buffer lleno
    }
    
    ultra_simple_msg_t* entry = &ultra_messages[ultra_msg_idx];
    
    // Crear mensaje: "component: message"
    int pos = 0;
    
    // Limpiar mensaje
    for (int i = 0; i < SIMPLE_MSG_LEN; i++) {
        entry->msg[i] = 0;
    }
    
    // Copiar component (máximo 12 chars)
    if (component) {
        int i = 0;
        while (component[i] && i < 12 && pos < (SIMPLE_MSG_LEN - 3)) {
            char c = component[i];
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
                entry->msg[pos++] = c;
            }
            i++;
        }
        
        // Añadir separador
        if (pos < (SIMPLE_MSG_LEN - 2)) {
            entry->msg[pos++] = ':';
            entry->msg[pos++] = ' ';
        }
    }
    
    // Copiar message (resto del espacio disponible)
    if (message) {
        int i = 0;
        while (message[i] && pos < (SIMPLE_MSG_LEN - 1)) {
            char c = message[i];
            // Permitir más caracteres seguros
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || 
                (c >= '0' && c <= '9') || c == ' ' || c == '.' || 
                c == '-' || c == '_') {
                entry->msg[pos++] = c;
            } else if (c == '/' || c == '\\') {
                entry->msg[pos++] = '/';  // Normalizar slashes
            }
            i++;
        }
    }
    
    entry->msg[pos] = 0;
    entry->valid = 12345;  // Magic number
    
    ultra_msg_idx++;
}

void display_kernel_messages(void) {
    if (!ultra_initialized) {
        puts("dmesg not initialized");
        return;
    }
    
    if (ultra_msg_idx == 0) {
        puts("No kernel messages recorded");
        return;
    }
    
    puts("=== Kernel Messages ===");
    
    // Mostrar cada mensaje
    for (int i = 0; i < ultra_msg_idx && i < SIMPLE_MSG_COUNT; i++) {
        ultra_simple_msg_t* entry = &ultra_messages[i];
        
        if (entry->valid != 12345) {
            puts("Invalid entry");
            continue;
        }
        
        // Mostrar número de entrada con formato mejorado
        if (i < 10) {
            putc('0');
            putc('0' + i);
        } else {
            putc('1');
            putc('0' + (i - 10));
        }
        
        putc(':');
        putc(' ');
        
        // Mostrar mensaje completo
        for (int j = 0; j < SIMPLE_MSG_LEN && entry->msg[j] != 0; j++) {
            char c = entry->msg[j];
            // Filtro de caracteres más permisivo
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || 
                (c >= '0' && c <= '9') || c == ' ' || c == '.' || 
                c == ':' || c == '-' || c == '_' || c == '/') {
                putc(c);
            } else {
                putc('?');
            }
        }
        putc('\n');
    }
    
    // Mostrar resumen
    puts("=== End Messages (");
    if (ultra_msg_idx < 10) {
        putc('0' + ultra_msg_idx);
    } else {
        putc('1');
        putc('0' + (ultra_msg_idx - 10));
    }
    puts(" shown) ===");
}

void dmesg_clear(void) {
    // Limpiar todos los mensajes
    for (int i = 0; i < SIMPLE_MSG_COUNT; i++) {
        ultra_messages[i].valid = 0;
        for (int j = 0; j < SIMPLE_MSG_LEN; j++) {
            ultra_messages[i].msg[j] = 0;
        }
    }
    ultra_msg_idx = 0;
    puts("dmesg: Message buffer cleared");
}

void dmesg_stats(void) {
    if (!ultra_initialized) {
        puts("dmesg: System not initialized");
        return;
    }
    
    puts("dmesg Statistics:");
    
    puts("  Buffer size: ");
    if (SIMPLE_MSG_COUNT < 10) {
        putc('0' + SIMPLE_MSG_COUNT);
    } else {
        putc('2');
        putc('0');
    }
    puts(" entries");
    
    puts("  Used entries: ");
    if (ultra_msg_idx < 10) {
        putc('0' + ultra_msg_idx);
    } else {
        putc('1');
        putc('0' + (ultra_msg_idx - 10));
    }
    putc('\n');
    
    int free_entries = SIMPLE_MSG_COUNT - ultra_msg_idx;
    puts("  Free entries: ");
    if (free_entries < 10) {
        putc('0' + free_entries);
    } else {
        putc('1');
        putc('0' + (free_entries - 10));
    }
    putc('\n');
    
    // Verificar integridad
    int valid_count = 0;
    for (int i = 0; i < ultra_msg_idx; i++) {
        if (ultra_messages[i].valid == 12345) {
            valid_count++;
        }
    }
    
    puts("  Valid entries: ");
    if (valid_count < 10) {
        putc('0' + valid_count);
    } else {
        putc('1');
        putc('0' + (valid_count - 10));
    }
    putc('\n');
}

// ============================================================================
// FUNCIÓN PRINCIPAL DEL KERNEL
// ============================================================================

void start(BootParams* bootParams)
{
    // call global constructors
    _init();

    // PASO 1: Agregar primer mensaje
    kernel_add_message('I', "boot", "MiqOSoft kernel starting");

    // PASO 2: HAL
    kernel_add_message('I', "hal", "Hardware layer init");
    HAL_Initialize();
    kernel_add_message('I', "hal", "Hardware ready");
    
    log_debug("Main", "Initializing timer system...");
    kernel_add_message('I', "timer", "Timer ready");
    
    // PASO 3: System calls
    log_debug("Main", "Initializing syscall system...");
    kernel_add_message('I', "syscall", "System calls init");
    syscall_initialize();
    kernel_add_message('I', "syscall", "27 syscalls registered");

    // PASO 4: Keyboard
    kernel_add_message('I', "keyboard", "PS2 keyboard init");
    keyboard_init();
    kernel_add_message('D', "keyboard", "Keyboard driver ready");
    
    i686_IRQ_RegisterHandler(1, keyboard_handler);
    kernel_add_message('D', "irq", "Keyboard IRQ registered");
    
    // PASO 5: Interrupciones
    kernel_add_message('I', "pic", "Configuring interrupts");
    
    // Declarar función externa
    extern void i8259_Unmask(int irq);
    
    i8259_Unmask(0);
    kernel_add_message('D', "pic", "Timer IRQ unmasked");
    
    i8259_Unmask(1);
    kernel_add_message('D', "pic", "Keyboard IRQ unmasked");
    
    kernel_add_message('I', "cpu", "Enabling interrupts");
    i686_EnableInterrupts();
    kernel_add_message('I', "cpu", "Interrupts enabled");
    
    // PASO 6: Información de boot
    kernel_add_message('D', "boot", "Processing boot params");
    log_debug("Main", "Boot device: %x", bootParams->BootDevice);
    
    kernel_add_message('I', "memory", "Scanning memory");
    log_debug("Main", "Memory region count: %d", bootParams->Memory.RegionCount);
    for (int i = 0; i < bootParams->Memory.RegionCount; i++) 
    {
        log_debug("Main", "MEM: start=0x%llx length=0x%llx type=%x", 
            bootParams->Memory.Regions[i].Begin,
            bootParams->Memory.Regions[i].Length,
            bootParams->Memory.Regions[i].Type);
    }
    kernel_add_message('I', "memory", "Memory scan complete");

    // PASO 7: Sistema de logging
    log_info("Main", "This is an info msg!");
    log_warn("Main", "This is a warning msg!");
    log_err("Main", "This is an error msg!");
    log_crit("Main", "This is a critical msg!");
    
    kernel_add_message('I', "demo", "Logging test complete");
    
    // PASO 8: Interfaz de usuario
    printf("OS MiqOSoft v0.18.5\n");
    printf("This operating system is under construction.\n");
    printf("\n");
    
    kernel_add_message('I', "ui", "Console ready");
    
    // PASO 9: Shell
    log_info("Main", "Entering main shell loop...");
    kernel_add_message('I', "shell", "Shell starting");
    shell_init();
    kernel_add_message('I', "shell", "Shell ready");
    
    // PASO 10: Finalización
    kernel_add_message('I', "boot", "Init complete");
    
    // Bucle principal del sistema operativo
    while(1)
    {
        shell_run();
    }
}