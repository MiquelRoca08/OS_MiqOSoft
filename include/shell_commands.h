#pragma once

#include <stdint.h>

//
// DATA TYPES
//

// Structure to define shell commands
typedef struct {
    const char* name;
    const char* description;
    int (*function)(int argc, char* argv[]);
} ShellCommandEntry;

// Basic Commands
int cmd_help(int argc, char* argv[]);
int cmd_clear(int argc, char* argv[]);
int cmd_echo(int argc, char* argv[]);
int cmd_version(int argc, char* argv[]);

// System Information
int cmd_memory(int argc, char* argv[]);
int cmd_uptime(int argc, char* argv[]);
int cmd_cpuinfo(int argc, char* argv[]);
int cmd_cpuid(int argc, char* argv[]);
int cmd_lsmod(int argc, char* argv[]);
int cmd_dmesg(int argc, char* argv[]);

// File System
int cmd_ls(int argc, char* argv[]);
int cmd_cat(int argc, char* argv[]);
int cmd_mkdir(int argc, char* argv[]);
int cmd_rm(int argc, char* argv[]);
int cmd_find(int argc, char* argv[]);
int cmd_grep(int argc, char* argv[]);
int cmd_wc(int argc, char* argv[]);
int cmd_create_file(int argc, char* argv[]);
int cmd_edit(int argc, char* argv[]);

// Hardware & Debug
int cmd_memtest(int argc, char* argv[]);
int cmd_ports(int argc, char* argv[]);
int cmd_interrupt(int argc, char* argv[]);
int cmd_hexdump(int argc, char* argv[]);
int cmd_keytest(int argc, char* argv[]);
int cmd_benchmark(int argc, char* argv[]);
int cmd_registers(int argc, char* argv[]);

// System Calls
int cmd_syscall_test(int argc, char* argv[]);
int cmd_syscall_info(int argc, char* argv[]);
int cmd_malloc_test(int argc, char* argv[]);
int cmd_heap_info(int argc, char* argv[]);
int cmd_sleep_test(int argc, char* argv[]);

// System Control
int cmd_reboot(int argc, char* argv[]);
int cmd_panic(int argc, char* argv[]);

//
// Command Table And Management Functions
//

// Main command table
extern const ShellCommandEntry commands[];

// Functions to manage the command table
int get_shell_command_count(void);
const ShellCommandEntry* find_shell_command(const char* name);

//
// Auxiliary Functions
//

// String conversion
uint32_t hex_str_to_int(const char* hex_str);
uint32_t dec_str_to_int(const char* dec_str);

char keyboard_scancode_to_ascii(uint8_t scancode);