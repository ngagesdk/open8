/** @file core.h
 *
 *  A portable PICO-8 emulator written in C.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#ifndef CORE_H
#define CORE_H

#include <SDL3/SDL.h>
#include <stdint.h>

#define CART_WIDTH 160
#define CART_HEIGHT 205
#define CART_DATA_SIZE 0x8020

#define MAX_CODE_SIZE 65536

typedef struct cart
{
    SDL_Texture* image;
    uint8_t* data;
    uint32_t size;

    uint8_t cart_data[0x8020];
    uint8_t* code;
    uint32_t code_size;

    bool is_corrupt;

} cart_t;

typedef enum state
{
    STATE_MENU,
    STATE_EMULATOR

} state_t;

bool init_core(SDL_Renderer* renderer);
void destroy_core(void);
bool handle_events(SDL_Renderer* renderer, SDL_Event* event);
bool iterate_emulator(SDL_Renderer* renderer);

#endif // CORE_H
