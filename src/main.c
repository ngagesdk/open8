/** @file main.c
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
#include "config.h"
#include "emulator.h"

static SDL_Window* window;
static SDL_Renderer* renderer;

// This function runs once at startup.
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
    SDL_SetHint("SDL_RENDER_VSYNC", "1");
#ifndef __SYMBIAN32__
    SDL_SetHint("SDL_RENDER_DRIVER", "software");
#endif
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_INFO);
    SDL_SetAppMetadata("Pico-8", "1.0", "com.open8.ngagesdk");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_WindowFlags window_flags;
#ifdef __SYMBIAN32__
    window_flags = 0;
#else
    window_flags = SDL_WINDOW_UTILITY | SDL_WINDOW_ALWAYS_ON_TOP;
#endif

    window = SDL_CreateWindow("Pico-8", NGAGE_W * SCALE, NGAGE_H * SCALE, window_flags);
    if (!window)
    {
        SDL_Log("Couldn't create window: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    renderer = SDL_CreateRenderer(window, 0);
    if (!renderer)
    {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }


#if SCALE > 1
    if (!SDL_SetRenderScale(renderer, SCALE, SCALE))
    {
        SDL_Log("Could not apply drawing scale factor: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
#endif

#ifdef __SYMBIAN32__
    SDL_AudioSpec spec;
    spec.channels = 1;
    spec.format = SDL_AUDIO_S16;
    spec.freq = 8000;

    if (!Mix_OpenAudio(0, &spec))
    {
        SDL_Log("Mix_Init: %s", SDL_GetError());
    }
#endif

    if (!init_emulator(renderer))
    {
        SDL_Log("Couldn't initialize emulator.");
        return SDL_APP_FAILURE;
    }
    render_selection(renderer);
    SDL_RenderPresent(renderer);

    return SDL_APP_CONTINUE;
}

// This function runs when a new event (Keypresses, etc) occurs.
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    if (!handle_event(renderer, event))
    {
        return SDL_APP_SUCCESS;
    }

    return SDL_APP_CONTINUE;
}

// This function runs once per frame, and is the heart of the program.
SDL_AppResult SDL_AppIterate(void* appstate)
{
    iterate_emulator(renderer);
    return SDL_APP_CONTINUE;
}

// This function runs once at shutdown.
void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    destroy_emulator();
#ifdef __SYMBIAN32__
    Mix_CloseAudio();
    Mix_Quit();
#endif
    // SDL will clean up the window/renderer for us.
}
