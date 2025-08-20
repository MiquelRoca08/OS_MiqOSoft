#include "shell.h"
#include "syscall.h"
#include <stdio.h>
#include <memory.h>
#include <string.h>

// Función externa para las pruebas completas
extern void syscall_run_all_tests(void);
extern void syscall_test_heap_integrity(void);

// ============================================================================
// COMANDOS DE SYSCALL PARA LA SHELL
// ============================================================================

int cmd_syscall_test(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: syscall_test <test_type>\n");
        printf("Available tests:\n");
        printf("  all       - Run all syscall tests\n");
        printf("  basic     - Test basic syscalls (print, getpid, time)\n");
        printf("  memory    - Test malloc/free\n");
        printf("  files     - Test file operations\n");
        printf("  heap      - Test heap integrity\n");
        printf("  stress    - Stress test\n");
        return 1;
    }
    
    if (shell_strcmp(argv[1], "all") == 0) {
        syscall_run_all_tests();
    } else if (shell_strcmp(argv[1], "basic") == 0) {
        printf("=== Basic Syscall Test ===\n\n");
        
        // Test sys_print
        printf("Testing sys_print: ");
        int result = sys_print("Hello from syscall!\n");
        printf("Result: %d\n\n", result);
        
        // Test sys_getpid
        int32_t pid = sys_getpid();
        printf("Current PID: %d\n\n", pid);
        
        // Test sys_time
        uint32_t time = sys_time();
        printf("Current time: %u\n\n", time);
        
    } else if (shell_strcmp(argv[1], "memory") == 0) {
        printf("=== Memory Syscall Test ===\n\n");
        
        // Probar malloc
        printf("Testing malloc:\n");
        void* ptr1 = sys_malloc(256);
        void* ptr2 = sys_malloc(512);
        printf("  Allocated 256 bytes at: 0x%08X\n", (uint32_t)ptr1);
        printf("  Allocated 512 bytes at: 0x%08X\n", (uint32_t)ptr2);
        
        if (ptr1 && ptr2) {
            // Escribir datos de prueba
            strcpy((char*)ptr1, "Test data 1");
            strcpy((char*)ptr2, "Test data 2 - longer string");
            
            printf("  Data in ptr1: %s\n", (char*)ptr1);
            printf("  Data in ptr2: %s\n", (char*)ptr2);
            
            // Liberar memoria
            printf("\nTesting free:\n");
            int free1 = sys_free(ptr1);
            int free2 = sys_free(ptr2);
            printf("  Free ptr1 result: %d\n", free1);
            printf("  Free ptr2 result: %d\n", free2);
        }
        
    } else if (shell_strcmp(argv[1], "files") == 0) {
        printf("=== File Syscall Test ===\n\n");
        
        // Abrir archivo existente
        printf("Opening existing file 'welcome.txt':\n");
        int32_t fd = sys_open("welcome.txt", OPEN_READ);
        printf("  File descriptor: %d\n", fd);
        
        if (fd >= 0) {
            char buffer[200];
            memset(buffer, 0, sizeof(buffer));
            int32_t bytes = sys_read(fd, buffer, sizeof(buffer) - 1);
            printf("  Read %d bytes:\n%s\n", bytes, buffer);
            sys_close(fd);
        }
        
        // Crear nuevo archivo
        printf("Creating new file 'shell_test.txt':\n");
        fd = sys_open("shell_test.txt", OPEN_CREATE | OPEN_WRITE);
        printf("  File descriptor: %d\n", fd);
        
        if (fd >= 0) {
            const char* data = "This file was created from the shell!\nUsing syscalls for file operations.\n";
            int32_t written = sys_write(fd, data, strlen(data));
            printf("  Wrote %d bytes\n", written);
            sys_close(fd);
            
            // Leer el archivo creado
            fd = sys_open("shell_test.txt", OPEN_READ);
            if (fd >= 0) {
                char buffer[200];
                memset(buffer, 0, sizeof(buffer));
                int32_t bytes = sys_read(fd, buffer, sizeof(buffer) - 1);
                printf("  Reading back file (%d bytes):\n%s\n", bytes, buffer);
                sys_close(fd);
            }
        }
        
    } else if (shell_strcmp(argv[1], "heap") == 0) {
        syscall_test_heap_integrity();
        
    } else if (shell_strcmp(argv[1], "stress") == 0) {
        printf("=== Stress Test ===\n\n");
        
        printf("Memory stress test (100 allocations):\n");
        int success = 0, failed = 0;
        for (int i = 0; i < 100; i++) {
            void* ptr = sys_malloc(64 + (i % 200));
            if (ptr) {
                success++;
                sys_free(ptr);
            } else {
                failed++;
            }
        }
        printf("  Success: %d, Failed: %d\n\n", success, failed);
        
        printf("File stress test (creating 10 files):\n");
        char filename[32];
        success = 0;
        failed = 0;
        for (int i = 0; i < 10; i++) {
            snprintf(filename, sizeof(filename), "stress_%d.txt", i);
            int32_t fd = sys_open(filename, OPEN_CREATE | OPEN_WRITE);
            if (fd >= 0) {
                success++;
                char data[64];
                snprintf(data, sizeof(data), "Stress test file #%d\n", i);
                sys_write(fd, data, strlen(data));
                sys_close(fd);
            } else {
                failed++;
            }
        }
        printf("  Success: %d, Failed: %d\n", success, failed);
        
    } else {
        printf("Unknown test type: %s\n", argv[1]);
        return 1;
    }
    
    return 0;
}

