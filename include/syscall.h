#pragma once

#include <stdint.h>
#include <stddef.h>

// Interrupt number for syscalls
#define SYSCALL_INTERRUPT   0x80

// Syscall numbers
typedef enum {
    SYSCALL_EXIT = 0,
    SYSCALL_PRINT = 1,
    SYSCALL_READ = 2,
    SYSCALL_MALLOC = 3,
    SYSCALL_FREE = 4,
    SYSCALL_OPEN = 5,
    SYSCALL_CLOSE = 6,
    SYSCALL_WRITE = 7,
    SYSCALL_SEEK = 8,
    SYSCALL_GETPID = 9,
    SYSCALL_TIME = 10,
    SYSCALL_SLEEP = 11,
    SYSCALL_YIELD = 12,
    SYSCALL_FORK = 13,
    SYSCALL_EXEC = 14,
    SYSCALL_WAIT = 15,
    SYSCALL_KILL = 16,
    SYSCALL_MMAP = 17,
    SYSCALL_MUNMAP = 18,
    SYSCALL_STAT = 19,
    SYSCALL_CHDIR = 20,
    SYSCALL_GETCWD = 21,
    SYSCALL_MKDIR = 22,
    SYSCALL_RMDIR = 23,
    SYSCALL_UNLINK = 24,
    SYSCALL_MOUNT = 25,
    SYSCALL_UMOUNT = 26,
    
    SYSCALL_COUNT = 27  // Total number of syscalls
} syscall_number_t;

// Error codes
typedef enum {
    SYSCALL_OK = 0,
    SYSCALL_ERROR = -1,
    SYSCALL_INVALID_SYSCALL = -2,
    SYSCALL_INVALID_PARAMS = -3,
    SYSCALL_PERMISSION_DENIED = -4,
    SYSCALL_NOT_FOUND = -5,
    SYSCALL_OUT_OF_MEMORY = -6,
    SYSCALL_IO_ERROR = -7,
    SYSCALL_BUSY = -8,
    SYSCALL_EXISTS = -9,
    SYSCALL_NOT_DIRECTORY = -10,
    SYSCALL_IS_DIRECTORY = -11,
    SYSCALL_NOT_EMPTY = -12
} syscall_error_t;

// Structure for file information
typedef struct {
    uint32_t size;
    uint32_t type;      // 0 = file, 1 = directory
    uint32_t mode;
    uint32_t created_time;
    uint32_t modified_time;
} stat_info_t;

// Flags for open
#define OPEN_READ       0x01
#define OPEN_WRITE      0x02
#define OPEN_CREATE     0x04
#define OPEN_TRUNCATE   0x08
#define OPEN_APPEND     0x10

//Flags for mmap
#define MMAP_READ       0x01
#define MMAP_WRITE      0x02
#define MMAP_EXECUTE    0x04
#define MMAP_PRIVATE    0x08
#define MMAP_SHARED     0x10

// Function type for syscall handlers
typedef int32_t (*syscall_handler_t)(uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4);

// Kernel functions for handling syscalls
void syscall_initialize(void);
void syscall_register_handler(syscall_number_t num, syscall_handler_t handler);
int32_t syscall_dispatch(uint32_t syscall_num, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4);

// Utility functions for making syscalls from kernel code
int32_t syscall_invoke(syscall_number_t num, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4);

// Macro to define syscalls inline assembly (for userland)
#define SYSCALL0(num) ({ \
    int32_t result; \
    __asm__ volatile( \
        "int $0x80" \
        : "=a"(result) \
        : "a"(num) \
        : "memory" \
    ); \
    result; \
})

#define SYSCALL1(num, arg1) ({ \
    int32_t result; \
    __asm__ volatile( \
        "int $0x80" \
        : "=a"(result) \
        : "a"(num), "b"(arg1) \
        : "memory" \
    ); \
    result; \
})

#define SYSCALL2(num, arg1, arg2) ({ \
    int32_t result; \
    __asm__ volatile( \
        "int $0x80" \
        : "=a"(result) \
        : "a"(num), "b"(arg1), "c"(arg2) \
        : "memory" \
    ); \
    result; \
})

#define SYSCALL3(num, arg1, arg2, arg3) ({ \
    int32_t result; \
    __asm__ volatile( \
        "int $0x80" \
        : "=a"(result) \
        : "a"(num), "b"(arg1), "c"(arg2), "d"(arg3) \
        : "memory" \
    ); \
    result; \
})

#define SYSCALL4(num, arg1, arg2, arg3, arg4) ({ \
    int32_t result; \
    __asm__ volatile( \
        "int $0x80" \
        : "=a"(result) \
        : "a"(num), "b"(arg1), "c"(arg2), "d"(arg3), "S"(arg4) \
        : "memory" \
    ); \
    result; \
})

// Wrapper functions for syscalls (for userland)
static inline int32_t sys_exit(int32_t code) {
    return SYSCALL1(SYSCALL_EXIT, code);
}

static inline int32_t sys_print(const char* msg) {
    return SYSCALL1(SYSCALL_PRINT, (uint32_t)msg);
}

static inline int32_t sys_read(int32_t fd, void* buffer, size_t count) {
    return SYSCALL3(SYSCALL_READ, fd, (uint32_t)buffer, count);
}

static inline void* sys_malloc(size_t size) {
    return (void*)SYSCALL1(SYSCALL_MALLOC, size);
}

static inline int32_t sys_free(void* ptr) {
    return SYSCALL1(SYSCALL_FREE, (uint32_t)ptr);
}

static inline int32_t sys_open(const char* path, uint32_t flags) {
    return SYSCALL2(SYSCALL_OPEN, (uint32_t)path, flags);
}

static inline int32_t sys_close(int32_t fd) {
    return SYSCALL1(SYSCALL_CLOSE, fd);
}

static inline int32_t sys_write(int32_t fd, const void* buffer, size_t count) {
    return SYSCALL3(SYSCALL_WRITE, fd, (uint32_t)buffer, count);
}

static inline int32_t sys_getpid(void) {
    return SYSCALL0(SYSCALL_GETPID);
}

static inline uint32_t sys_time(void) {
    return SYSCALL0(SYSCALL_TIME);
}

static inline int32_t sys_sleep(uint32_t milliseconds) {
    return SYSCALL1(SYSCALL_SLEEP, milliseconds);
}

static inline int32_t sys_yield(void) {
    return SYSCALL0(SYSCALL_YIELD);
}