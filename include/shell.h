#pragma once

#include <stdint.h>
#include <stdbool.h>

#define SHELL_BUFFER_SIZE 256
#define SHELL_MAX_ARGS 16
#define SHELL_PROMPT "miqos> "

// Estructura para el sistema de archivos virtual
typedef struct {
    char name[64];
    char content[256];
    bool is_directory;
    bool exists;
} VirtualFile;

// Declaración externa del sistema de archivos virtual
extern VirtualFile virtual_fs[];

typedef struct {
    char* name;
    char* description;
    int (*function)(int argc, char* argv[]);
} ShellCommand;

// Funciones de utilidad de string para la shell
int shell_strlen(const char* str);
int shell_strcmp(const char* s1, const char* s2);
char* shell_strchr(const char* str, int c);
char* shell_strstr(const char* haystack, const char* needle);
char* shell_strncpy(char* dest, const char* src, int n);

// Función para controlar el modo shell en keyboard.c
void keyboard_set_shell_mode(bool enabled);

// Funciones principales de la shell
void shell_init(void);
void shell_run(void);
void shell_process_command(const char* input);
char* shell_get_current_line(void);  // Nueva función

// Utilidades
void shell_print_prompt(void);
int shell_parse_command(const char* input, char* argv[]);
ShellCommand* shell_find_command(const char* name);

// Comandos built-in
int cmd_help(int argc, char* argv[]);
int cmd_clear(int argc, char* argv[]);
int cmd_echo(int argc, char* argv[]);
int cmd_version(int argc, char* argv[]);
int cmd_reboot(int argc, char* argv[]);
int cmd_panic(int argc, char* argv[]);
int cmd_memory(int argc, char* argv[]);
int cmd_uptime(int argc, char* argv[]);
int cmd_lspci(int argc, char* argv[]);
int cmd_cpuinfo(int argc, char* argv[]);

// Sistema de archivos simulado (para demostración)
int cmd_ls(int argc, char* argv[]);
int cmd_cat(int argc, char* argv[]);
int cmd_mkdir(int argc, char* argv[]);
int cmd_rm(int argc, char* argv[]);

// Utilidades del kernel
int cmd_lsmod(int argc, char* argv[]);
int cmd_dmesg(int argc, char* argv[]);
int cmd_ps(int argc, char* argv[]);

// Comandos de sistema y hardware
int cmd_memtest(int argc, char* argv[]);
int cmd_ports(int argc, char* argv[]);
int cmd_interrupt(int argc, char* argv[]);
int cmd_hexdump(int argc, char* argv[]);
int cmd_keytest(int argc, char* argv[]);
int cmd_benchmark(int argc, char* argv[]);

// Comandos de debugging
int cmd_registers(int argc, char* argv[]);
int cmd_stack(int argc, char* argv[]);
int cmd_cpuid(int argc, char* argv[]);

// Comandos de red (stubs)
int cmd_ping(int argc, char* argv[]);
int cmd_netstat(int argc, char* argv[]);

// Comandos avanzados de archivos
int cmd_find(int argc, char* argv[]);
int cmd_grep(int argc, char* argv[]);
int cmd_wc(int argc, char* argv[]);