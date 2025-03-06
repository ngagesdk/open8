/** @file p8scii.c
 *
 *  A portable PICO-8 emulator written in C.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#include <stdint.h>
#include "memory.h"
#include "p8scii.h"

#define FONT_WIDTH 3
#define FONT_HEIGHT 5

const uint8_t font[256][FONT_HEIGHT] =
{
    { 0b111, 0b101, 0b101, 0b101, 0b111 }
};

void blit_char_to_screen(uint8_t char_index, int x, int y, uint8_t color)
{
    const uint8_t* char_bitmap = font[char_index];

    for (int row = 0; row < FONT_HEIGHT; row++)
    {
        uint8_t row_data = char_bitmap[row];
        for (int col = 0; col < FONT_WIDTH; col++)
        {
            if (row_data & (1 << (FONT_WIDTH - 1 - col)))
            {
                int screen_x = x + col;
                int screen_y = y + row;
                uint16_t addr = 0x6000 + (screen_y << 6) + (screen_x >> 1);
                uint8_t mask = (screen_x & 1) ? 0x0F : 0xF0;
                uint8_t shift = (screen_x & 1) ? 4 : 0;
                pico8_ram[addr] = (pico8_ram[addr] & mask) | (color << shift);
            }
        }
    }
}
