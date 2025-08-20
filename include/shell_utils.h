#pragma once

#include <stddef.h>

// Definición de size_t si no está disponible
#ifndef SIZE_T_DEFINED
#define SIZE_T_DEFINED
typedef unsigned long size_t;
#endif

// Declaraciones de funciones que pueden faltar
int snprintf(char* buffer, size_t size, const char* format, ...);
int vsnprintf(char* buffer, size_t size, const char* format, va_list args);
int vprintf(const char* format, va_list args);

// Función auxiliar para formatear números
void format_number(char* buffer, int num);