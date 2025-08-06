#pragma once

#include <stdint.h>

extern const unsigned SCREEN_WIDTH;
extern const unsigned SCREEN_HEIGHT;
extern int g_ScreenX, g_ScreenY;

void VGA_clrscr();
void VGA_putc(char c);
void VGA_setcursor(int x, int y);
void VGA_putchr(int x, int y, char c);

int VGA_get_cursor_x();
int VGA_get_cursor_y();