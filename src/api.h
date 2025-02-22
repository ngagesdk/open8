/** @file api.h
 *
 *  A Pico-8 emulator for the Nokia N-Gage.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#ifndef API_H
#define API_H

#include <SDL3/SDL.h>
#include <stdint.h>
#include "z8lua/lauxlib.h"
#include "z8lua/lua.h"
#include  "config.h"

void register_api(lua_State* L, SDL_Renderer* renderer);
void reset_draw_state(SDL_Renderer* renderer);
void update_time(void);

extern Uint8 pico8_ram[RAM_SIZE];

#endif // API_H
