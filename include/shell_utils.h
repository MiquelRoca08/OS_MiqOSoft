#pragma once

#include <stddef.h>
#include <stdarg.h>

// ============================================================================
// UTILIDADES DE LA SHELL
// ============================================================================

// Este header ahora es mucho más simple ya que la mayoría de funciones
// se han movido a shell_commands.h o están implementadas internamente

// ============================================================================
// DEFINICIONES DE COMPATIBILIDAD
// ============================================================================

// Definición de size_t si no está disponible
#ifndef SIZE_T_DEFINED
#define SIZE_T_DEFINED
typedef unsigned long size_t;
#endif

// ============================================================================
// FUNCIONES DE FORMATEO
// ============================================================================

// Función auxiliar para formatear números (implementada en shell_main.c)
void format_number(char* buffer, int num);

// ============================================================================
// FUNCIONES DE PRINTF ALTERNATIVAS
// ============================================================================

// Estas funciones pueden faltar en algunos entornos embedded
// Si están disponibles en tu sistema, puedes comentar estas declaraciones

#ifndef PRINTF_FUNCTIONS_AVAILABLE
int snprintf(char* buffer, size_t size, const char* format, ...);
int vsnprintf(char* buffer, size_t size, const char* format, va_list args);
int vprintf(const char* format, va_list args);
#endif

// ============================================================================
// MACROS DE UTILIDAD
// ============================================================================

// Macro para obtener el tamaño de un array
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

// Macros para min/max (si no están definidas en minmax.h)
#ifndef MIN_MAX_DEFINED
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif