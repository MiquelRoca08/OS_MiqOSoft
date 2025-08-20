#include "shell.h"
#include <shell_commands.h>
#include <stdio.h>
#include <io.h>
#include <vga_text.h>
#include <keyboard.h>
#include <memory.h>

// ============================================================================
// COMANDOS DE SISTEMA Y HARDWARE
// ============================================================================

int cmd_memtest(int argc, char* argv[]) {
    printf("Basic memory test...\n");
    
    // Test básico de memoria
    uint8_t* test_ptr = (uint8_t*)0x200000; // 2MB
    uint8_t patterns[] = {0x00, 0xFF, 0xAA, 0x55, 0xCC, 0x33};
    int pattern_count = sizeof(patterns);
    int test_size = 1024; // 1KB test
    
    for (int p = 0; p < pattern_count; p++) {
        printf("  Testing pattern 0x%02X...", patterns[p]);
        
        // Escribir patrón
        for (int i = 0; i < test_size; i++) {
            test_ptr[i] = patterns[p];
        }
        
        // Verificar patrón
        bool success = true;
        for (int i = 0; i < test_size; i++) {
            if (test_ptr[i] != patterns[p]) {
                success = false;
                break;
            }
        }
        
        printf(" %s\n", success ? "PASS" : "FAIL");
    }
    
    printf("Memory test completed.\n");
    return 0;
}

int cmd_ports(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: ports <read|write> [port] [value]\n");
        printf("  ports read 0x3f8    - Read from port 0x3f8\n");
        printf("  ports write 0x3f8 65 - Write 65 to port 0x3f8\n");
        return 1;
    }
    
    if (shell_strcmp(argv[1], "read") == 0) {
        if (argc < 3) {
            printf("Usage: ports read <port>\n");
            return 1;
        }
        
        // Convertir string hexadecimal a número
        uint16_t port = 0;
        char* hex = argv[2];
        if (hex[0] == '0' && hex[1] == 'x') hex += 2;
        
        while (*hex) {
            port <<= 4;
            if (*hex >= '0' && *hex <= '9') port += *hex - '0';
            else if (*hex >= 'a' && *hex <= 'f') port += *hex - 'a' + 10;
            else if (*hex >= 'A' && *hex <= 'F') port += *hex - 'A' + 10;
            hex++;
        }
        
        uint8_t value = i686_inb(port);
        printf("Port 0x%04X = 0x%02X (%d)\n", port, value, value);
        
    } else if (shell_strcmp(argv[1], "write") == 0) {
        if (argc < 4) {
            printf("Usage: ports write <port> <value>\n");
            return 1;
        }
        
        // Convertir port
        uint16_t port = 0;
        char* hex = argv[2];
        if (hex[0] == '0' && hex[1] == 'x') hex += 2;
        
        while (*hex) {
            port <<= 4;
            if (*hex >= '0' && *hex <= '9') port += *hex - '0';
            else if (*hex >= 'a' && *hex <= 'f') port += *hex - 'a' + 10;
            else if (*hex >= 'A' && *hex <= 'F') port += *hex - 'A' + 10;
            hex++;
        }
        
        // Convertir valor
        uint8_t value = 0;
        char* num = argv[3];
        while (*num >= '0' && *num <= '9') {
            value = value * 10 + (*num - '0');
            num++;
        }
        
        i686_outb(port, value);
        printf("Wrote 0x%02X to port 0x%04X\n", value, port);
        
    } else {
        printf("Invalid operation: %s\n", argv[1]);
        return 1;
    }
    
    return 0;
}

