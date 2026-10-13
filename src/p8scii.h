/** @file p8scii.h
 *
 *  A portable PICO-8 emulator written in C.
 *
 *  Copyright (c) 2025-2026, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#ifndef P8SCII_H
#define P8SCII_H

#include <stdint.h>

uint8_t p8scii_char_width(uint8_t char_index);
int p8scii_custom_char_width(uint8_t char_index);
int p8scii_line_height(void);
void p8scii_font_metrics(int custom, int* char_h, int* tab_width);
typedef struct p8scii_style
{
    uint8_t fg;
    int bg;
    uint8_t scale_x;
    uint8_t scale_y;
    uint8_t stripey;
    uint8_t invert;
    uint8_t outline_mask;
    uint8_t outline_color;
    uint8_t outline_hollow;
    uint8_t custom_font;
} p8scii_style_t;

void p8scii_draw_char(uint8_t char_index, int x, int y, int cell_w, int cell_h, const p8scii_style_t* style);
void p8scii_draw_bitmap(int x, int y, const uint8_t* rows, int fg, int bg);

#endif // P8SCII_H
