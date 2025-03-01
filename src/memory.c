/** @file memory.c
 *
 *  A portable PICO-8 emulator written in C.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#include <SDL3/SDL.h>
#include <stdint.h>
#include "auxiliary.h"
#include "config.h"
#include "memory.h"

uint8_t pico8_ram[RAM_SIZE];

static SDL_Texture* screen;
static SDL_Texture* sprite_sheet;

bool init_memory(SDL_Renderer* renderer)
{
    if (!renderer)
    {
        return false;
    }

    SDL_memset(&pico8_ram, 0x00, RAM_SIZE);

    screen = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_SIZE, SCREEN_SIZE);
    if (screen == NULL)
    {
        SDL_Log("Could not create screen texture: %s", SDL_GetError());
        return false;
    }

    sprite_sheet = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_SIZE, SCREEN_SIZE);
    if (screen == NULL)
    {
        SDL_Log("Could not create sprite sheet texture: %s", SDL_GetError());
        return false;
    }

    return true;
}

void destroy_memory(void)
{
    if (sprite_sheet)
    {
        SDL_DestroyTexture(sprite_sheet);
    }

    if (screen)
    {
        SDL_DestroyTexture(screen);
    }
}

void update_from_virtual_memory(SDL_Renderer* renderer)
{
    if (!renderer)
    {
        return;
    }

    /* Update screen.
     * 0x6000 - 0x7FFF
     * All 128 rows of the screen, top to bottom. Each row contains 128 pixels in 64 bytes.
     * Each byte contains two adjacent pixels, with the low 4 bits being the left/even pixel
     * and the high 4 bits being the right/odd pixel.
     *
     * Each pixel can be one of 16 colors, and is represented by a 4 - bit value, also known
     * as a nybble. Thus, an 8 - bit byte represents two pixels, horizontally adjacent, where
     * the most - significant(left - most) 4 - bit nybble is the right pixel of the pair, and
     * the least - significant(right - most) 4 - bit nybble is the left pixel.
     *
     */

    void* pixels;
    int pitch;

    if (!SDL_LockTexture(screen, NULL, &pixels, &pitch))
    {
        for (int y = 0; y < SCREEN_SIZE; y++)
        {
            for (int x = 0; x < SCREEN_SIZE; x++)
            {
                uint8_t byte = peek(0x6000 + y * 64 + x / 2);
                uint8_t color = (x % 2 == 0) ? byte & 0x0F : byte >> 4;
                uint32_t* pixel = (uint32_t*)pixels + y * SCREEN_SIZE + x;
                *pixel = lookup_color(color);
            }
        }
        SDL_UnlockTexture(screen);
    }
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
