/** @file api.c
 *
 *  A portable PICO-8 emulator written in C.
 *
 *  Copyright (c) 2025, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#include <SDL3/SDL.h>
#include <stdint.h>
#include <wchar.h>
#include "z8lua/lauxlib.h"
#include "z8lua/lua.h"
#include "z8lua/fix32.h"
#include "auxiliary.h"
#include "config.h"
#include "memory.h"

static SDL_Renderer* r;
static fix32_t seed_lo, seed_hi;

static uint8_t fill_mask[0x4000]; // Fill pattern mask.

fix32_t seconds_since_start;

 /************************
  * Auxiliary functions. *
  ************************/

static void pset(int x, int y, int* color)
{
    if ((unsigned)x >= 128 || (unsigned)y >= 128)
    {
        return;
    }

    uint32_t* fill_mask_ptr = (uint32_t*)fill_mask;

    // This needs to be optimized.
    if (*fill_mask_ptr)
    {
        int index = y * 128 + x;
        if (fill_mask[index])
        {
            return;
        }
    }

    uint8_t col;

    if (!color)
    {
        col = peek(0x5f25);
    }
    else
    {
        col = *color & 0x0F;
    }

    uint16_t addr = 0x6000 + (y << 6) + (x >> 1);
    uint8_t current_byte = peek(addr);

    if (x & 1)
    {
        // Right pixel (most-significant nybble).
        current_byte = (current_byte & 0x0F) | (col << 4);
    }
    else
    {
        // Left pixel (least-significant nybble).
        current_byte = (current_byte & 0xF0) | col;
    }

    poke(addr, current_byte);
}

static void draw_circle(int cx, int cy, int radius, int* color, bool fill)
{
    if (!r)
    {
        return;
    }

    int x = 0;
    int y = radius;
    int d = 3 - 2 * radius;

    while (x <= y)
    {
        if (fill)
        {
            for (int i = (cx - x); i <= (cx + x); i++)
            {
                pset(i, cy + y, color);
                pset(i, cy - y, color);
            }
            for (int i = (cx - y); i <= (cx + y); i++)
            {
                pset(i, cy + x, color);
                pset(i, cy - x, color);
            }
        }
        else
        {
            pset(cx + x, cy + y, color);
            pset(cx - x, cy + y, color);
            pset(cx + x, cy - y, color);
            pset(cx - x, cy - y, color);
            pset(cx + y, cy + x, color);
            pset(cx - y, cy + x, color);
            pset(cx + y, cy - x, color);
            pset(cx - y, cy - x, color);
        }

        if (d < 0)
        {
            d += 4 * x + 6;
        }
        else
        {
            d += 4 * (x - y) + 10;
            y--;
        }
        x++;
    }
}

static void draw_rect(int x0, int y0, int x1, int y1, int* color, bool fill)
{
   if (!r)
   {
       return;
   }

   if (fill)
   {
       for (int y = y0; y <= y1; y++)
       {
           for (int x = x0; x <= x1; x++)
           {
               pset(x, y, color);
           }
       }
   }
   else
   {
       for (int x = x0; x <= x1; x++)
       {
           pset(x, y0, color);
           pset(x, y1, color);
       }
       for (int y = y0; y <= y1; y++)
       {
           pset(x0, y, color);
           pset(x1, y, color);
       }
   }
}

 /***************************
  * Flow-control functions. *
  ***************************/

static int pico8_time(lua_State* L)
{
    lua_pushnumber(L, seconds_since_start);
    return 1;
}

 /***********************
  * Graphics functions. *
  ***********************/

static int pico8_camera(lua_State* L)
{
    return 0;
}

static int pico8_circ(lua_State* L)
{
    int cx = fix32_to_int(luaL_checknumber(L, 1));
    int cy = fix32_to_int(luaL_checknumber(L, 2));
    int radius = fix32_to_int(luaL_optnumber(L, 3, fix32_value(4, 0)));
    int color;

    if (lua_gettop(L) == 4)
    {
        color = fix32_to_int(luaL_checkinteger(L, 4));
        draw_circle(cx, cy, radius, &color, false);
    }
    else
    {
        draw_circle(cx, cy, radius, NULL, false);
    }

    return 0;
}

