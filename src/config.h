/** @file config.h
 *
 *  A portable PICO-8 emulator written in C.
 *
 *  Copyright (c) 2025-2026, Michael Fitzmayer. All rights reserved.
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
#elif defined __DOS__
#define SCALE 1
#define WINDOW_W 320
#define WINDOW_H 200
#define WINDOW_FLAGS 0
#define FRAME_OFFSET_X 96
#define FRAME_OFFSET_Y 36
#endif

#endif // CONFIG_H
