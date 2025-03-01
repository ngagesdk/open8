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

#include <stdint.h>

#define RAM_SIZE 0x8000

extern uint8_t pico8_ram[RAM_SIZE];

#endif // MEMORY_H
