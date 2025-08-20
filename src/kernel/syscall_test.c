#include "syscall.h"
#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <stdbool.h>

// Tests para las syscalls
void syscall_test_basic(void) {
    printf("=== Testing Basic System Calls ===\n\n");
    
    // Test SYSCALL_PRINT
    printf("1. Testing sys_print():\n");
    int result = sys_print("Hello from syscall!\n");
    printf("   Result: %d\n\n", result);
    
    // Test SYSCALL_GETPID
    printf("2. Testing sys_getpid():\n");
    int32_t pid = sys_getpid();
    printf("   PID: %d\n\n", pid);
    
    // Test SYSCALL_TIME
    printf("3. Testing sys_time():\n");
    uint32_t time1 = sys_time();
    uint32_t time2 = sys_time();
    printf("   Time1: %u, Time2: %u\n\n", time1, time2);
}

void syscall_test_memory(void) {
    printf("=== Testing Memory Management ===\n\n");
    
    // Test SYSCALL_MALLOC
    printf("1. Testing sys_malloc():\n");
    void* ptr1 = sys_malloc(256);
    void* ptr2 = sys_malloc(512);
    void* ptr3 = sys_malloc(128);
    
    printf("   Allocated ptr1: 0x%08X (256 bytes)\n", (uint32_t)ptr1);
    printf("   Allocated ptr2: 0x%08X (512 bytes)\n", (uint32_t)ptr2);
    printf("   Allocated ptr3: 0x%08X (128 bytes)\n", (uint32_t)ptr3);
    
    if (ptr1 && ptr2 && ptr3) {
        // Escribir datos de prueba
        char* data1 = (char*)ptr1;
        char* data2 = (char*)ptr2;
        char* data3 = (char*)ptr3;
        
        strcpy(data1, "Test data 1");
        strcpy(data2, "This is a longer test string for the 512-byte buffer");
        strcpy(data3, "Short test");
        
        printf("   Data1: %s\n", data1);
        printf("   Data2: %s\n", data2);
        printf("   Data3: %s\n", data3);
        
        // Test SYSCALL_FREE
        printf("\n2. Testing sys_free():\n");
        int free_result1 = sys_free(ptr1);
        int free_result2 = sys_free(ptr2);
        int free_result3 = sys_free(ptr3);
        
        printf("   Free ptr1 result: %d\n", free_result1);
        printf("   Free ptr2 result: %d\n", free_result2);
        printf("   Free ptr3 result: %d\n", free_result3);
    } else {
        printf("   ERROR: Memory allocation failed!\n");
    }
    
    printf("\n");
}

void syscall_test_files(void) {
    printf("=== Testing File System ===\n\n");
    
    // Test SYSCALL_OPEN (archivo existente)
    printf("1. Testing sys_open() on existing file:\n");
    int32_t fd1 = sys_open("welcome.txt", OPEN_READ);
    printf("   File descriptor for 'welcome.txt': %d\n", fd1);
    
    if (fd1 >= 0) {
        // Test SYSCALL_READ
        printf("\n2. Testing sys_read():\n");
        char buffer[256];
        memset(buffer, 0, sizeof(buffer));
        int32_t bytes_read = sys_read(fd1, buffer, sizeof(buffer) - 1);
        printf("   Bytes read: %d\n", bytes_read);
        printf("   Content: %s\n", buffer);
        
        // Test SYSCALL_CLOSE
        printf("\n3. Testing sys_close():\n");
        int32_t close_result = sys_close(fd1);
        printf("   Close result: %d\n", close_result);
    }
    
    // Test crear nuevo archivo
    printf("\n4. Testing file creation:\n");
    int32_t fd2 = sys_open("test_file.txt", OPEN_CREATE | OPEN_WRITE);
    printf("   Created file descriptor: %d\n", fd2);
    
    if (fd2 >= 0) {
        // Test SYSCALL_WRITE
        printf("\n5. Testing sys_write():\n");
        const char* test_data = "This is test data written via syscall!\nLine 2 of test data.\n";
        int32_t bytes_written = sys_write(fd2, test_data, strlen(test_data));
        printf("   Bytes written: %d\n", bytes_written);
        
        sys_close(fd2);
        
        // Leer el archivo que acabamos de crear
        printf("\n6. Testing read back created file:\n");
        int32_t fd3 = sys_open("test_file.txt", OPEN_READ);
        if (fd3 >= 0) {
            char read_buffer[256];
            memset(read_buffer, 0, sizeof(read_buffer));
            int32_t read_bytes = sys_read(fd3, read_buffer, sizeof(read_buffer) - 1);
            printf("   Read back %d bytes:\n%s", read_bytes, read_buffer);
            sys_close(fd3);
        }
    }
    
    printf("\n");
}

void syscall_test_advanced(void) {
    printf("=== Testing Advanced Features ===\n\n");
    
    // Test múltiples allocaciones y fragmentación
    printf("1. Testing memory fragmentation:\n");
    void* ptrs[10];
    
    // Alocar múltiples bloques
    for (int i = 0; i < 10; i++) {
        ptrs[i] = sys_malloc(64 + i * 32);
        printf("   Allocated block %d: 0x%08X (%d bytes)\n", 
               i, (uint32_t)ptrs[i], 64 + i * 32);
    }
    
    // Liberar algunos bloques (crear fragmentación)
    printf("\n2. Creating fragmentation by freeing alternate blocks:\n");
    for (int i = 1; i < 10; i += 2) {
        sys_free(ptrs[i]);
        printf("   Freed block %d\n", i);
        ptrs[i] = NULL;
    }
    
    // Intentar alocar un bloque grande
    printf("\n3. Trying to allocate large block after fragmentation:\n");
    void* large_ptr = sys_malloc(1024);
    printf("   Large allocation result: 0x%08X\n", (uint32_t)large_ptr);
    
    // Limpiar memoria restante
    for (int i = 0; i < 10; i++) {
        if (ptrs[i]) sys_free(ptrs[i]);
    }
    if (large_ptr) sys_free(large_ptr);
    
    // Test de sleep
    printf("\n4. Testing sys_sleep():\n");
    printf("   Sleeping for 100ms...\n");
    sys_sleep(100);
    printf("   Sleep completed\n");
    
    // Test de yield
    printf("\n5. Testing sys_yield():\n");
    int32_t yield_result = sys_yield();
    printf("   Yield result: %d\n", yield_result);
    
    printf("\n");
}

