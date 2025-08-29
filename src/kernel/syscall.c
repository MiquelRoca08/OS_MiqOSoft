#include <syscall.h>
#include <idt.h>
#include <isr.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <debug.h>
#include <vga_text.h>
#include <io.h>
#include <time.h>

// Tabla de handlers de syscalls
static syscall_handler_t syscall_handlers[SYSCALL_COUNT];

#define HEAP_START      0x400000    // 4MB
#define HEAP_SIZE       0x100000    // 1MB
#define BLOCK_SIZE      32          // Tamaño mínimo de bloque

typedef struct heap_block {
    uint32_t size;
    uint32_t is_free;
    struct heap_block* next;
} heap_block_t;

static heap_block_t* heap_start = NULL;
static uint32_t heap_initialized = 0;

// Inicializar el heap simple
static void heap_init(void) {
    if (heap_initialized) return;
    
    heap_start = (heap_block_t*)HEAP_START;
    heap_start->size = HEAP_SIZE - sizeof(heap_block_t);
    heap_start->is_free = 1;
    heap_start->next = NULL;
    heap_initialized = 1;
    
    log_info("Syscall", "Heap initialized at 0x%08X, size: %d bytes", HEAP_START, HEAP_SIZE);
}

// Sistema de archivos virtual simple (para las syscalls de archivos)
#define MAX_OPEN_FILES  32
#define MAX_FILENAME    64

typedef struct {
    char name[MAX_FILENAME];
    char* data;
    uint32_t size;
    uint32_t capacity;
    uint32_t is_directory;
    uint32_t in_use;
} vfile_t;

typedef struct {
    vfile_t* file;
    uint32_t position;
    uint32_t flags;
    uint32_t in_use;
} file_descriptor_t;

static vfile_t virtual_files[MAX_OPEN_FILES];
static file_descriptor_t file_descriptors[MAX_OPEN_FILES];
static uint32_t fs_initialized = 0;

// Inicializar el sistema de archivos virtual
static void vfs_init(void) {
    if (fs_initialized) return;
    
    memset(virtual_files, 0, sizeof(virtual_files));
    memset(file_descriptors, 0, sizeof(file_descriptors));
    
    // Crear algunos archivos por defecto
    strcpy(virtual_files[0].name, "welcome.txt");
    virtual_files[0].data = "Welcome to MiqOSoft!\nThis is a virtual file system.\n";
    virtual_files[0].size = strlen(virtual_files[0].data);
    virtual_files[0].capacity = virtual_files[0].size;
    virtual_files[0].is_directory = 0;
    virtual_files[0].in_use = 1;
    
    strcpy(virtual_files[1].name, "info.txt");
    virtual_files[1].data = "System calls are working!\nYou can use the syscall interface.\n";
    virtual_files[1].size = strlen(virtual_files[1].data);
    virtual_files[1].capacity = virtual_files[1].size;
    virtual_files[1].is_directory = 0;
    virtual_files[1].in_use = 1;
    
    fs_initialized = 1;
    log_info("Syscall", "Virtual file system initialized");
}

// ============================================================================
// IMPLEMENTACIONES DE SYSCALLS - CORREGIDAS
// ============================================================================

static int32_t sys_handler_exit(uint32_t code, uint32_t arg2, uint32_t arg3, uint32_t arg4) {
    log_info("Syscall", "Process exit with code: %d", code);
    printf("Process exited with code: %d\n", code);
    return SYSCALL_OK;
}

static int32_t sys_handler_print(uint32_t msg_ptr, uint32_t arg2, uint32_t arg3, uint32_t arg4) {
    const char* msg = (const char*)msg_ptr;
    if (!msg) return SYSCALL_INVALID_PARAMS;
    
    printf("%s", msg);
    return strlen(msg);
}

