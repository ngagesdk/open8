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

const uint8_t font[95][FONT_HEIGHT] =
{
    // ASCII (32 - 126)
    { 0b000, 0b000, 0b000, 0b000, 0b000 }, // space
    { 0b010, 0b010, 0b010, 0b000, 0b010 }, // !
    { 0b101, 0b101, 0b000, 0b000, 0b000 }, // "
    { 0b101, 0b111, 0b101, 0b111, 0b101 }, // #
    { 0b111, 0b110, 0b011, 0b111, 0b010 }, // $
    { 0b101, 0b001, 0b010, 0b100, 0b101 }, // %
    { 0b110, 0b110, 0b011, 0b101, 0b111 }, // &
    { 0b010, 0b100, 0b000, 0b000, 0b000 }, // '
    { 0b010, 0b100, 0b100, 0b100, 0b010 }, // (
    { 0b010, 0b001, 0b001, 0b001, 0b010 }, // )
    { 0b101, 0b010, 0b111, 0b010, 0b101 }, // *
    { 0b000, 0b010, 0b111, 0b010, 0b000 }, // +
    { 0b000, 0b000, 0b000, 0b010, 0b100 }, // ,
    { 0b000, 0b000, 0b111, 0b000, 0b000 }, // -
    { 0b000, 0b000, 0b000, 0b000, 0b010 }, // .
    { 0b001, 0b010, 0b010, 0b010, 0b100 }, // /
    { 0b111, 0b101, 0b101, 0b101, 0b111 }, // 0
    { 0b110, 0b010, 0b010, 0b010, 0b111 }, // 1
    { 0b111, 0b001, 0b111, 0b100, 0b111 }, // 2
    { 0b111, 0b001, 0b011, 0b001, 0b111 }, // 3
    { 0b101, 0b101, 0b111, 0b001, 0b001 }, // 4
    { 0b111, 0b100, 0b111, 0b001, 0b111 }, // 5
    { 0b100, 0b100, 0b111, 0b101, 0b111 }, // 6
    { 0b111, 0b001, 0b001, 0b001, 0b001 }, // 7
    { 0b111, 0b101, 0b111, 0b101, 0b111 }, // 8
    { 0b111, 0b101, 0b111, 0b001, 0b001 }, // 9
    { 0b000, 0b010, 0b000, 0b010, 0b000 }, // :
    { 0b000, 0b010, 0b000, 0b010, 0b100 }, // ;
    { 0b001, 0b010, 0b100, 0b010, 0b001 }, // <
    { 0b000, 0b111, 0b000, 0b111, 0b000 }, // =
    { 0b100, 0b010, 0b001, 0b010, 0b100 }, // >
    { 0b111, 0b001, 0b011, 0b000, 0b010 }, // ?
    { 0b010, 0b101, 0b101, 0b100, 0b011 }, // @
    { 0b000, 0b011, 0b101, 0b111, 0b101 }, // A
    { 0b000, 0b110, 0b110, 0b101, 0b111 }, // B
    { 0b000, 0b011, 0b100, 0b100, 0b011 }, // C
    { 0b000, 0b110, 0b101, 0b101, 0b110 }, // D
    { 0b000, 0b111, 0b110, 0b100, 0b011 }, // E
    { 0b000, 0b111, 0b110, 0b100, 0b100 }, // F
    { 0b000, 0b011, 0b100, 0b101, 0b111 }, // G
    { 0b000, 0b101, 0b101, 0b111, 0b101 }, // H
    { 0b000, 0b111, 0b010, 0b010, 0b111 }, // I
    { 0b000, 0b111, 0b010, 0b010, 0b110 }, // J
    { 0b000, 0b101, 0b110, 0b101, 0b101 }, // K
    { 0b000, 0b100, 0b100, 0b100, 0b011 }, // L
    { 0b000, 0b111, 0b111, 0b101, 0b101 }, // M
    { 0b000, 0b110, 0b101, 0b101, 0b101 }, // N
    { 0b000, 0b011, 0b101, 0b101, 0b110 }, // O
    { 0b000, 0b011, 0b101, 0b111, 0b100 }, // P
    { 0b000, 0b010, 0b101, 0b110, 0b011 }, // Q
    { 0b000, 0b110, 0b101, 0b110, 0b101 }, // R
    { 0b000, 0b011, 0b100, 0b001, 0b110 }, // S
    { 0b000, 0b111, 0b010, 0b010, 0b010 }, // T
    { 0b000, 0b101, 0b101, 0b101, 0b011 }, // U
    { 0b000, 0b101, 0b101, 0b111, 0b010 }, // V
    { 0b000, 0b101, 0b101, 0b111, 0b111 }, // W
    { 0b000, 0b101, 0b010, 0b010, 0b101 }, // X
    { 0b000, 0b101, 0b111, 0b001, 0b110 }, // Y
    { 0b000, 0b111, 0b001, 0b100, 0b111 }, // Z
    { 0b110, 0b100, 0b100, 0b100, 0b110 }, // [
    { 0b100, 0b010, 0b010, 0b010, 0b001 }, // '\'
    { 0b011, 0b001, 0b001, 0b001, 0b011 }, // ]
    { 0b010, 0b101, 0b000, 0b000, 0b000 }, // ^
    { 0b000, 0b000, 0b000, 0b000, 0b111 }, // _
    { 0b010, 0b001, 0b000, 0b000, 0b000 }, // `
    { 0b111, 0b101, 0b111, 0b101, 0b101 }, // a
    { 0b111, 0b101, 0b110, 0b101, 0b111 }, // b
    { 0b011, 0b100, 0b100, 0b100, 0b011 }, // c
    { 0b110, 0b101, 0b101, 0b101, 0b111 }, // d
    { 0b111, 0b100, 0b110, 0b100, 0b111 }, // e
    { 0b111, 0b100, 0b110, 0b100, 0b100 }, // f
    { 0b011, 0b100, 0b100, 0b101, 0b111 }, // g
    { 0b101, 0b101, 0b111, 0b101, 0b101 }, // h
    { 0b101, 0b010, 0b010, 0b010, 0b101 }, // i
    { 0b101, 0b010, 0b010, 0b010, 0b110 }, // j
    { 0b101, 0b101, 0b110, 0b101, 0b101 }, // k
    { 0b100, 0b100, 0b100, 0b100, 0b111 }, // l
    { 0b111, 0b111, 0b101, 0b101, 0b101 }, // m
    { 0b110, 0b101, 0b101, 0b101, 0b101 }, // n
    { 0b001, 0b101, 0b101, 0b101, 0b110 }, // o
    { 0b111, 0b101, 0b111, 0b100, 0b100 }, // p
    { 0b010, 0b101, 0b101, 0b110, 0b011 }, // q
    { 0b111, 0b101, 0b110, 0b101, 0b101 }, // r
    { 0b011, 0b100, 0b111, 0b001, 0b110 }, // s
    { 0b111, 0b010, 0b010, 0b010, 0b010 }, // t
    { 0b101, 0b101, 0b101, 0b101, 0b011 }, // u
    { 0b101, 0b101, 0b101, 0b111, 0b010 }, // v
    { 0b101, 0b101, 0b101, 0b111, 0b111 }, // w
    { 0b101, 0b101, 0b010, 0b101, 0b101 }, // x
    { 0b101, 0b101, 0b111, 0b001, 0b111 }, // y
    { 0b111, 0b001, 0b010, 0b101, 0b101 }, // z
    { 0b011, 0b010, 0b110, 0b010, 0b011 }, // {
    { 0b010, 0b010, 0b010, 0b010, 0b010 }, // |
    { 0b110, 0b010, 0b011, 0b010, 0b110 }, // }
    { 0b000, 0b001, 0b111, 0b100, 0b000 }  // ~
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
