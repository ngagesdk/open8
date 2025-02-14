/** @file api.c
 *
 *  A Pico-8 emulator for the Nokia N-Gage.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#include <SDL3/SDL.h>
#include "z8lua/lauxlib.h"
#include "z8lua/lua.h"

int pico8_pset(lua_State* L)
{
    return 0;
}

void register_api(lua_State* L)
{
    lua_pushcfunction(L, pico8_pset);
    lua_setglobal(L, "pset");
}