int cmd_interrupt(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: interrupt <enable|disable|status>\n");
        return 1;
    }
    
    if (shell_strcmp(argv[1], "enable") == 0) {
        i686_EnableInterrupts();
        printf("Interrupts enabled\n");
    } else if (shell_strcmp(argv[1], "disable") == 0) {
        i686_DisableInterrupts();
        printf("Interrupts disabled\n");
    } else if (shell_strcmp(argv[1], "status") == 0) {
        // Leer EFLAGS para verificar el bit de interrupciones
        uint32_t eflags;
        __asm__ volatile("pushfl; popl %0" : "=r"(eflags));
        bool interrupts_enabled = (eflags & 0x200) != 0;
        printf("Interrupts: %s\n", interrupts_enabled ? "ENABLED" : "DISABLED");
    } else {
        printf("Invalid option: %s\n", argv[1]);
        return 1;
    }
    
    return 0;
}

int cmd_hexdump(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Usage: hexdump <address> <length>\n");
        printf("  hexdump 0x100000 256  - Dump 256 bytes from 0x100000\n");
        return 1;
    }
    
    // Convertir dirección
    uint32_t addr = 0;
    char* hex = argv[1];
    if (hex[0] == '0' && hex[1] == 'x') hex += 2;
    
    while (*hex) {
        addr <<= 4;
        if (*hex >= '0' && *hex <= '9') addr += *hex - '0';
        else if (*hex >= 'a' && *hex <= 'f') addr += *hex - 'a' + 10;
        else if (*hex >= 'A' && *hex <= 'F') addr += *hex - 'A' + 10;
        hex++;
    }
    
    // Convertir longitud
    uint32_t len = 0;
    char* num = argv[2];
    while (*num >= '0' && *num <= '9') {
        len = len * 10 + (*num - '0');
        num++;
    }
    
    // Limitar longitud para evitar problemas
    if (len > 1024) {
        printf("Length limited to 1024 bytes\n");
        len = 1024;
    }
    
    uint8_t* ptr = (uint8_t*)addr;
    
    printf("Hex dump of 0x%08X (%d bytes):\n", addr, len);
    
    for (uint32_t i = 0; i < len; i += 16) {
        printf("%08X: ", addr + i);
        
        // Mostrar hex
        for (int j = 0; j < 16 && i + j < len; j++) {
            printf("%02X ", ptr[i + j]);
        }
        
        // Rellenar espacios si es necesario
        for (int j = len - i; j < 16; j++) {
            printf("   ");
        }
        
        printf(" |");
        
        // Mostrar ASCII
        for (int j = 0; j < 16 && i + j < len; j++) {
            char c = ptr[i + j];
            printf("%c", (c >= 32 && c <= 126) ? c : '.');
        }
        
        printf("|\n");
    }
    
    return 0;
}

int cmd_keytest(int argc, char* argv[]) {
    printf("Keyboard test mode - press keys to see scancodes\n");
    printf("Press ESC to exit\n\n");
    
    while (1) {
        keyboard_process_buffer();
        
        // Leer directamente del puerto del teclado para obtener scancodes
        if (i686_inb(0x64) & 0x01) { // Datos disponibles
            uint8_t scancode = i686_inb(0x60);
            
            if (scancode == 0x01) { // ESC
                printf("\nExiting keyboard test\n");
                break;
            }
            
            printf("Scancode: 0x%02X", scancode);
            if (scancode & 0x80) {
                printf(" (key released)\n");
            } else {
                printf(" (key pressed)\n");
            }
        }
    }
    
    return 0;
}

int cmd_benchmark(int argc, char* argv[]) {
    printf("Running basic CPU benchmark...\n");
    
    // Test de operaciones aritméticas
    printf("  Arithmetic test: ");
    volatile uint32_t result = 0;
    for (uint32_t i = 0; i < 1000000; i++) {
        result += i * 2 + 1;
    }
    printf("DONE (result: %u)\n", result);
    
    // Test de acceso a memoria
    printf("  Memory access test: ");
    volatile uint8_t* mem = (volatile uint8_t*)0x200000;
    for (uint32_t i = 0; i < 100000; i++) {
        mem[i % 1024] = (uint8_t)(i & 0xFF);
        result += mem[i % 1024];
    }
    printf("DONE\n");
    
    // Test de llamadas a función
    printf("  Function call test: ");
    for (uint32_t i = 0; i < 50000; i++) {
        VGA_get_cursor_x();
    }
    printf("DONE\n");
    
    printf("Benchmark completed\n");
    return 0;
}

