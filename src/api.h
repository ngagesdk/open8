/** @file api.h
 *
 *  A Pico-8 emulator for the Nokia N-Gage.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#ifndef API_H
#define API_H

#include "z8lua/lauxlib.h"
#include "z8lua/lua.h"

void register_api(lua_State* L);

#endif // API_H
