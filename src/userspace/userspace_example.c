#include "syscall.h"

// Ejemplo de programa de usuario que usa syscalls
// (En un OS real, esto se compilaría separadamente como binario de usuario)

void user_program_example(void) {
    // Mostrar información del proceso
    sys_print("=== User Program Example ===\n");
    
    int32_t pid = sys_getpid();
    sys_print("Process ID: ");
    // Nota: en un OS real tendríamos una función para convertir int a string
    
    uint32_t time = sys_time();
    sys_print("Current time: ");
    
    // Ejemplo de uso de memoria dinámica
    sys_print("\n--- Memory Management Test ---\n");
    
    void* buffer1 = sys_malloc(256);
    if (buffer1) {
        sys_print("Successfully allocated 256 bytes\n");
        
        // En un OS real, aquí usaríamos la memoria
        char* str_buffer = (char*)buffer1;
        // strcpy equivalente manual
        const char* msg = "Hello from user space!";
        char* dst = str_buffer;
        const char* src = msg;
        while (*src) {
            *dst++ = *src++;
        }
        *dst = '\0';
        
        sys_print("Data in buffer: ");
        sys_print(str_buffer);
        sys_print("\n");
        
        sys_free(buffer1);
        sys_print("Memory freed successfully\n");
    } else {
        sys_print("Memory allocation failed!\n");
    }
    
    // Ejemplo de uso de archivos
    sys_print("\n--- File System Test ---\n");
    
    // Crear un archivo
    int32_t fd = sys_open("user_test.txt", OPEN_CREATE | OPEN_WRITE);
    if (fd >= 0) {
        sys_print("Created file successfully\n");
        
        const char* file_content = "This file was created by a user program!\nUsing system calls for I/O operations.\n";
        int32_t written = sys_write(fd, file_content, 0); // En un OS real calcularíamos el length
        
        // Función manual para calcular longitud de string
        const char* p = file_content;
        int len = 0;
        while (*p++) len++;
        
        written = sys_write(fd, file_content, len);
        sys_print("Wrote data to file\n");
        
        sys_close(fd);
        sys_print("File closed\n");
        
        // Leer el archivo
        fd = sys_open("user_test.txt", OPEN_READ);
        if (fd >= 0) {
            char read_buffer[200];
            
            // Limpiar buffer manualmente
            for (int i = 0; i < 200; i++) {
                read_buffer[i] = 0;
            }
            
            int32_t bytes_read = sys_read(fd, read_buffer, 199);
            sys_print("Read file content:\n");
            sys_print(read_buffer);
            
            sys_close(fd);
        }
    } else {
        sys_print("Failed to create file\n");
    }
    
    // Ejemplo de control de proceso
    sys_print("\n--- Process Control Test ---\n");
    sys_print("Yielding CPU...\n");
    sys_yield();
    
    sys_print("Sleeping for a short time...\n");
    sys_sleep(500); // 500ms
    
    sys_print("User program completed successfully!\n");
    sys_exit(0);
}

// Función que simula un programa más complejo
void complex_user_program(void) {
    sys_print("=== Complex User Program ===\n");
    
    // Simular un programa que hace múltiples operaciones
    
    // 1. Múltiples allocaciones de memoria
    void* memory_blocks[10];
    int successful_allocs = 0;
    
    sys_print("Allocating multiple memory blocks...\n");
    for (int i = 0; i < 10; i++) {
        int size = 64 + (i * 32); // Tamaños variables
        memory_blocks[i] = sys_malloc(size);
        if (memory_blocks[i]) {
            successful_allocs++;
            
            // Escribir patrón de datos
            char* data = (char*)memory_blocks[i];
            for (int j = 0; j < size && j < 60; j++) {
                data[j] = 'A' + (j % 26);
            }
            if (size > 0) data[size-1] = '\0';
        }
    }
    
    sys_print("Allocated blocks successfully\n");
    
    // 2. Crear múltiples archivos
    sys_print("Creating multiple files...\n");
    for (int i = 0; i < 5; i++) {
        // Crear nombre de archivo manualmente
        char filename[20];
        filename[0] = 'f';
        filename[1] = 'i';
        filename[2] = 'l';
        filename[3] = 'e';
        filename[4] = '0' + i;
        filename[5] = '.';
        filename[6] = 't';
        filename[7] = 'x';
        filename[8] = 't';
        filename[9] = '\0';
        
        int32_t fd = sys_open(filename, OPEN_CREATE | OPEN_WRITE);
        if (fd >= 0) {
            char content[100];
            // Crear contenido manualmente
            content[0] = 'F';
            content[1] = 'i';
            content[2] = 'l';
            content[3] = 'e';
            content[4] = ' ';
            content[5] = 'n';
            content[6] = 'u';
            content[7] = 'm';
            content[8] = 'b';
            content[9] = 'e';
            content[10] = 'r';
            content[11] = ' ';
            content[12] = '0' + i;
            content[13] = '\n';
            content[14] = '\0';
            
            sys_write(fd, content, 14);
            sys_close(fd);
        }
    }
    
    // 3. Limpiar memoria
    sys_print("Cleaning up memory...\n");
    for (int i = 0; i < 10; i++) {
        if (memory_blocks[i]) {
            sys_free(memory_blocks[i]);
        }
    }
    
    // 4. Operaciones de tiempo
    uint32_t start_time = sys_time();
    sys_sleep(100);
    uint32_t end_time = sys_time();
    
    sys_print("Time operations completed\n");
    
    sys_print("Complex user program finished!\n");
    sys_exit(0);
}

// Función para demostrar manejo de errores
void error_handling_example(void) {
    sys_print("=== Error Handling Example ===\n");
    
    // Intentar abrir archivo inexistente
    sys_print("Trying to open non-existent file...\n");
    int32_t fd = sys_open("nonexistent.txt", OPEN_READ);
    if (fd < 0) {
        sys_print("Error: File not found (expected)\n");
    }
    
    // Intentar allocation muy grande
    sys_print("Trying to allocate very large memory block...\n");
    void* huge_ptr = sys_malloc(1000000); // 1MB
    if (!huge_ptr) {
        sys_print("Error: Out of memory (expected)\n");
    } else {
        sys_print("Unexpectedly succeeded - freeing memory\n");
        sys_free(huge_ptr);
    }
    
    // Intentar usar descriptor de archivo inválido
    sys_print("Trying to use invalid file descriptor...\n");
    char buffer[10];
    int32_t result = sys_read(999, buffer, 10);
    if (result < 0) {
        sys_print("Error: Invalid file descriptor (expected)\n");
    }
    
    sys_print("Error handling tests completed\n");
    sys_exit(0);
}

// Funciones que pueden ser llamadas desde la shell para demostrar
void run_user_program_example(void) {
    user_program_example();
}

void run_complex_user_program(void) {
    complex_user_program();
}

void run_error_handling_example(void) {
    error_handling_example();
}