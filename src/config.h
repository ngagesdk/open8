/** @file config.h
 *
 *  A Pico-8 emulator for the Nokia N-Gage.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#ifndef CONFIG_H
#define CONFIG_H

#define NGAGE_W 176
#define NGAGE_H 208

#define SCREEN_SIZE 128
#ifdef _WIN32
#define SCREEN_OFFSET_X 0
#define SCREEN_OFFSET_Y 0
#else
#define SCREEN_OFFSET_X 24
#define SCREEN_OFFSET_Y 25
#endif

#define RAM_SIZE 0x8000

#endif // CONFIG_H
