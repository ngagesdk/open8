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

    if (!SDL_SetTextureScaleMode(screen, SDL_SCALEMODE_NEAREST))
    {
        SDL_Log("Couldn't set texture scale mode: %s", SDL_GetError());
    }

    sprite_sheet = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_SIZE, SCREEN_SIZE);
    if (screen == NULL)
    {
        SDL_Log("Could not create sprite sheet texture: %s", SDL_GetError());
        return false;
    }

    if (!SDL_SetTextureScaleMode(sprite_sheet, SDL_SCALEMODE_NEAREST))
    {
        SDL_Log("Couldn't set texture scale mode: %s", SDL_GetError());
    }

    return true;
}

void reset_memory(void)
{
    SDL_memset(&pico8_ram, 0x00, RAM_SIZE);
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

    if (SDL_LockTexture(screen, NULL, &pixels, &pitch))
    {
        uint8_t* row = (uint8_t*)pixels;
        for (int y = 0; y < SCREEN_SIZE; y++, row += pitch)
        {
            uint32_t* pixel = (uint32_t*)row;
            for (int x = 0; x < SCREEN_SIZE; x++, pixel++)
            {
                uint8_t byte = peek(0x6000 + (y << 6) + (x >> 1));
                uint8_t color = (byte >> ((x & 1) << 2)) & 0xF;
                *pixel = lookup_color(color);
            }
        }
        SDL_UnlockTexture(screen);
    }

    SDL_FRect dest;
    dest.x = SCREEN_OFFSET_X;
    dest.y = SCREEN_OFFSET_Y;
    dest.w = SCREEN_SIZE;
    dest.h = SCREEN_SIZE;

    SDL_RenderTexture(renderer, screen, NULL, &dest);
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

void p8_memset(uint16_t addr, uint8_t data, uint16_t len)
{
    if (addr + len >= RAM_SIZE)
    {
        len = RAM_SIZE - addr;
    }
    for (uint16_t i = 0; i < len; i++)
    {
        pico8_ram[addr + i] = data;
    }
}

bool is_fill_mask_bit_set(int x, int y)
{
    uint16_t pattern = (pico8_ram[0x5f31] << 8) | pico8_ram[0x5f32];
    int row = (y % 4);
    int col = (x % 4);

    uint8_t nibble = 0;
    switch (row)
    {
        case 0: nibble = (pattern & 0xF000) >> 12; break;
        case 1: nibble = (pattern & 0x0F00) >> 8;  break;
        case 2: nibble = (pattern & 0x00F0) >> 4;  break;
        case 3: nibble = (pattern & 0x000F);       break;
    }

    uint8_t pixel = (nibble >> (3 - col)) & 0x1;
    return pixel != 0;
}

void set_fill_mask_bit(int x, int y)
{
    uint16_t pattern = (pico8_ram[0x5f31] << 8) | pico8_ram[0x5f32];
    int row = (y % 4);
    int col = (x % 4);

    uint8_t nibble = 0;
    switch (row)
    {
        case 0: nibble = (pattern & 0xF000) >> 12; break;
        case 1: nibble = (pattern & 0x0F00) >> 8;  break;
        case 2: nibble = (pattern & 0x00F0) >> 4;  break;
        case 3: nibble = (pattern & 0x000F);       break;
    }

    uint8_t pixel = (nibble >> (3 - col)) & 0x1;
    if (pixel)
    {
        pico8_ram[0x6000 + y * 128 + x] = 0xFF;
    }
}