static int pico8_circfill(lua_State* L)
{
    int cx = fix32_to_int(luaL_checknumber(L, 1));
    int cy = fix32_to_int(luaL_checknumber(L, 2));
    int radius = fix32_to_int(luaL_optnumber(L, 3, fix32_value(4, 0)));
    int color;

    if (lua_gettop(L) == 4)
    {
        color = fix32_to_int(luaL_checkinteger(L, 4));
        draw_circle(cx, cy, radius, &color, true);
    }
    else
    {
        draw_circle(cx, cy, radius, NULL, true);
    }

    return 0;
}

static int pico8_clip(lua_State* L)
{
    // Reset region when calling cls().
    return 0;
}

// Todo: Reset clip region, reset cursor position to 0,0.
static int pico8_cls(lua_State* L)
{
    int color = fix32_to_int32(luaL_optinteger(L, 1, 0));
    uint8_t color_pair = (color & 0x0F) << 4 | (color & 0x0F);

    p8_memset(0x6000, color_pair, 0x2000);

    return 0;
}

static int pico8_color(lua_State* L)
{
    return 0;
}

static int pico8_cursor(lua_State* L)
{
    // Reset position when calling cls().
    return 0;
}

static int pico8_fget(lua_State* L)
{
    return 0;
}

static int pico8_fillp(lua_State* L)
{
    uint16_t pattern = fix32_to_uint16(luaL_optnumber(L, 1, 0));

#if 0
    // Predefined pattern values.
    // P8SCI, 128 - 135.
    switch (pattern)
    {
        case 0:
        case 0x25CB: // 128, Solid.
            pattern = 0x0000;
            break;
        case 0x2588: // 129, Checkerboard.
            pattern = 0x5A5A;
            break;
        case 0x1F431: // 130, Jelpi.
            pattern = 0x511F;
            break;
        case 0x2B07FE0F: // 131, Down key.
            pattern = 0x0003;
            break;
        case 0x2591: // 132, Dot pattern.
            pattern = 0x7070;
            break;
        case 0x273D: // 133, Throwing star.
            pattern = 0x8810;
            break;
        case 0x25CF: // 134, Ball.
            pattern = 0xF99F;
            break;
        case 0x2665: // 135, Heart.
            pattern = 0x51BF;
            break;
        default:
            break;
    }
#endif

    uint8_t high = (pattern & 0xFF00) >> 8;
    uint8_t low = pattern & 0x00FF;

    poke(0x5f31, high);
    poke(0x5f32, low);

    // Set fill pattern mask.
    for (int i = 0; i < 0x4000; i++)
    {
        int row = (i / 128) % 4;
        int col = (i % 128) % 4;

        uint8_t nibble = 0;
        switch (row)
        {
            case 0: nibble = (pattern & 0xF000) >> 12; break;
            case 1: nibble = (pattern & 0x0F00) >> 8;  break;
            case 2: nibble = (pattern & 0x00F0) >> 4;  break;
            case 3: nibble = (pattern & 0x000F);       break;
        }

        uint8_t pixel = (nibble >> (3 - col)) & 0x1;
        fill_mask[i] = pixel ? 0xFF : 0x00;
    }

    return 0;
}

static int pico8_flip(lua_State* L)
{
    return 0;
}

static int pico8_fset(lua_State* L)
{
    return 0;
}

static int pico8_line(lua_State* L)
{
    return 0;
}

static int pico8_oval(lua_State* L)
{
    return 0;
}

static int pico8_ovalfill(lua_State* L)
{
    return 0;
}

static int pico8_pal(lua_State* L)
{
    return 0;
}

static int pico8_palt(lua_State* L)
{
    return 0;
}

static int pico8_pget(lua_State* L)
{
    return 0;
}

static int pico8_print(lua_State* L)
{
    return 0;
}

static int pico8_pset(lua_State* L)
{
    int x = (int)fix32_to_int32(luaL_checknumber(L, 1));
    int y = (int)fix32_to_int32(luaL_checknumber(L, 2));

    if (lua_gettop(L) == 3)
    {
        int color = (int)fix32_to_int32(luaL_checkinteger(L, 3));
        pset(x, y, &color);
    }
    else
    {
        pset(x, y, NULL);
    }

    return 0;
}