int cmd_malloc_test(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: malloc_test <size_in_bytes>\n");
        printf("Example: malloc_test 1024\n");
        return 1;
    }
    
    // Convertir argumento a número
    int size = 0;
    char* num = argv[1];
    while (*num >= '0' && *num <= '9') {
        size = size * 10 + (*num - '0');
        num++;
    }
    
    if (size <= 0) {
        printf("Invalid size: %s\n", argv[1]);
        return 1;
    }
    
    printf("Allocating %d bytes...\n", size);
    void* ptr = sys_malloc(size);
    
    if (ptr) {
        printf("Success! Allocated at address: 0x%08X\n", (uint32_t)ptr);
        
        // Escribir datos de prueba
        char* data = (char*)ptr;
        for (int i = 0; i < size && i < 100; i++) {
            data[i] = 'A' + (i % 26);
        }
        data[size < 100 ? size - 1 : 99] = '\0';
        
        printf("Test data written: %.50s%s\n", data, size > 50 ? "..." : "");
        
        // Liberar memoria
        printf("Freeing memory...\n");
        int result = sys_free(ptr);
        printf("Free result: %d\n", result);
        
    } else {
        printf("Allocation failed - out of memory\n");
    }
    
    return 0;
}

int cmd_file_create(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: file_create <filename> [content]\n");
        printf("Example: file_create myfile.txt \"Hello World\"\n");
        return 1;
    }
    
    const char* filename = argv[1];
    const char* content = argc > 2 ? argv[2] : "Default file content\n";
    
    printf("Creating file: %s\n", filename);
    
    int32_t fd = sys_open(filename, OPEN_CREATE | OPEN_WRITE);
    if (fd < 0) {
        printf("Failed to create file: %s (error: %d)\n", filename, fd);
        return 1;
    }
    
    printf("File descriptor: %d\n", fd);
    
    int32_t bytes_written = sys_write(fd, content, strlen(content));
    printf("Wrote %d bytes\n", bytes_written);
    
    int32_t close_result = sys_close(fd);
    printf("Close result: %d\n", close_result);
    
    // Verificar leyendo el archivo
    printf("\nVerifying by reading back:\n");
    fd = sys_open(filename, OPEN_READ);
    if (fd >= 0) {
        char buffer[256];
        memset(buffer, 0, sizeof(buffer));
        int32_t bytes_read = sys_read(fd, buffer, sizeof(buffer) - 1);
        printf("Read %d bytes: %s\n", bytes_read, buffer);
        sys_close(fd);
    }
    
    return 0;
}

int cmd_file_read(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: file_read <filename>\n");
        printf("Example: file_read welcome.txt\n");
        return 1;
    }
    
    const char* filename = argv[1];
    
    printf("Reading file: %s\n", filename);
    
    int32_t fd = sys_open(filename, OPEN_READ);
    if (fd < 0) {
        printf("Failed to open file: %s (error: %d)\n", filename, fd);
        return 1;
    }
    
    printf("File descriptor: %d\n", fd);
    
    char buffer[512];
    memset(buffer, 0, sizeof(buffer));
    int32_t bytes_read = sys_read(fd, buffer, sizeof(buffer) - 1);
    
    printf("Read %d bytes:\n", bytes_read);
    printf("--- File Content ---\n");
    printf("%s", buffer);
    if (bytes_read == sizeof(buffer) - 1) {
        printf("... (file may be larger)\n");
    }
    printf("--- End Content ---\n");
    
    sys_close(fd);
    return 0;
}

