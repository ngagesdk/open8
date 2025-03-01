/** @file memory.c
 *
 *  A portable PICO-8 emulator written in C.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#include "memory.h"

#include <SDL3/SDL.h>
#include <stdint.h>

uint8_t pico8_ram[RAM_SIZE];

void init_memory(void)
{
    SDL_memset(&pico8_ram, 0x00, RAM_SIZE);
}

void update_from_virtual_memory(SDL_Renderer* renderer)
{
    /* Update screen.
     * 0x6000 - 0x7FFF
     * All 128 rows of the screen, top to bottom. Each row contains 128 pixels in 64 bytes.
     * Each byte contains two adjacent pixels, with the low 4 bits being the left/even pixel
     * and the high 4 bits being the right/odd pixel.
     */



}

/****************************
 * Memory access functions. *
 ****************************/

uint8_t peek(uint16_t addr)
{
    if (addr >= RAM_SIZE-1)
    {
        return 0;
    }

    return (pico8_ram[addr]);
}

void poke(uint16_t addr, uint8_t data)
{
    if (addr >= RAM_SIZE-1)
    {
        return;
    }
    pico8_ram[addr] = data;
}
