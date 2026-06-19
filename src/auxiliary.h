/** @file auxiliary.h
 *
 *  A portable PICO-8 emulator written in C.
 *
 *  Copyright (c) 2025-2026, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#ifndef AUXILIARY_H
#define AUXILIARY_H

#include <SDL3/SDL.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct cart
{
    SDL_Texture* image;

    uint8_t cart_data[0x8020];
    uint8_t* code;
    uint32_t code_size;

    bool is_corrupt;

} cart_t;

void color_lookup(int col, uint8_t* r, uint8_t* g, uint8_t* b);
cart_t* get_cart(void);

#endif // AUXILIARY_H