// ============================================================================
// COMANDOS DE DEBUGGING Y DESARROLLO
// ============================================================================

int cmd_registers(int argc, char* argv[]) {
    uint32_t eax, ebx, ecx, edx, esp, ebp, esi, edi, eflags;
    
    // Obtener valores de registros
    __asm__ volatile(
        "movl %%eax, %0\n"
        "movl %%ebx, %1\n"
        "movl %%ecx, %2\n"
        "movl %%edx, %3\n"
        "movl %%esp, %4\n"
        "movl %%ebp, %5\n"
        "movl %%esi, %6\n"
        "movl %%edi, %7\n"
        "pushfl\n"
        "popl %8\n"
        : "=m"(eax), "=m"(ebx), "=m"(ecx), "=m"(edx),
          "=m"(esp), "=m"(ebp), "=m"(esi), "=m"(edi), "=m"(eflags)
        :
        : "memory"
    );
    
    printf("CPU Registers:\n");
    printf("  EAX: 0x%08X    EBX: 0x%08X\n", eax, ebx);
    printf("  ECX: 0x%08X    EDX: 0x%08X\n", ecx, edx);
    printf("  ESI: 0x%08X    EDI: 0x%08X\n", esi, edi);
    printf("  ESP: 0x%08X    EBP: 0x%08X\n", esp, ebp);
    printf("  EFLAGS: 0x%08X\n", eflags);
    
    printf("\nEFLAGS bits:\n");
    printf("  CF=%d PF=%d AF=%d ZF=%d SF=%d IF=%d DF=%d OF=%d\n",
           (eflags & 0x001) ? 1 : 0,  // Carry Flag
           (eflags & 0x004) ? 1 : 0,  // Parity Flag
           (eflags & 0x010) ? 1 : 0,  // Auxiliary Flag
           (eflags & 0x040) ? 1 : 0,  // Zero Flag
           (eflags & 0x080) ? 1 : 0,  // Sign Flag
           (eflags & 0x200) ? 1 : 0,  // Interrupt Flag
           (eflags & 0x400) ? 1 : 0,  // Direction Flag
           (eflags & 0x800) ? 1 : 0); // Overflow Flag
    
    return 0;
}

int cmd_stack(int argc, char* argv[]) {
    uint32_t* stack_ptr;
    __asm__ volatile("movl %%esp, %0" : "=r"(stack_ptr));
    
    printf("Stack dump (ESP: 0x%08X):\n", (uint32_t)stack_ptr);
    
    for (int i = 0; i < 16; i++) {
        printf("  [ESP+%02d] 0x%08X: 0x%08X\n", 
               i * 4, (uint32_t)(stack_ptr + i), stack_ptr[i]);
    }
    
    return 0;
}

int cmd_cpuid(int argc, char* argv[]) {
    uint32_t eax, ebx, ecx, edx;
    
    // CPUID leaf 0 - Vendor ID
    __asm__ volatile(
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(0)
    );
    
    char vendor[13];
    *((uint32_t*)&vendor[0]) = ebx;
    *((uint32_t*)&vendor[4]) = edx;
    *((uint32_t*)&vendor[8]) = ecx;
    vendor[12] = '\0';
    
    printf("CPU Information:\n");
    printf("  Vendor: %s\n", vendor);
    printf("  Max CPUID level: %u\n", eax);
    
    if (eax >= 1) {
        // CPUID leaf 1 - Feature flags
        __asm__ volatile(
            "cpuid"
            : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
            : "a"(1)
        );
        
        printf("  Family: %u\n", (eax >> 8) & 0xF);
        printf("  Model: %u\n", (eax >> 4) & 0xF);
        printf("  Stepping: %u\n", eax & 0xF);
        
        printf("  Features (EDX): ");
        if (edx & (1 << 0)) printf("FPU ");
        if (edx & (1 << 4)) printf("TSC ");
        if (edx & (1 << 15)) printf("CMOV ");
        if (edx & (1 << 23)) printf("MMX ");
        if (edx & (1 << 25)) printf("SSE ");
        if (edx & (1 << 26)) printf("SSE2 ");
        printf("\n");
    }
    
    return 0;
}

