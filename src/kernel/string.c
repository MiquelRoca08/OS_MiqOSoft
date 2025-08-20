#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

int strcmp(const char* a, const char* b) {
    while (*a && *b && *a == *b) {
        ++a;
        ++b;
    }
    return *a - *b;
}

char* strcpy(char* dst, const char* src) {
    char* origDst = dst;

    if (dst == NULL)
        return NULL;

    if (src == NULL) {
        *dst = '\0';
        return dst;
    }

    while (*src) {
        *dst = *src;
        ++src;
        ++dst;
    }
    
    *dst = '\0';
    return origDst;
}

unsigned strlen(const char* str) {
    unsigned len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

char* strncpy(char* dest, const char* src, size_t n) {
    size_t i;

    for (i = 0; i < n && src[i] != '\0'; i++)
        dest[i] = src[i];
    for (; i < n; i++)
        dest[i] = '\0';

    return dest;
}

// Simple implementation of vsprintf/snprintf
static void itoa(int num, char* str, int base) {
    int i = 0;
    int isNegative = 0;

    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }

    if (num < 0 && base == 10) {
        isNegative = 1;
        num = -num;
    }

    while (num != 0) {
        int rem = num % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        num = num / base;
    }

    if (isNegative)
        str[i++] = '-';

    str[i] = '\0';

    // Reverse the string
    int start = 0;
    int end = i - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

int vsprintf(char* str, const char* format, va_list args) {
    if (!str || !format)
        return -1;

    char* str_ptr = str;
    const char* fmt_ptr = format;
    char num_str[32];

    while (*fmt_ptr) {
        if (*fmt_ptr != '%') {
            *str_ptr++ = *fmt_ptr++;
            continue;
        }

        fmt_ptr++; // Skip '%'

        switch (*fmt_ptr) {
            case 's': {
                const char* s = va_arg(args, const char*);
                if (!s) s = "(null)";
                while (*s) *str_ptr++ = *s++;
                break;
            }
            case 'd': {
                int num = va_arg(args, int);
                itoa(num, num_str, 10);
                char* s = num_str;
                while (*s) *str_ptr++ = *s++;
                break;
            }
            case 'x': {
                unsigned int num = va_arg(args, unsigned int);
                itoa(num, num_str, 16);
                char* s = num_str;
                while (*s) *str_ptr++ = *s++;
                break;
            }
            case 'c': {
                char c = (char)va_arg(args, int);
                *str_ptr++ = c;
                break;
            }
            case '%':
                *str_ptr++ = '%';
                break;
            default:
                *str_ptr++ = '%';
                *str_ptr++ = *fmt_ptr;
        }
        fmt_ptr++;
    }

    *str_ptr = '\0';
    return str_ptr - str;
}

int snprintf(char* str, size_t size, const char* format, ...) {
    if (!str || !format || size == 0)
        return -1;
        
    va_list args;
    va_start(args, format);
    
    // Create a temporary buffer for vsprintf
    char temp[1024]; // Assuming max length is 1024
    int result = vsprintf(temp, format, args);
    va_end(args);
    
    // Copy to final buffer with size check
    if (result >= 0) {
        size_t to_copy = (result < (int)(size - 1)) ? result : (size - 1);
        strncpy(str, temp, to_copy);
        str[to_copy] = '\0';
    } else if (size > 0) {
        str[0] = '\0';
    }
        
    return result;
}
