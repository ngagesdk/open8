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

static int pico8_pset(lua_State* L);
static int pico8_SDL_log(lua_State* L);

void register_api(lua_State* L)
{
    lua_pushcfunction(L, pico8_pset);
    lua_setglobal(L, "pset");

    lua_pushcfunction(L, pico8_SDL_log);
    lua_setglobal(L, "SDL_log");
}

static int pico8_pset(lua_State* L)
{
    return 0;
}

static int pico8_SDL_log(lua_State* L)
{
    int n = lua_gettop(L);
    lua_getglobal(L, "tostring");
    for (int i = 1; i <= n; i++)
    {
        lua_pushvalue(L, -1);
        lua_pushvalue(L, i);
        lua_call(L, 1, 1);
        const char* s = lua_tostring(L, -1);
        if (s == NULL)
        {
            return luaL_error(L, "'tostring' must return a string to 'print'");
        }
        SDL_Log("%s", s);
        lua_pop(L, 1);
    }
    return 0;
}
