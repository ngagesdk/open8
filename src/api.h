/** @file api.h
 *
 *  A portable PICO-8 emulator written in C.
 *
 *  Copyright (c) 2025-2026, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#ifndef API_H
#define API_H

#include <stdint.h>

#include "z8lua/fix32.h"
#include "z8lua/lauxlib.h"
#include "z8lua/lua.h"

extern fix32_t seconds_since_start;

// Frame timing information (set by core.c each frame) used by stat(1).
extern uint64_t pico8_frame_start;
extern uint32_t pico8_frame_ms;

void init_api(lua_State* L);
void update_input(void);
void update_time(void);

#endif // API_H