int cmd_heap_info(int argc, char* argv[]) {
    printf("=== Heap Information ===\n\n");
    
    // Información básica del heap
    printf("Heap configuration:\n");
    printf("  Start address: 0x%08X\n", 0x400000);
    printf("  Size: %d KB (%d bytes)\n", 0x100000 / 1024, 0x100000);
    printf("  Block size: %d bytes\n", 32);
    
    // Hacer algunas allocaciones de prueba para mostrar fragmentación
    printf("\nTesting current heap state:\n");
    
    void* test_ptrs[5];
    int sizes[] = {64, 128, 256, 512, 1024};
    
    for (int i = 0; i < 5; i++) {
        test_ptrs[i] = sys_malloc(sizes[i]);
        if (test_ptrs[i]) {
            printf("  Allocated %d bytes at 0x%08X\n", sizes[i], (uint32_t)test_ptrs[i]);
        } else {
            printf("  Failed to allocate %d bytes\n", sizes[i]);
        }
    }
    
    // Liberar memoria de prueba
    printf("\nCleaning up test allocations:\n");
    for (int i = 0; i < 5; i++) {
        if (test_ptrs[i]) {
            sys_free(test_ptrs[i]);
            printf("  Freed block at 0x%08X\n", (uint32_t)test_ptrs[i]);
        }
    }
    
    return 0;
}

int cmd_syscall_info(int argc, char* argv[]) {
    printf("=== System Call Information ===\n\n");
    
    printf("Syscall Interface:\n");
    printf("  Interrupt vector: 0x80\n");
    printf("  Total syscalls: %d\n", SYSCALL_COUNT);
    printf("  Calling convention: EAX=syscall_num, EBX=arg1, ECX=arg2, EDX=arg3, ESI=arg4\n");
    
    printf("\nImplemented System Calls:\n");
    printf("  0  - exit(code)\n");
    printf("  1  - print(message)\n");
    printf("  2  - read(fd, buffer, count)\n");
    printf("  3  - malloc(size)\n");
    printf("  4  - free(ptr)\n");
    printf("  5  - open(path, flags)\n");
    printf("  6  - close(fd)\n");
    printf("  7  - write(fd, buffer, count)\n");
    printf("  8  - seek(fd, offset, whence)\n");
    printf("  9  - getpid()\n");
    printf("  10 - time()\n");
    printf("  11 - sleep(milliseconds)\n");
    printf("  12 - yield()\n");
    printf("  13-26 - Additional syscalls (placeholder)\n");
    
    printf("\nError Codes:\n");
    printf("  0  - SYSCALL_OK\n");
    printf("  -1 - SYSCALL_ERROR\n");
    printf("  -2 - SYSCALL_INVALID_SYSCALL\n");
    printf("  -3 - SYSCALL_INVALID_PARAMS\n");
    printf("  -4 - SYSCALL_PERMISSION_DENIED\n");
    printf("  -5 - SYSCALL_NOT_FOUND\n");
    printf("  -6 - SYSCALL_OUT_OF_MEMORY\n");
    
    printf("\nUsage examples:\n");
    printf("  syscall_test basic   - Test basic functionality\n");
    printf("  malloc_test 1024     - Test memory allocation\n");
    printf("  file_create test.txt - Create a test file\n");
    printf("  file_read test.txt   - Read a file\n");
    
    return 0;
}

int cmd_sleep_test(int argc, char* argv[]) {
    int milliseconds = 1000; // Default 1 second
    
    if (argc > 1) {
        // Convertir argumento a número
        int ms = 0;
        char* num = argv[1];
        while (*num >= '0' && *num <= '9') {
            ms = ms * 10 + (*num - '0');
            num++;
        }
        if (ms > 0) {
            milliseconds = ms;
        }
    }
    
    printf("Sleeping for %d milliseconds...\n", milliseconds);
    uint32_t start_time = sys_time();
    
    int result = sys_sleep(milliseconds);
    
    uint32_t end_time = sys_time();
    printf("Sleep completed (result: %d)\n", result);
    printf("Time difference: %u\n", end_time - start_time);
    
    return 0;
}