// ============================================================================
// COMANDOS DE RED Y COMUNICACIONES (STUBS)
// ============================================================================

int cmd_ping(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: ping <host>\n");
        return 1;
    }
    
    printf("PING %s: Network stack not implemented\n", argv[1]);
    printf("This would send ICMP echo requests when networking is available\n");
    return 0;
}

int cmd_netstat(int argc, char* argv[]) {
    printf("Network Statistics:\n");
    printf("  Network stack: Not implemented\n");
    printf("  Interfaces: None\n");
    printf("  Connections: None\n");
    return 0;
}

// ============================================================================
// COMANDOS DEL SISTEMA DE ARCHIVOS AVANZADOS
// ============================================================================

int cmd_find(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: find <pattern>\n");
        return 1;
    }
    
    printf("Searching for files matching '%s':\n", argv[1]);
    
    // Buscar en el sistema de archivos virtual
    extern VirtualFile virtual_fs[];
    
    for (int i = 0; virtual_fs[i].exists; i++) {
        if (shell_strstr(virtual_fs[i].name, argv[1]) != 0) {
            printf("  %s %s\n", 
                   virtual_fs[i].is_directory ? "[DIR] " : "[FILE]",
                   virtual_fs[i].name);
        }
    }
    
    return 0;
}

int cmd_grep(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Usage: grep <pattern> <file>\n");
        return 1;
    }
    
    printf("Searching for '%s' in file '%s':\n", argv[1], argv[2]);
    
    // Buscar archivo en el sistema virtual
    extern VirtualFile virtual_fs[];
    
    for (int i = 0; virtual_fs[i].exists; i++) {
        if (!virtual_fs[i].is_directory && 
            shell_strcmp(virtual_fs[i].name, argv[2]) == 0) {
            
            char* content = virtual_fs[i].content;
            char* line_start = content;
            int line_number = 1;
            
            while (*line_start) {
                char* line_end = shell_strchr(line_start, '\n');
                int line_length = line_end ? (line_end - line_start) : shell_strlen(line_start);
                
                // Crear una copia temporal de la línea
                char line_buffer[256];
                shell_strncpy(line_buffer, line_start, line_length);
                line_buffer[line_length] = '\0';
                
                if (shell_strstr(line_buffer, argv[1]) != 0) {
                    printf("  %d: %s\n", line_number, line_buffer);
                }
                
                line_number++;
                line_start = line_end ? line_end + 1 : line_start + line_length;
                if (!line_end) break;
            }
            
            return 0;
        }
    }
    
    printf("grep: %s: No such file\n", argv[2]);
    return 1;
}

int cmd_wc(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: wc <file>\n");
        return 1;
    }
    
    // Buscar archivo en el sistema virtual
    extern VirtualFile virtual_fs[];
    
    for (int i = 0; virtual_fs[i].exists; i++) {
        if (!virtual_fs[i].is_directory && 
            shell_strcmp(virtual_fs[i].name, argv[1]) == 0) {
            
            char* content = virtual_fs[i].content;
            int lines = 0, words = 0, chars = 0;
            bool in_word = false;
            
            while (*content) {
                chars++;
                
                if (*content == '\n') {
                    lines++;
                }
                
                if (*content == ' ' || *content == '\t' || *content == '\n') {
                    if (in_word) {
                        words++;
                        in_word = false;
                    }
                } else {
                    in_word = true;
                }
                
                content++;
            }
            
            if (in_word) words++;
            
            printf("  %d lines, %d words, %d characters in %s\n", 
                   lines, words, chars, argv[1]);
            return 0;
        }
    }
    
    printf("wc: %s: No such file\n", argv[1]);
    return 1;
}