static int pico8_rect(lua_State* L)
{
    int x0 = fix32_to_int(luaL_checknumber(L, 1));
    int y0 = fix32_to_int(luaL_checknumber(L, 2));
    int x1 = fix32_to_int(luaL_checknumber(L, 3));
    int y1 = fix32_to_int(luaL_checknumber(L, 4));

    if (lua_gettop(L) == 5)
    {
        int color = fix32_to_int(luaL_checkinteger(L, 5));
        draw_rect(x0, y0, x1, y1, &color, false);
    }
    else
    {
        draw_rect(x0, y0, x1, y1, NULL, false);
    }

    return 0;
}

static int pico8_rectfill(lua_State* L)
{
    int x0 = fix32_to_int(luaL_checknumber(L, 1));
    int y0 = fix32_to_int(luaL_checknumber(L, 2));
    int x1 = fix32_to_int(luaL_checknumber(L, 3));
    int y1 = fix32_to_int(luaL_checknumber(L, 4));

    if (lua_gettop(L) == 5)
    {
        int color = fix32_to_int(luaL_checkinteger(L, 5));
        draw_rect(x0, y0, x1, y1, &color, true);
    }
    else
    {
        draw_rect(x0, y0, x1, y1, NULL, true);
    }

    return 0;
}

static int pico8_sget(lua_State* L)
{
    return 0;
}

static int pico8_spr(lua_State* L)
{
    return 0;
}

static int pico8_sset(lua_State* L)
{
    return 0;
}

static int pico8_sspr(lua_State* L)
{
    return 0;
}

static int pico8_tline(lua_State* L)
{
    return 0;
}

/*******************
 * Math functions. *
 *******************/

// Special thanks to pancelor for documenting rnd() and srand()!
// https://www.lexaloffle.com/bbs/?pid=81103#p
// https://www.lexaloffle.com/bbs/?pid=153638#p

static fix32_t pico8_random(fix32_t limit)
{
    fix32_t result;
    seed_hi = fix32_rotl(seed_hi, 16);
    seed_hi += seed_lo;
    seed_lo += seed_hi;
    if (limit < 0)
    {
        if (limit <= seed_hi && seed_hi < 0)
        {
            return seed_hi - limit;
        }
        else
        {
            return seed_hi;
        }
    }
    else
    {
        if (seed_hi < 0)
        {
            result = fix32_mod(fix32_value(0x7fff, 0), limit) + fix32_value(1, 0);
        }
        else {
            result = 0;
        }
        result += fix32_mod(seed_hi & 0x7fffffff, limit);
        result = fix32_mod(result, limit);
        return result;
    }
}

static int pico8_rnd(lua_State* L)
{
    if (lua_istable(L, 1))
    {
        const fix32_t value = pico8_random(lua_rawlen(L, 1) << 8);
        lua_pushnumber(L, ((value >> 8) + 1) * fix32_value(1, 0));
        lua_gettable(L, -2);
    }
    else
    {
        fix32_t value = fix32_value(1, 0);
        if (lua_gettop(L) > 0)
        {
            value = lua_tonumberx(L, 1, NULL);
        }
        value = pico8_random(value);
        lua_pushnumber(L, value);
    }

    return 1;
}

static int pico8_srand(lua_State* L)
{
    fix32_t new_seed = luaL_checknumber(L, 1);
    if (new_seed == 0)
    {
        seed_hi = 0x60009755;
        seed_lo = 0xdeadbeef;
    }
    else
    {
        new_seed &= 0x7fffffff;
        seed_hi = new_seed ^ 0xbead29ba;
        seed_lo = new_seed;
    }
    for (int i = 0; i < 32; i++)
    {
        pico8_random(0);
    }
    return 0;
}

/*********************
 * Memory functions. *
 *********************/

static int pico8_peek(lua_State* L)
{
    uint16_t addr = fix32_to_uint16(luaL_checkunsigned(L, 1));
    unsigned int len = fix32_to_uint32(luaL_optunsigned(L, 2, fix32_value(1, 0)));

    if (len > RAM_SIZE - 1)
    {
        len = RAM_SIZE - 1;
    }

    for (unsigned int i = 0; i < len; i++)
    {
        uint8_t data = peek(addr + i);
        lua_pushnumber(L, fix32_from_uint8(data));
    }
    return len;
}

