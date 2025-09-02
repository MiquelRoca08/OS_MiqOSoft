#pragma once

#include <stddef.h>
#include <stdarg.h>

//
// Compatibility Definitions
//

// Defining size_t if not available
#ifndef SIZE_T_DEFINED
#define SIZE_T_DEFINED
typedef unsigned long size_t;
#endif

//
// Formatting Functions
//

// Helper function for formatting numbers
void format_number(char* buffer, int num);

//
// Alternative Prinf Functions
//

// These functions may be missing in some embedded environments
// If they are available on your system, you can comment out these declarations

#ifndef PRINTF_FUNCTIONS_AVAILABLE
int snprintf(char* buffer, size_t size, const char* format, ...);
int vsnprintf(char* buffer, size_t size, const char* format, va_list args);
int vprintf(const char* format, va_list args);
#endif

//
// Utility Macros
//

// Macro to get the size of an array
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

// Macros for min/max
#ifndef MIN_MAX_DEFINED
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif