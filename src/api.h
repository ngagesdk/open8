/* @file api.h
 *
 * A Pico-8 emulator for the Nokia N-Gage.
 */

#ifndef API_H
#define API_H

#include "z8lua/lua.h"

void register_api(lua_State* L);

#endif // API_H