static int32_t sys_handler_read(uint32_t fd, uint32_t buffer_ptr, uint32_t count, uint32_t arg4) {
    if (fd >= MAX_OPEN_FILES || !file_descriptors[fd].in_use) {
        return SYSCALL_INVALID_PARAMS;
    }
    
    file_descriptor_t* desc = &file_descriptors[fd];
    vfile_t* file = desc->file;
    char* buffer = (char*)buffer_ptr;
    
    if (!buffer || !file) return SYSCALL_INVALID_PARAMS;
    
    uint32_t to_read = count;
    if (desc->position + to_read > file->size) {
        to_read = file->size - desc->position;
    }
    
    memcpy(buffer, file->data + desc->position, to_read);
    desc->position += to_read;
    
    return to_read;
}

static int32_t sys_handler_malloc(uint32_t size, uint32_t arg2, uint32_t arg3, uint32_t arg4) {
    if (!heap_initialized) heap_init();
    
    if (size == 0) return (int32_t)NULL;
    
    // Alinear al tamaño de bloque
    size = (size + BLOCK_SIZE - 1) & ~(BLOCK_SIZE - 1);
    
    heap_block_t* current = heap_start;
    while (current) {
        if (current->is_free && current->size >= size) {
            // Encontramos un bloque libre suficientemente grande
            current->is_free = 0;
            
            // Si el bloque es mucho más grande, dividirlo
            if (current->size > size + sizeof(heap_block_t) + BLOCK_SIZE) {
                heap_block_t* new_block = (heap_block_t*)((uint8_t*)current + sizeof(heap_block_t) + size);
                new_block->size = current->size - size - sizeof(heap_block_t);
                new_block->is_free = 1;
                new_block->next = current->next;
                current->next = new_block;
                current->size = size;
            }
            
            return (int32_t)((uint8_t*)current + sizeof(heap_block_t));
        }
        current = current->next;
    }
    
    return (int32_t)NULL; // No hay memoria disponible
}

