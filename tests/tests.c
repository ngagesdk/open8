﻿#include <SDL3/SDL.h>
#include <stdlib.h>
#include "z8lua/lua.h"
#include "z8lua/lualib.h"
#include "api.h"

static uint8_t ram[32768];

int main()
{
    lua_State* vm = luaL_newstate();
    if (!vm)
    {
        SDL_Log("Couldn't create Lua state.");
        return EXIT_FAILURE;
    }
    lua_setpico8memory(vm, ram);
    luaL_openlibs(vm);
    init_api(vm);

    if (luaL_loadfile(vm, "tests.lua") || lua_pcall(vm, 0, 1, 0))
    {
        SDL_Log("Lua error: %s", lua_tostring(vm, -1));
        lua_pop(vm, 1);
        return EXIT_FAILURE;
    }

    if (lua_isnumber(vm, -1))
    {
        int result = lua_tointeger(vm, -1);
        if (result)
        {
            lua_pop(vm, 1);
            lua_close(vm);
            return EXIT_FAILURE;
        }
    }
    else
    {
        SDL_Log("Lua script did not return a number.");
    }

    lua_pop(vm, 1);
    lua_close(vm);

    return EXIT_SUCCESS;
}
