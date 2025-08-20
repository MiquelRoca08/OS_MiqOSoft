#pragma once

#include <stdint.h>

// Command function declarations
int cmd_memtest(int argc, char* argv[]);
int cmd_ports(int argc, char* argv[]);
int cmd_interrupt(int argc, char* argv[]);
int cmd_hexdump(int argc, char* argv[]);
int cmd_keytest(int argc, char* argv[]);
int cmd_benchmark(int argc, char* argv[]);
int cmd_registers(int argc, char* argv[]);
int cmd_stack(int argc, char* argv[]);
int cmd_cpuid(int argc, char* argv[]);
int cmd_ping(int argc, char* argv[]);
int cmd_netstat(int argc, char* argv[]);
int cmd_find(int argc, char* argv[]);
int cmd_grep(int argc, char* argv[]);
int cmd_wc(int argc, char* argv[]);
