/** @file auxiliary.h
 *
 *  A portable PICO-8 emulator written in C.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#ifndef AUXILIARY_H
#define AUXILIARY_H

#include <stdint.h>

void color_lookup(int col, uint8_t* r, uint8_t* g, uint8_t* b);

#endif // AUXILIARY_H
