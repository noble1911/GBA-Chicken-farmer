#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "gba.h"

// Initialize video mode
void init_video();

// Drawing functions
void draw_pixel(int x, int y, u16 color);
void draw_rect(int x, int y, int width, int height, u16 color);
void fill_screen(u16 color);
void draw_char(int x, int y, char c, u16 color);
void draw_string(int x, int y, const char* str, u16 color);
void draw_number(int x, int y, int num, u16 color);

#endif // GRAPHICS_H