static int pico8_peek2(lua_State* L)
{
    uint16_t addr = fix32_to_uint16(luaL_checkunsigned(L, 1));
    unsigned int len = fix32_to_uint32(luaL_optunsigned(L, 2, fix32_value(1, 0)));

    if (len > RAM_SIZE - 1)
    {
        len = RAM_SIZE - 1;
    }

    for (unsigned int i = 0; i < len; i++)
    {
        uint16_t data = (uint16_t)peek(addr + i) << 8 | (uint16_t)peek(addr + i + 1);
        lua_pushnumber(L, fix32_from_uint16(data));
    }

    return len;
}

static int pico8_peek4(lua_State* L)
{
    uint16_t addr = fix32_to_uint16(luaL_checkunsigned(L, 1));
    unsigned int len = fix32_to_uint32(luaL_optunsigned(L, 2, fix32_value(1, 0)));

    if (len > RAM_SIZE - 1)
    {
        len = RAM_SIZE - 1;
    }

    for (unsigned int i = 0; i < len; i++)
    {
        uint32_t data = (uint32_t)peek(addr + i) << 24 | (uint32_t)peek(addr + i + 1) << 16 | (uint32_t)peek(addr + i + 2) << 8 | (uint32_t)peek(addr + i + 3);
        lua_pushnumber(L, fix32_from_uint32(data));
    }
    return len;
}

static int pico8_poke(lua_State* L)
{
    uint16_t addr = fix32_to_uint16(luaL_checkunsigned(L, 1));
    unsigned int num_args = lua_gettop(L);

    for (unsigned int i = 1; i < num_args; i++)
    {
        if (addr + i >= RAM_SIZE)
        {
            break;
        }
        uint8_t data = fix32_to_uint8(luaL_checkinteger(L, 1 + i));
        poke(addr + i, data);
    }

    return 0;
}

static int pico8_poke2(lua_State* L)
{
    uint16_t addr = fix32_to_uint16(luaL_checkunsigned(L, 1));
    unsigned int num_args = lua_gettop(L);

    for (unsigned int i = 2; i <= num_args; i++)
    {
        if (addr >= RAM_SIZE - 1)
        {
            break;
        }
        uint16_t data = fix32_to_uint16(luaL_checkinteger(L, i));
        poke(addr, (data >> 8) & 0xFF);
        poke(addr + 1, data & 0xFF);
        addr += 2;
    }

    return 0;
}

static int pico8_poke4(lua_State* L)
{
    uint16_t addr = fix32_to_uint16(luaL_checkunsigned(L, 1));
    unsigned int num_args = lua_gettop(L);

    for (unsigned int i = 2; i <= num_args; i++)
    {
        if (addr >= RAM_SIZE - 3)
        {
            break;
        }
        uint32_t data = fix32_to_uint32(luaL_checkinteger(L, i));
        poke(addr, (data >> 24) & 0xFF);
        poke(addr + 1, (data >> 16) & 0xFF);
        poke(addr + 2, (data >> 8) & 0xFF);
        poke(addr + 3, data & 0xFF);
        addr += 4;
    }

    return 0;
}

/********************
 * Table functions. *
 ********************/

static int pico8_add(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    int n = luaL_len(L, 1);
    int index = luaL_optinteger(L, 3, n + 1);
    luaL_argcheck(L, 1 <= index && index <= n + 1, 3, "index out of range");

    for (int i = n; i >= index; i--)
    {
        lua_rawgeti(L, 1, i);
        lua_rawseti(L, 1, i + 1);
    }

    lua_pushvalue(L, 2);
    lua_rawseti(L, 1, index);
    lua_settop(L, 2);
    return 1;
}

static int pico8_foreach(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    int n = luaL_len(L, 1);
    for (int i = 1; i <= n; i++)
    {
        lua_rawgeti(L, 1, i);
        lua_pushvalue(L, 2);
        lua_pushvalue(L, -3);
        lua_pushinteger(L, i);
        lua_call(L, 2, 0);
    }
    return 0;
}

/********************
 * Debug functions. *
 ********************/

static int pico8_log(lua_State* L)
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

