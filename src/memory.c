/** @file memory.c
 *
 *  A portable PICO-8 emulator written in C.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#include "memory.h"

#include <stdint.h>

uint8_t pico8_ram[RAM_SIZE];
