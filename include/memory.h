#pragma once
#include <stdint.h>
#include <stddef.h>

void* memcpy(void* dst, const void* src, uint16_t num);
void* memset(void* s, int c, size_t n);
int memcmp(const void* ptr1, const void* ptr2, uint16_t num);
