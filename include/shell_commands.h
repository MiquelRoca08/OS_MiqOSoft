#pragma once

#include <stdint.h>

// ============================================================================
// DECLARACIONES DE COMANDOS UNIFICADOS
// ============================================================================

// Este header reemplaza a todos los anteriores:
// - shell_commands.h (anterior)
// - shell_commands_unified.h

// ============================================================================
// TIPOS DE DATOS
// ============================================================================

// Estructura para definir comandos de la shell
typedef struct {
    const char* name;
    const char* description;
    int (*function)(int argc, char* argv[]);
} ShellCommandEntry;

// ============================================================================
// COMANDOS BÁSICOS DEL SISTEMA
// ============================================================================

int cmd_help(int argc, char* argv[]);
int cmd_clear(int argc, char* argv[]);
int cmd_echo(int argc, char* argv[]);
int cmd_version(int argc, char* argv[]);

// ============================================================================
// INFORMACIÓN DEL SISTEMA
// ============================================================================

int cmd_memory(int argc, char* argv[]);
int cmd_uptime(int argc, char* argv[]);
int cmd_cpuinfo(int argc, char* argv[]);
int cmd_cpuid(int argc, char* argv[]);
int cmd_lsmod(int argc, char* argv[]);
int cmd_dmesg(int argc, char* argv[]);
int cmd_ps(int argc, char* argv[]);
int cmd_lspci(int argc, char* argv[]);

// ============================================================================
// SISTEMA DE ARCHIVOS VIRTUAL
// ============================================================================

int cmd_ls(int argc, char* argv[]);
int cmd_cat(int argc, char* argv[]);
int cmd_mkdir(int argc, char* argv[]);
int cmd_rm(int argc, char* argv[]);
int cmd_find(int argc, char* argv[]);
int cmd_grep(int argc, char* argv[]);
int cmd_wc(int argc, char* argv[]);
int cmd_create_file(int argc, char* argv[]);
int cmd_edit(int argc, char* argv[]);

// ============================================================================
// HARDWARE Y DEBUGGING
// ============================================================================

int cmd_memtest(int argc, char* argv[]);
int cmd_ports(int argc, char* argv[]);
int cmd_interrupt(int argc, char* argv[]);
int cmd_hexdump(int argc, char* argv[]);
int cmd_keytest(int argc, char* argv[]);
int cmd_benchmark(int argc, char* argv[]);
int cmd_registers(int argc, char* argv[]);
int cmd_stack(int argc, char* argv[]);

// ============================================================================
// COMANDOS DE RED (STUBS)
// ============================================================================

int cmd_ping(int argc, char* argv[]);
int cmd_netstat(int argc, char* argv[]);

// ============================================================================
// SYSTEM CALLS
// ============================================================================

int cmd_syscall_test(int argc, char* argv[]);
int cmd_syscall_info(int argc, char* argv[]);
int cmd_malloc_test(int argc, char* argv[]);
int cmd_heap_info(int argc, char* argv[]);
int cmd_sleep_test(int argc, char* argv[]);

// ============================================================================
// CONTROL DEL SISTEMA
// ============================================================================

int cmd_reboot(int argc, char* argv[]);
int cmd_panic(int argc, char* argv[]);

// ============================================================================
// TABLA DE COMANDOS Y FUNCIONES DE GESTIÓN
// ============================================================================

// Tabla principal de comandos
extern const ShellCommandEntry commands[];

// Funciones para gestionar la tabla de comandos
int get_shell_command_count(void);
const ShellCommandEntry* find_shell_command(const char* name);

// ============================================================================
// FUNCIONES AUXILIARES
// ============================================================================

// Estas funciones son utilizadas internamente por los comandos
// Están disponibles para facilitar el desarrollo de nuevos comandos

// Conversión de strings
uint32_t hex_str_to_int(const char* hex_str);
uint32_t dec_str_to_int(const char* dec_str);


char keyboard_scancode_to_ascii(uint8_t scancode);

// Nota: Las funciones hex_str_to_int y dec_str_to_int están implementadas
// como static en shell_commands_unified.c. Si necesitas usarlas desde
// otros archivos, cambia 'static' por 'extern' en la implementación.