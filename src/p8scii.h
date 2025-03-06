/** @file p8scii.h
 *
 *  A portable PICO-8 emulator written in C.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#ifndef P8SCII_H
#define P8SCII_H

#include <stdint.h>

void blit_char_to_screen(uint8_t char_index, int x, int y, uint8_t color);

#endif // P8SCII_H
