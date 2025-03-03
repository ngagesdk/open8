/** @file api.h
 *
 *  A portable PICO-8 emulator written in C.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#ifndef API_H
#define API_H

#include "z8lua/fix32.h"
#include "z8lua/lauxlib.h"
#include "z8lua/lua.h"
#include  "config.h"

extern fix32_t seconds_since_start;

void init_api(lua_State* L, SDL_Renderer* renderer);
void destroy_api(void);
void update_time(void);

#endif // API_H
