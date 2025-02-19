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
#include "atan2_table.h"
#include "sin_table.h"

#define FIXED_SCALE 32767.0 // Scale factor for fixed-point values.
#define M_PI 3.14159265358979323846

static Uint64 seed;

 /************************
 * Auxiliary functions. *
 ************************/

// Lookup sine using a full circle range [0,1]
double sin_lookup(double x)
{
    if (x < 0.0 || x > 1.0)
    {
        x -= (int)x; // Ensure 0 <= x < 1 (handle periodicity).
    }

    double lookup_x = x * SIN_TABLE_SIZE * 2;  // Scale to full sine wave.
    int index = (int)lookup_x;
    double fraction = lookup_x - index;

    // Determine actual lookup index based on sine symmetry.
    if (index < SIN_TABLE_SIZE)
    {
        // First half (0 to 0.5).
        double y1 = SIN_TABLE[index] / FIXED_SCALE;
        double y2 = SIN_TABLE[index + 1] / FIXED_SCALE;
        return y1 + fraction * (y2 - y1);
    }
    else
    {
        // Second half (0.5 to 1.0): Use mirroring (sin(x) = -sin(1-x)).
        int mirrored_index = (2 * SIN_TABLE_SIZE - index);
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

double atan2_lookup(double dy, double dx)
{
    if (dy == 0.0 && dx == 0.0)
    {
        return 0.25;
    }
    else if (dy > 0.0 && dx == 0.0)
    {
        return 0.0;
    }

    double angle = SDL_atan2(dy, dx);

    // Normalize the angle to the range [0, 1].
    double normalized_angle = angle + M_PI;
    normalized_angle *= 0.15915494309189535; // 1 / (2 * M_PI)

    // Phase shift to match the lookup table.
    normalized_angle += 0.25;
    if (normalized_angle > 1.0)
    {
        normalized_angle -= 1.0;
    }

    unsigned int index = (unsigned int)(normalized_angle * ATAN2_TABLE_SIZE);

    if (index >= ATAN2_TABLE_SIZE)
    {
        index = ATAN2_TABLE_SIZE - 1;
    }

    return ATAN2_TABLE[index];
}

#if 0
static void generate_sin_lookup()
{
    FILE *file = fopen("sin_table.h", "w+");
    if (!file)
    {
        SDL_Log("Error opening file!");
        return;
    }

    fprintf(file, "static const short SIN_TABLE[%d] = {\n", SIN_TABLE_SIZE);

    for (int i = 0; i < SIN_TABLE_SIZE; i++)
    {
        double angle = (double)i / SIN_TABLE_SIZE * M_PI; // Half-circle (0 to PI).
        short value = (short)(sin(angle) * 32767); // Scale to 16-bit fixed point.
        fprintf(file, "0x%04X%s", (unsigned short)value, (i < SIN_TABLE_SIZE - 1) ? ", " : "");
        if ((i + 1) % 8 == 0) fprintf(file, "\n");
    }

    fprintf(file, "};\n");
    fclose(file);

    SDL_Log("Lookup table generated in sin_table.h");
}
#endif

/*******************
 * Math functions. *
 *******************/

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
    double dy = luaL_checknumber(L, 1);
    double dx = luaL_checknumber(L, 2);
    double result = atan2_lookup(dy, dx);
    lua_pushnumber(L, result);
    return 1;
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
        double rounded_cos = ((int)(cos * 10000.0 + (cos >= 0 ? 0.5 : -0.5))) / 10000.0;
        lua_pushnumber(L, rounded_cos);
    }

    return 1;
}

static int pico8_flr(lua_State* L)
{
    double value = SDL_floor(lua_tonumber(L, 1));
    lua_pushnumber(L, value);
    return 1;
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
    double first = lua_tonumber(L, 1);
    double second = lua_tonumber(L, 2);
    double third = lua_tonumber(L, 3);

    if (first > second)
    {
        double temp = first;
        first = second;
        second = temp;
    }
    if (second > third)
    {
        double temp = second;
        second = third;
        third = temp;
    }
    if (first > second)
    {
        double temp = first;
        first = second;
        second = temp;
    }
    lua_pushnumber(L, second);

    return 1;
}

static int pico8_min(lua_State* L)
{
    float a = (float)lua_tonumber(L, 1);
    float b = (float)lua_tonumber(L, 2);
    lua_pushnumber(L, a < b ? a : b);
    return 1;
}

// This function does not return the same values as PICO-8's rnd() function (yet).
// Not sure how important this is, but it might be worth looking into.
static int pico8_rnd(lua_State* L)
{
    if (lua_istable(L, 1))
    {
        int len = luaL_len(L, 1);
        if (len == 0)
        {
            lua_pushnil(L);
        }
        else
        {
            int index = SDL_rand(0xb00b) % len + 1;
            lua_rawgeti(L, 1, index);
        }
    }
    else
    {
        double limit = luaL_optnumber(L, 1, 1.0);

        if (limit <= 0.0)
        {
            lua_pushnumber(L, 0.0);
        }
        else
        {
            if (limit == 1.0) // Always returns 0.0, why?
            {
                limit = 0.9999;
            }

            double result = SDL_randf() * (limit - 0.0001);
            double rounded_result = ((int)(result * 10000.0 + (result >= 0 ? 0.5 : -0.5))) / 10000.0;
            lua_pushnumber(L, rounded_result);
        }
    }

    return 1;
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
        double rounded_sin = ((int)(sin * 10000.0 + (sin >= 0 ? 0.5 : -0.5))) / 10000.0;
        lua_pushnumber(L, -rounded_sin);
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
    double new_seed = luaL_checknumber(L, 1);
    memcpy(&seed, &new_seed, sizeof(seed));
    if (seed == 0)
    {
        seed = 1;
    }
    SDL_srand(seed);
    return 0;
}

/********************
 * Debug functions. *
 ********************/

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

/*********************
 * API Registration. *
 *********************/

void register_api(lua_State* L)
{
    // Math.
    static bool seed_initialized = false;
    if (!seed_initialized)
    {
        seed = SDL_GetPerformanceCounter();
        seed_initialized = true;
        SDL_srand(seed);
    }

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