void register_api(lua_State* L, SDL_Renderer* renderer)
{
    r = renderer;

    // Flow-control.
    lua_pushcfunction(L, pico8_time);
    lua_setglobal(L, "time");
    lua_pushcfunction(L, pico8_time);
    lua_setglobal(L, "t");

    // Graphics.
    lua_pushcfunction(L, pico8_camera);
    lua_setglobal(L, "camera");
    lua_pushcfunction(L, pico8_circ);
    lua_setglobal(L, "circ");
    lua_pushcfunction(L, pico8_circfill);
    lua_setglobal(L, "circfill");
    lua_pushcfunction(L, pico8_clip);
    lua_setglobal(L, "clip");
    lua_pushcfunction(L, pico8_cls);
    lua_setglobal(L, "cls");
    lua_pushcfunction(L, pico8_color);
    lua_setglobal(L, "color");
    lua_pushcfunction(L, pico8_cursor);
    lua_setglobal(L, "cursor");
    lua_pushcfunction(L, pico8_fget);
    lua_setglobal(L, "fget");
    lua_pushcfunction(L, pico8_fillp);
    lua_setglobal(L, "fillp");
    lua_pushcfunction(L, pico8_flip);
    lua_setglobal(L, "flip");
    lua_pushcfunction(L, pico8_fset);
    lua_setglobal(L, "fset");
    lua_pushcfunction(L, pico8_line);
    lua_setglobal(L, "line");
    lua_pushcfunction(L, pico8_oval);
    lua_setglobal(L, "oval");
    lua_pushcfunction(L, pico8_ovalfill);
    lua_setglobal(L, "ovalfill");
    lua_pushcfunction(L, pico8_pal);
    lua_setglobal(L, "pal");
    lua_pushcfunction(L, pico8_palt);
    lua_setglobal(L, "palt");
    lua_pushcfunction(L, pico8_pget);
    lua_setglobal(L, "pget");
    lua_pushcfunction(L, pico8_print);
    lua_setglobal(L, "print");
    lua_pushcfunction(L, pico8_pset);
    lua_setglobal(L, "pset");
    lua_pushcfunction(L, pico8_rect);
    lua_setglobal(L, "rect");
    lua_pushcfunction(L, pico8_rectfill);
    lua_setglobal(L, "rectfill");
    lua_pushcfunction(L, pico8_sget);
    lua_setglobal(L, "sget");
    lua_pushcfunction(L, pico8_spr);
    lua_setglobal(L, "spr");
    lua_pushcfunction(L, pico8_sset);
    lua_setglobal(L, "sset");
    lua_pushcfunction(L, pico8_sspr);
    lua_setglobal(L, "sspr");
    lua_pushcfunction(L, pico8_tline);
    lua_setglobal(L, "tline");

    // Math.
    static bool seed_initialized = false;
    if (!seed_initialized)
    {
        seed_lo = (fix32_t)SDL_GetPerformanceCounter();
        SDL_DelayNS(SDL_NS_PER_US);
        seed_hi = (fix32_t)SDL_GetPerformanceCounter();
        seed_initialized = true;
    }

    lua_pushcfunction(L, pico8_rnd);
    lua_setglobal(L, "rnd");
    lua_pushcfunction(L, pico8_srand);
    lua_setglobal(L, "srand");

    // Memory.
    lua_pushcfunction(L, pico8_peek);
    lua_setglobal(L, "peek");
    lua_pushcfunction(L, pico8_peek2);
    lua_setglobal(L, "peek2");
    lua_pushcfunction(L, pico8_peek4);
    lua_setglobal(L, "peek4");
    lua_pushcfunction(L, pico8_poke);
    lua_setglobal(L, "poke");
    lua_pushcfunction(L, pico8_poke2);
    lua_setglobal(L, "poke2");
    lua_pushcfunction(L, pico8_poke4);
    lua_setglobal(L, "poke4");

    // Tables.
    lua_pushcfunction(L, pico8_add);
    lua_setglobal(L, "add");
    lua_pushcfunction(L, pico8_foreach);
    lua_setglobal(L, "foreach");

    // Debug.
    lua_pushcfunction(L, pico8_log);
    lua_setglobal(L, "log");
}

#ifndef __SYMBIAN32__
#include <time.h>

void update_time(void)
{
    static clock_t start_time = 0;
    if (start_time == 0)
    {
        start_time = clock();
    }

    clock_t current_time = clock();
    double elapsed_time = (double)(current_time - start_time) / CLOCKS_PER_SEC;
    seconds_since_start = fix32_from_double(elapsed_time);
}
#endif
