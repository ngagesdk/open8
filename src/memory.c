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

#define CRC32_POLYNOMIAL 0x04c11db7
#define CRC32_SEED 0x12345678

uint8_t pico8_ram[RAM_SIZE];
static uint32_t crc32_table[256];
static uint32_t palette_map[16];

static SDL_Texture* screen;
static SDL_PixelFormat screen_format;

bool init_memory(SDL_Renderer* renderer)
{
    if (!renderer)
    {
        return false;
    }

    SDL_memset(&pico8_ram, 0x00, RAM_SIZE);

    screen_format = SDL_PIXELFORMAT_UNKNOWN;

    const SDL_PixelFormat *texture_formats = (const SDL_PixelFormat *)SDL_GetPointerProperty(SDL_GetRendererProperties(renderer), SDL_PROP_RENDERER_TEXTURE_FORMATS_POINTER, NULL);
    if (texture_formats) {
        // Use the first compatible format that the renderer supports
        for (int i = 0; i < texture_formats[i]; ++i) {
            if ((SDL_BYTESPERPIXEL(texture_formats[i]) != 1 &&
                 SDL_BYTESPERPIXEL(texture_formats[i]) != 2 &&
                 SDL_BYTESPERPIXEL(texture_formats[i]) != 4) ||
                !SDL_ISPIXELFORMAT_PACKED(texture_formats[i]))
                break;
            screen_format = texture_formats[i];
            break;
        }
    }

    // If all else fails, use RGBA32 as a fallback.
    if (screen_format == SDL_PIXELFORMAT_UNKNOWN)
        screen_format = SDL_PIXELFORMAT_RGBA32;

    screen = SDL_CreateTexture(renderer, screen_format, SDL_TEXTUREACCESS_STREAMING, SCREEN_SIZE, SCREEN_SIZE);
    if (screen == NULL)
    {
        SDL_Log("Could not create screen texture: %s", SDL_GetError());
        return false;
    }

    if (!SDL_SetTextureScaleMode(screen, SDL_SCALEMODE_NEAREST))
    {
        SDL_Log("Couldn't set texture scale mode: %s", SDL_GetError());
    }

    uint8_t r, g, b;
    const SDL_PixelFormatDetails *details = SDL_GetPixelFormatDetails(screen_format);

    for (int i = 0; i < 16; i++) {
        color_lookup(i, &r, &g, &b);
        palette_map[i] = SDL_MapRGB(details, NULL, r, g, b);
    }

    init_crc32();

    return true;
}

void reset_memory(void)
{
    SDL_memset(&pico8_ram, 0x00, RAM_SIZE);
    pico8_ram[0x5f25] = 0x06; // Default color, light gray.
}

void destroy_memory(void)
{
    if (screen)
    {
        SDL_DestroyTexture(screen);
    }
}

void update_from_virtual_memory(SDL_Renderer* renderer)
{
    /* Sprite sheet.
     * 0x0000  - 0x0fff
     *
     */


    /* Sprite sheet / Map, row 32 - 63 (shared)
     * 0x1000  - 0x1fff
     *
     */


    /* Map, row 0 - 31.
     * 0x2000  - 0x2fff
     *
     */


    /* Sprite flags.
     * 0x3000  - 0x30ff
     *
     */


    /* Music.
     * 0x3100  - 0x31ff
     *
     */


    /* Sound effects.
     * 0x3200  - 0x42ff
     *
     */


    /* General use or work RAM.
     * 0x4300  - 0x55ff
     *
     */


    /* General use / custom font.
     * 0x5600  - 0x5dff
     *
     */


    /* Persistent cart data.
     * 0x5e00  - 0x5eff
     *
     */


    /* Draw state.
     * 0x5f00  - 0x5f3f
     *
     */


    /* Hardware state.
     * 0x5f40  - 0x5f7f
     *
     */


    /* GPIO pins.
     * 0x5f80  - 0x5FFF
     *
     */


    /* Screen data.
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
        switch (SDL_BYTESPERPIXEL(screen_format))
        {
        case 1:
            for (int y = 0; y < SCREEN_SIZE; y++, row += pitch)
            {
                uint8_t* pixel = (uint8_t*)row;
                for (int x = 0; x < SCREEN_SIZE; x++, pixel++)
                {
                    uint8_t byte = pico8_ram[0x6000 + (y << 6) + (x >> 1)];
                    uint8_t color = (byte >> ((x & 1) << 2)) & 0xF;
                    *pixel = palette_map[color];
                }
            }
            break;
        case 2:
            for (int y = 0; y < SCREEN_SIZE; y++, row += pitch)
            {
                uint16_t* pixel = (uint16_t*)row;
                for (int x = 0; x < SCREEN_SIZE; x++, pixel++)
                {
                    uint8_t byte = pico8_ram[0x6000 + (y << 6) + (x >> 1)];
                    uint8_t color = (byte >> ((x & 1) << 2)) & 0xF;
                    *pixel = palette_map[color];
                }
            }
            break;
        case 4:
            for (int y = 0; y < SCREEN_SIZE; y++, row += pitch)
            {
                uint32_t* pixel = (uint32_t*)row;
                for (int x = 0; x < SCREEN_SIZE; x++, pixel++)
                {
                    uint8_t byte = pico8_ram[0x6000 + (y << 6) + (x >> 1)];
                    uint8_t color = (byte >> ((x & 1) << 2)) & 0xF;
                    *pixel = palette_map[color];
                }
            }
            break;
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

uint32_t crc32(const char* data, size_t start, size_t length)
{
    uint32_t crc = CRC32_SEED;

    for (size_t i = 0; i < length; ++i)
    {
        // Wrap around the array using modulus.
        size_t index = (start + i) % RAM_SIZE;
        uint8_t byte = (uint8_t)data[index];

        crc = (crc << 8) ^ crc32_table[((crc >> 24) ^ byte) & 0xFF];
    }

    return crc;
}

void init_crc32()
{
    for (uint32_t i = 0; i < 256; ++i)
    {
        uint32_t crc = i;
        for (uint32_t j = 8; j > 0; --j)
        {
            if (crc & 0x80000000)
            {
                crc = (crc << 1) ^ CRC32_POLYNOMIAL;
            }
            else
            {
                crc <<= 1;
            }
        }
        crc32_table[i] = crc;
    }
}
