/** @file app.c
 *
 *  A portable PICO-8 emulator written in C.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#include "SDL3/SDL.h"
#ifdef __SYMBIAN32__
#include <SDL3_mixer/SDL_mixer.h>
#endif
#include "app.h"
#include "config.h"

bool init_app(SDL_Renderer** renderer, SDL_Window* window)
{
    SDL_SetHint("SDL_RENDER_VSYNC", "1");
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_INFO);
    SDL_SetAppMetadata("Pico-8", "1.0", "com.open8.ngagesdk");

#ifdef __SYMBIAN32__
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
#else
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK))
#endif
    {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return false;
    }

    window = SDL_CreateWindow("open8", WINDOW_W * SCALE, WINDOW_H * SCALE, WINDOW_FLAGS);
    if (!window)
    {
        SDL_Log("Couldn't create window: %s", SDL_GetError());
        return false;
    }

    *renderer = SDL_CreateRenderer(window, 0);
    if (!*renderer)
    {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return false;
    }

#if SCALE > 1
    if (!SDL_SetRenderScale(*renderer, SCALE, SCALE))
    {
        SDL_Log("Could not apply drawing scale factor: %s", SDL_GetError());
        return false;
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

    return true;
}

void destroy_app(void)
{
#ifdef __SYMBIAN32__
    Mix_CloseAudio();
    Mix_Quit();
#endif
}
