/** @file api.c
 *
 *  A Pico-8 emulator for the Nokia N-Gage.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#include <SDL3/SDL.h>
#include <stdint.h>
#include "z8lua/lauxlib.h"
#include "z8lua/lua.h"
#include "sin_table.h"

#define TABLE_SIZE 4096
#define FIXED_SCALE 32767.0  // Scale factor for fixed-point values.

/***
* Auxiliary functions.
*/

// Lookup sine using a full circle range [0,1]
double sin_lookup(double x)
{
    if (x < 0.0 || x > 1.0)
    {
        x -= (int)x; // Ensure 0 <= x < 1 (handle periodicity).
    }

    double lookup_x = x * TABLE_SIZE * 2;  // Scale to full sine wave.
    int index = (int)lookup_x;
    double fraction = lookup_x - index;

    // Determine actual lookup index based on sine symmetry.
    if (index < TABLE_SIZE)
    {
        // First half (0 to 0.5).
        double y1 = SIN_TABLE[index] / FIXED_SCALE;
        double y2 = SIN_TABLE[index + 1] / FIXED_SCALE;
        return y1 + fraction * (y2 - y1);
    }
    else
    {
        // Second half (0.5 to 1.0): Use mirroring (sin(x) = -sin(1-x)).
        int mirrored_index = (2 * TABLE_SIZE - index);
        double y1 = -SIN_TABLE[mirrored_index] / FIXED_SCALE;
        double y2 = -SIN_TABLE[mirrored_index - 1] / FIXED_SCALE;
        return y1 + fraction * (y2 - y1);
    }
}

// Cosine lookup using phase shift.
double cos_lookup(double x)
{
    x += 0.25;
    if (x > 1.0)
    {
        x -= 1.0;
    }
    return sin_lookup(x);
}


#if 0
#define PI 3.141592653589793
static void generate_lookup()
{
    FILE *file = fopen("E:\\sin_table.h", "w+");
    if (!file)
    {
        SDL_Log("Error opening file!");
        return;
    }

    fprintf(file, "static const short SIN_TABLE[%d] = {\n", TABLE_SIZE);

    for (int i = 0; i < TABLE_SIZE; i++)
    {
        double angle = (double)i / TABLE_SIZE * PI; // Half-circle (0 to PI).
        short value = (short)(sin(angle) * 32767); // Scale to 16-bit fixed point.
        fprintf(file, "0x%04X%s", (unsigned short)value, (i < TABLE_SIZE - 1) ? ", " : "");
        if ((i + 1) % 8 == 0) fprintf(file, "\n");
    }

    fprintf(file, "};\n");
    fclose(file);

    SDL_Log("Lookup table generated in sin_table.h");
}
#endif

/***
 * Math functions.
 */
static int pico8_abs(lua_State* L)
{
    float value = (float)lua_tonumber(L, 1);
    if (value == 0x8000)
    {
        // PICO-8 0.2.3 changelog: abs(0x8000) should be 0x7fff.ffff
        lua_pushnumber(L, 32767.9999f);
    }
    else
    {
        lua_pushnumber(L, SDL_fabs(value));
    }
    return 1;
}

static int pico8_atan2(lua_State* L)
{
    return 0;
}

static int pico8_cos(lua_State* L)
{
    double angle = luaL_checknumber(L, 1);
    double cos = cos_lookup(angle);

    if (angle == 0.25)
    {
        lua_pushnumber(L, 0.0);
    }
    else
    {
        lua_pushnumber(L, cos);
    }

    return 1;
}

static int pico8_flr(lua_State* L)
{
    return 0;
}

static int pico8_max(lua_State* L)
{
    float a = (float)lua_tonumber(L, 1);
    float b = (float)lua_tonumber(L, 2);
    lua_pushnumber(L, a > b ? a : b);
    return 1;
}

static int pico8_mid(lua_State* L)
{
    return 0;
}

static int pico8_min(lua_State* L)
{
    float a = (float)lua_tonumber(L, 1);
    float b = (float)lua_tonumber(L, 2);
    lua_pushnumber(L, a < b ? a : b);
    return 1;
}

static int pico8_rnd(lua_State* L)
{
    return 0;
}

static int pico8_sgn(lua_State* L)
{
    double num = luaL_checknumber(L, 1);
    if (num >= 0)
    {
        lua_pushnumber(L, 1.0);
    }
    else
    {
        lua_pushnumber(L, -1.0);
    }
    return 1;
}

static int pico8_sin(lua_State* L)
{
    double angle = luaL_checknumber(L, 1);
    double sin = sin_lookup(angle);

    if (angle == 0.0 || angle == 1.0 || angle == 0.5)
    {
        lua_pushnumber(L, 0.0);
    }
    else
    {
        lua_pushnumber(L, -sin);
    }

    return 1;
}

static int pico8_sqrt(lua_State* L)
{
    double x = luaL_checknumber(L, 1);
    double root = 0.0;

    if (x > 0)
    {
        root = SDL_sqrt(x);
    }

    lua_pushnumber(L, (int)(root * 10000) / 10000.0);
    return 1;
}

static int pico8_srand(lua_State* L)
{
    return 0;
}

/***
 * Debug functions.
 */

static int pico8_SDL_log(lua_State* L)
{
    int n = lua_gettop(L);
    lua_getglobal(L, "tostring");
    for (int i = 1; i <= n; i++)
    {
        lua_pushvalue(L, -1);
        lua_pushvalue(L, i);
        lua_call(L, 1, 1);
        const char* s = lua_tostring(L, -1);
        if (s == NULL)
        {
            return luaL_error(L, "'tostring' must return a string to 'print'");
        }
        SDL_Log("%s", s);
        lua_pop(L, 1);
    }
    return 0;
}

void register_api(lua_State* L)
{
    // Math.
    lua_pushcfunction(L, pico8_abs);
    lua_setglobal(L, "abs");
    lua_pushcfunction(L, pico8_atan2);
    lua_setglobal(L, "atan2");
    lua_pushcfunction(L, pico8_cos);
    lua_setglobal(L, "cos");
    lua_pushcfunction(L, pico8_flr);
    lua_setglobal(L, "flr");
    lua_pushcfunction(L, pico8_max);
    lua_setglobal(L, "max");
    lua_pushcfunction(L, pico8_mid);
    lua_setglobal(L, "mid");
    lua_pushcfunction(L, pico8_min);
    lua_setglobal(L, "min");
    lua_pushcfunction(L, pico8_rnd);
    lua_setglobal(L, "rnd");
    lua_pushcfunction(L, pico8_sgn);
    lua_setglobal(L, "sgn");
    lua_pushcfunction(L, pico8_sin);
    lua_setglobal(L, "sin");
    lua_pushcfunction(L, pico8_sqrt);
    lua_setglobal(L, "sqrt");
    lua_pushcfunction(L, pico8_srand);
    lua_setglobal(L, "srand");

    // Debug.
    lua_pushcfunction(L, pico8_SDL_log);
    lua_setglobal(L, "SDL_log");
}
