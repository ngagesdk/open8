/** @file emulator.h
 *
 *  A Pico-8 emulator for the Nokia N-Gage.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#ifndef EMULATOR_H
#define EMULATOR_H

#include <SDL3/SDL.h>

#define CART_WIDTH 160
#define CART_HEIGHT 205
#define CART_DATA_SIZE 0x8020

#define MAX_CODE_SIZE 65536

typedef struct cart
{
    SDL_Texture* image;
    Uint8* data;
    Uint32 size;

    Uint8 cart_data[0x8020];
    Uint8* code;
    Uint32 code_size;

    bool is_corrupt;

} cart_t;

typedef enum state
{
    STATE_MENU,
    STATE_EMULATOR

} state_t;

bool init_emulator(SDL_Renderer* renderer);
void destroy_emulator(void);
void select_next(SDL_Renderer* renderer);
void select_prev(SDL_Renderer* renderer);
void render_selection(SDL_Renderer* renderer);
bool run_script(SDL_Renderer* renderer, const char* file_name);
bool run_selection(SDL_Renderer* renderer);

bool handle_event(SDL_Renderer* renderer, SDL_Event* event);
bool iterate_emulator(SDL_Renderer* renderer);

#endif // EMULATOR_H
