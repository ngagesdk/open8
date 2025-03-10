/** @file memory.h
 *
 *  A portable PICO-8 emulator written in C.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#ifndef MEMORY_H
#define MEMORY_H

#include <SDL3/SDL.h>
#include <stdint.h>

#define RAM_SIZE 0x8000

extern uint8_t pico8_ram[RAM_SIZE];

bool init_memory(SDL_Renderer* renderer);
void reset_memory(void);
void destroy_memory(void);
void update_from_virtual_memory(SDL_Renderer* renderer);

#endif // MEMORY_H
