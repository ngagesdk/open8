/** @file config.h
 *
 *  A portable PICO-8 emulator written in C.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#ifndef CONFIG_H
#define CONFIG_H

#include "SDL3/SDL.h"

#define SCREEN_SIZE 128

#define WINDOW_W 176
#define WINDOW_H 208

#define SCREEN_OFFSET_X 24
#define SCREEN_OFFSET_Y 25

#ifdef __SYMBIAN32__
#define SCALE 1
#define WINDOW_FLAGS
#else
#define SCALE 3
#define WINDOW_FLAGS (SDL_WINDOW_UTILITY | SDL_WINDOW_ALWAYS_ON_TOP)
#endif

#endif // CONFIG_H