void syscall_stress_test(void) {
    printf("=== Stress Testing ===\n\n");
    
    printf("1. Memory allocation stress test:\n");
    int success_count = 0;
    int fail_count = 0;
    
    for (int i = 0; i < 50; i++) {
        void* ptr = sys_malloc(128);
        if (ptr) {
            success_count++;
            // Escribir datos de prueba
            memset(ptr, 0xAA, 128);
            sys_free(ptr);
        } else {
            fail_count++;
        }
    }
    
    printf("   Successful allocations: %d\n", success_count);
    printf("   Failed allocations: %d\n", fail_count);
    
    printf("\n2. File system stress test:\n");
    char filename[32];
    int file_success = 0;
    int file_fail = 0;
    
    // Crear múltiples archivos
    for (int i = 0; i < 20; i++) {
        snprintf(filename, sizeof(filename), "stress_test_%d.txt", i);
        int32_t fd = sys_open(filename, OPEN_CREATE | OPEN_WRITE);
        if (fd >= 0) {
            file_success++;
            char data[64];
            snprintf(data, sizeof(data), "Stress test file number %d\n", i);
            sys_write(fd, data, strlen(data));
            sys_close(fd);
        } else {
            file_fail++;
        }
    }
    
    printf("   Successful file operations: %d\n", file_success);
    printf("   Failed file operations: %d\n", file_fail);
    
    printf("\n");
}

void syscall_run_all_tests(void) {
    printf("====================================\n");
    printf("    SYSCALL COMPREHENSIVE TEST\n");
    printf("====================================\n\n");
    
    syscall_test_basic();
    syscall_test_memory();
    syscall_test_files();
    syscall_test_advanced();
    syscall_stress_test();
    
    printf("====================================\n");
    printf("         ALL TESTS COMPLETED\n");
    printf("====================================\n");
}

// Función de prueba específica para malloc/free
void syscall_test_heap_integrity(void) {
    printf("=== Heap Integrity Test ===\n\n");
    
    void* test_ptrs[20];
    
    // Fase 1: Allocar bloques de diferentes tamaños
    printf("Phase 1: Allocating various sized blocks\n");
    for (int i = 0; i < 20; i++) {
        int size = 32 + (i * 16); // Tamaños de 32, 48, 64, 80, etc.
        test_ptrs[i] = sys_malloc(size);
        if (test_ptrs[i]) {
            // Escribir patrón de prueba
            memset(test_ptrs[i], 0x55 + i, size);
            printf("  Block %d: 0x%08X (%d bytes)\n", i, (uint32_t)test_ptrs[i], size);
        } else {
            printf("  Block %d: ALLOCATION FAILED\n", i);
        }
    }
    
    // Fase 2: Verificar integridad de datos
    printf("\nPhase 2: Verifying data integrity\n");
    bool integrity_ok = true;
    for (int i = 0; i < 20; i++) {
        if (test_ptrs[i]) {
            uint8_t* data = (uint8_t*)test_ptrs[i];
            uint8_t expected = 0x55 + i;
            int size = 32 + (i * 16);
            
            bool block_ok = true;
            for (int j = 0; j < size; j++) {
                if (data[j] != expected) {
                    block_ok = false;
                    break;
                }
            }
            
            printf("  Block %d integrity: %s\n", i, block_ok ? "OK" : "CORRUPTED");
            if (!block_ok) integrity_ok = false;
        }
    }
    
    // Fase 3: Liberar bloques alternos
    printf("\nPhase 3: Freeing alternate blocks\n");
    for (int i = 1; i < 20; i += 2) {
        if (test_ptrs[i]) {
            sys_free(test_ptrs[i]);
            test_ptrs[i] = NULL;
            printf("  Freed block %d\n", i);
        }
    }
    
    // Fase 4: Verificar que los bloques restantes siguen íntegros
    printf("\nPhase 4: Verifying remaining blocks\n");
    for (int i = 0; i < 20; i += 2) {
        if (test_ptrs[i]) {
            uint8_t* data = (uint8_t*)test_ptrs[i];
            uint8_t expected = 0x55 + i;
            int size = 32 + (i * 16);
            
            bool block_ok = true;
            for (int j = 0; j < size; j++) {
                if (data[j] != expected) {
                    block_ok = false;
                    break;
                }
            }
            
            printf("  Block %d integrity: %s\n", i, block_ok ? "OK" : "CORRUPTED");
            if (!block_ok) integrity_ok = false;
        }
    }
    
    // Fase 5: Limpiar memoria restante
    printf("\nPhase 5: Cleaning up remaining blocks\n");
    for (int i = 0; i < 20; i += 2) {
        if (test_ptrs[i]) {
            sys_free(test_ptrs[i]);
            printf("  Freed block %d\n", i);
        }
    }
    
    printf("\nHeap integrity test: %s\n", integrity_ok ? "PASSED" : "FAILED");
    printf("\n");
}