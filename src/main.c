/** @file core.c
 *
 *  A portable PICO-8 emulator written in C.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>

#ifdef __SYMBIAN32__
#include <SDL3_mixer/SDL_mixer.h>
#endif
#include "app.h"
#include "core.h"

static SDL_Window* window;
static SDL_Renderer* renderer;

// This function runs once at startup.
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
    if (!init_app(&(SDL_Renderer*)renderer, window))
    {
        return SDL_APP_FAILURE;
    }

    if (!init_core(renderer))
    {
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

// This function runs when a new event (Keypresses, etc) occurs.
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    if (!handle_events(renderer, event))
    {
        return SDL_APP_SUCCESS;
    }

    return SDL_APP_CONTINUE;
}

// This function runs once per frame, and is the heart of the program.
SDL_AppResult SDL_AppIterate(void* appstate)
{
    iterate_core(renderer);
    return SDL_APP_CONTINUE;
}

// This function runs once at shutdown.
void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    destroy_core();
    destroy_app();

    // SDL will clean up the window/renderer for us.
}