static int32_t sys_handler_free(uint32_t ptr, uint32_t arg2, uint32_t arg3, uint32_t arg4) {
    if (!ptr || !heap_initialized) return SYSCALL_INVALID_PARAMS;
    
    heap_block_t* block = (heap_block_t*)((uint8_t*)ptr - sizeof(heap_block_t));
    block->is_free = 1;
    
    // Intentar fusionar con bloques adyacentes libres
    heap_block_t* current = heap_start;
    while (current && current->next) {
        if (current->is_free && current->next->is_free) {
            current->size += current->next->size + sizeof(heap_block_t);
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
    
    return SYSCALL_OK;
}

static int32_t sys_handler_open(uint32_t path_ptr, uint32_t flags, uint32_t arg3, uint32_t arg4) {
    const char* path = (const char*)path_ptr;
    if (!path) return SYSCALL_INVALID_PARAMS;
    
    vfs_init();
    
    // Buscar el archivo
    vfile_t* file = NULL;
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (virtual_files[i].in_use && strcmp(virtual_files[i].name, path) == 0) {
            file = &virtual_files[i];
            break;
        }
    }
    
    // Si no existe y tenemos flag CREATE, crearlo
    if (!file && (flags & OPEN_CREATE)) {
        for (int i = 0; i < MAX_OPEN_FILES; i++) {
            if (!virtual_files[i].in_use) {
                file = &virtual_files[i];
                strcpy(file->name, path);
                file->data = (char*)sys_handler_malloc(256, 0, 0, 0); // Buffer inicial de 256 bytes
                if (!file->data) return SYSCALL_OUT_OF_MEMORY;
                file->size = 0;
                file->capacity = 256;
                file->is_directory = 0;
                file->in_use = 1;
                break;
            }
        }
    }
    
    if (!file) return SYSCALL_NOT_FOUND;
    
    // Buscar descriptor libre
    for (int i = 3; i < MAX_OPEN_FILES; i++) { // 0,1,2 reservados para stdin,stdout,stderr
        if (!file_descriptors[i].in_use) {
            file_descriptors[i].file = file;
            file_descriptors[i].position = (flags & OPEN_APPEND) ? file->size : 0;
            file_descriptors[i].flags = flags;
            file_descriptors[i].in_use = 1;
            
            if (flags & OPEN_TRUNCATE) {
                file->size = 0;
                file_descriptors[i].position = 0;
            }
            
            return i;
        }
    }
    
    return SYSCALL_OUT_OF_MEMORY;
}

static int32_t sys_handler_close(uint32_t fd, uint32_t arg2, uint32_t arg3, uint32_t arg4) {
    if (fd >= MAX_OPEN_FILES || !file_descriptors[fd].in_use) {
        return SYSCALL_INVALID_PARAMS;
    }
    
    file_descriptors[fd].in_use = 0;
    file_descriptors[fd].file = NULL;
    file_descriptors[fd].position = 0;
    file_descriptors[fd].flags = 0;
    
    return SYSCALL_OK;
}

static int32_t sys_handler_write(uint32_t fd, uint32_t buffer_ptr, uint32_t count, uint32_t arg4) {
    if (fd >= MAX_OPEN_FILES || !buffer_ptr || count == 0) {
        return SYSCALL_INVALID_PARAMS;
    }
    
    const char* buffer = (const char*)buffer_ptr;
    
    // Casos especiales para stdout/stderr
    if (fd == 1 || fd == 2) {
        for (uint32_t i = 0; i < count; i++) {
            putc(buffer[i]);
        }
        return count;
    }
    
    if (!file_descriptors[fd].in_use) {
        return SYSCALL_INVALID_PARAMS;
    }
    
    file_descriptor_t* desc = &file_descriptors[fd];
    vfile_t* file = desc->file;
    
    if (!file || !(desc->flags & OPEN_WRITE)) {
        return SYSCALL_PERMISSION_DENIED;
    }
    
    // Verificar si necesitamos expandir el buffer
    uint32_t needed_size = desc->position + count;
    if (needed_size > file->capacity) {
        uint32_t new_capacity = (needed_size + 255) & ~255; // Redondear a múltiplo de 256
        char* new_data = (char*)sys_handler_malloc(new_capacity, 0, 0, 0);
        if (!new_data) return SYSCALL_OUT_OF_MEMORY;
        
        memcpy(new_data, file->data, file->size);
        sys_handler_free((uint32_t)file->data, 0, 0, 0);
        file->data = new_data;
        file->capacity = new_capacity;
    }
    
    memcpy(file->data + desc->position, buffer, count);
    desc->position += count;
    if (desc->position > file->size) {
        file->size = desc->position;
    }
    
    return count;
}

static int32_t sys_handler_getpid(uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4) {
    return 1;
}

// CORREGIDA: Función TIME simplificada que NO llama a sys_time()
static int32_t sys_handler_time(uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4) {
    // Devolver directamente los ticks convertidos a milisegundos
    uint64_t ticks = time_get_ticks();
    return (int32_t)(ticks); // Devolver simplemente los ticks como tiempo
}

// CORREGIDA: Función SLEEP simplificada sin bucle infinito
static int32_t sys_handler_sleep(uint32_t ms, uint32_t arg2, uint32_t arg3, uint32_t arg4) {
    if (ms == 0) return SYSCALL_OK;
    
    log_info("Syscall", "Sleep called for %u ms", ms);
    
    // Obtener tiempo inicial
    uint64_t start_ticks = time_get_ticks();
    uint64_t target_ticks = start_ticks + ms; // Asumiendo que los ticks son en ms
    
    log_info("Syscall", "Start ticks: %llu, Target ticks: %llu", start_ticks, target_ticks);
    
    // Esperar activamente, pero permitir que se procesen interrupciones
    uint32_t iteration = 0;
    while (1) {
        uint64_t current_ticks = time_get_ticks();
        
        // Handle timer wrap-around correctly
        if (current_ticks >= target_ticks) {
            break;
        }
        
        // Simple delay para evitar uso excesivo de CPU
        // Mantener interrupciones habilitadas para que el timer funcione
        __asm__ volatile(
            "sti\n\t"    // Habilitar interrupciones
            "pause\n\t"  // Instrucción de pausa en x86
            ::: "memory"
        );
        
        // Add a small delay to yield more CPU time for interrupts
        for (volatile int i = 0; i < 1000; i++) {}
        
        iteration++;
        if (iteration % 100000 == 0) {  // Reduced from 1M to 100K iterations
            log_debug("Syscall", "Sleep iteration %u, current ticks: %llu, target: %llu", 
                     iteration, current_ticks, target_ticks);
            
            // Safety check to prevent infinite loop - increase timeout for longer sleeps
            if (iteration > 100000000) {  // Increased from 50M to 100M iterations
                log_warn("Syscall", "Sleep timeout - breaking infinite loop");
                break;
            }
        }
    }
    
    uint64_t end_ticks = time_get_ticks();
    log_info("Syscall", "Sleep completed: start=%llu, end=%llu, diff=%llu", 
             start_ticks, end_ticks, end_ticks - start_ticks);
    
    return SYSCALL_OK;
}

static int32_t sys_handler_yield(uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4) {
    log_info("Syscall", "Yield requested");
    return SYSCALL_OK;
}

// ============================================================================
// MANEJADOR PRINCIPAL DE SYSCALLS
// ============================================================================

static void syscall_handler(Registers* regs) {
    uint32_t syscall_num = regs->eax;
    uint32_t arg1 = regs->ebx;
    uint32_t arg2 = regs->ecx;
    uint32_t arg3 = regs->edx;
    uint32_t arg4 = regs->esi;
    
    int32_t result = syscall_dispatch(syscall_num, arg1, arg2, arg3, arg4);
    
    // Retornar resultado en EAX
    regs->eax = result;
}

int32_t syscall_dispatch(uint32_t syscall_num, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4) {
    if (syscall_num >= SYSCALL_COUNT) {
        log_warn("Syscall", "Invalid syscall number: %d", syscall_num);
        return SYSCALL_INVALID_SYSCALL;
    }
    
    if (!syscall_handlers[syscall_num]) {
        log_warn("Syscall", "Unimplemented syscall: %d", syscall_num);
        return SYSCALL_INVALID_SYSCALL;
    }
    
    log_debug("Syscall", "Calling syscall %d with args: %x, %x, %x, %x", 
              syscall_num, arg1, arg2, arg3, arg4);
    
    return syscall_handlers[syscall_num](arg1, arg2, arg3, arg4);
}

void syscall_register_handler(syscall_number_t num, syscall_handler_t handler) {
    if (num < SYSCALL_COUNT) {
        syscall_handlers[num] = handler;
    }
}

int32_t syscall_invoke(syscall_number_t num, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4) {
    return syscall_dispatch(num, arg1, arg2, arg3, arg4);
}

void syscall_initialize(void) {
    log_info("Syscall", "Initializing system call interface...");
    
    // Limpiar tabla de handlers
    memset(syscall_handlers, 0, sizeof(syscall_handlers));
    
    // Registrar handlers básicos
    syscall_register_handler(SYSCALL_EXIT, sys_handler_exit);
    syscall_register_handler(SYSCALL_PRINT, sys_handler_print);
    syscall_register_handler(SYSCALL_READ, sys_handler_read);
    syscall_register_handler(SYSCALL_MALLOC, sys_handler_malloc);
    syscall_register_handler(SYSCALL_FREE, sys_handler_free);
    syscall_register_handler(SYSCALL_OPEN, sys_handler_open);
    syscall_register_handler(SYSCALL_CLOSE, sys_handler_close);
    syscall_register_handler(SYSCALL_WRITE, sys_handler_write);
    syscall_register_handler(SYSCALL_GETPID, sys_handler_getpid);
    syscall_register_handler(SYSCALL_TIME, sys_handler_time);
    syscall_register_handler(SYSCALL_SLEEP, sys_handler_sleep);
    syscall_register_handler(SYSCALL_YIELD, sys_handler_yield);
    
    // Instalar manejador de interrupción para syscalls
    i686_ISR_RegisterHandler(SYSCALL_INTERRUPT, syscall_handler);
    
    // Inicializar subsistemas
    heap_init();
    vfs_init();
    
    log_info("Syscall", "System call interface initialized successfully");
    log_info("Syscall", "Registered %d system calls", SYSCALL_COUNT);
}
