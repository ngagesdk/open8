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


#ifdef __SYMBIAN32__
#define SCALE 1
#define WINDOW_W 176
#define WINDOW_H 208
#define WINDOW_FLAGS 0
#define FRAME_OFFSET_X 8
#define FRAME_OFFSET_Y 1
#elif defined __3DS__
#define SCALE 1
#define WINDOW_W 400
#define WINDOW_H 240
#define WINDOW_FLAGS 0
#define FRAME_OFFSET_X 120
#define FRAME_OFFSET_Y 17
#else
#define SCALE 3
#define WINDOW_W 176
#define WINDOW_H 208
#define WINDOW_FLAGS (SDL_WINDOW_UTILITY)
#define FRAME_OFFSET_X 8
#define FRAME_OFFSET_Y 1
#endif

#define FRAME_W 160
#define FRAME_H 205

#define SCREEN_OFFSET_X FRAME_OFFSET_X + 16
#define SCREEN_OFFSET_Y FRAME_OFFSET_Y + 24
#define SCREEN_SIZE 128

#endif // CONFIG_H
