#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdlib.h>
#include "z8lua/lua.h"
#include "z8lua/lualib.h"
#include "api.h"
#include "Windows.h"

static SDL_Renderer* renderer = NULL;

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
    SDL_Window* window = NULL;
    float scale = 2.f;

    if (! SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }
    if (! SDL_CreateWindowAndRenderer("Pico-8", 128 * (int)scale, 128 * (int)scale, 0, &window, &renderer))
    {
        SDL_Log("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }
    SDL_SetRenderScale(renderer, scale, scale);

    lua_State* vm = luaL_newstate();
    if (!vm)
    {
        SDL_Log("Couldn't create Lua state.");
        return EXIT_FAILURE;
    }
    luaL_openlibs(vm);
    register_api(vm, renderer);

    if (luaL_loadfile(vm, "graphics.p8") || lua_pcall(vm, 0, 1, 0))
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

    SDL_RenderPresent(renderer);

    bool running = true;
    while (running)
    {
        SDL_Event event;
        SDL_PollEvent(&event);
        switch (event.type)
        {
            case SDL_EVENT_QUIT:
            case SDL_EVENT_KEY_DOWN:
            {
                running = false;
                break;
            }
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_SUCCESS;
}
