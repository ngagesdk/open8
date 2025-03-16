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
#include "p8scii.h"

#include <stdbool.h>

#define TO_BE_DONE \
    static bool warning_printed = false; \
    if (!warning_printed) { \
        SDL_LogWarn(0, "%s not yet implemented.", __func__); \
        warning_printed = true; \
    } \
    return 0

static fix32_t seed_lo, seed_hi;

fix32_t seconds_since_start;

// Auxiliary functions.

static void pset(int x, int y, int* color)
{
    if (((unsigned)x | (unsigned)y) >= 128)
    {
        return;
    }

    uint16_t pattern = (pico8_ram[0x5f31] << 8) | pico8_ram[0x5f32];
    int row_shift = 12 - ((y & 3) << 2); // Precompute shift amount.
    uint8_t nibble = (pattern >> row_shift) & 0x0F;

    if (nibble & (1 << (3 - (x & 3))))
    {
        return;
    }

    uint8_t p8_color = color ? (*color & 0x0F) : pico8_ram[0x5f25];
    uint16_t addr = 0x6000 + (y << 6) + (x >> 1);
    uint8_t mask = (x & 1) ? 0x0F : 0xF0;
    uint8_t shift = (x & 1) ? 4 : 0;
    pico8_ram[addr] = (pico8_ram[addr] & mask) | (p8_color << shift);
}

static uint8_t pget(int x, int y)
{
    if (((unsigned)x | (unsigned)y) >= 128)
    {
        return 0;
    }
    uint16_t addr = 0x6000 + (y << 6) + (x >> 1);
    uint8_t shift = (x & 1) ? 4 : 0;
    uint8_t p8_color = (pico8_ram[addr] >> shift) & 0x0F;
    return p8_color;
}

static void draw_circle(int cx, int cy, int radius, int* color, bool fill)
{
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

static void draw_line(int x0, int y0, int x1, int y1, int* color)
{
   int dx = abs(x1 - x0);
   int dy = abs(y1 - y0);
   int sx = (x0 < x1) ? 1 : -1;
   int sy = (y0 < y1) ? 1 : -1;
   int err = dx - dy;

   while (true)
   {
       pset(x0, y0, color);

       if (x0 == x1 && y0 == y1)
       {
           break;
       }

       int e2 = 2 * err;
       if (e2 > -dy)
       {
           err -= dy;
           x0 += sx;
       }
       if (e2 < dx)
       {
           err += dx;
           y0 += sy;
       }
   }
}

static void draw_oval(int x0, int y0, int x1, int y1, int* color, bool fill)
{
    int a = abs(x1 - x0) / 2;
    int b = abs(y1 - y0) / 2;
    int xc = (x0 + x1) / 2;
    int yc = (y0 + y1) / 2;

    int a2 = a * a;
    int b2 = b * b;
    int fa2 = 4 * a2, fb2 = 4 * b2;
    int x = 0, y = b;
    int sigma = 2 * b2 + a2 * (1 - 2 * b);

    // First half.
    while (b2 * x <= a2 * y)
    {
        if (fill)
        {
            for (int i = xc - x; i <= xc + x; i++)
            {
                pset(i, yc + y, color);
                pset(i, yc - y, color);
            }
        }
        else
        {
            pset(xc + x, yc + y, color);
            pset(xc - x, yc + y, color);
            pset(xc + x, yc - y, color);
            pset(xc - x, yc - y, color);
        }

        if (sigma >= 0)
        {
            sigma += fa2 * (1 - y);
            y--;
        }
        sigma += b2 * (4 * x + 6);
        x++;
    }

    // Second half.
    x = a;
    y = 0;
    sigma = 2 * a2 + b2 * (1 - 2 * a);
    while (a2 * y <= b2 * x)
    {
        if (fill)
        {
            for (int i = xc - x; i <= xc + x; i++)
            {
                pset(i, yc + y, color);
                pset(i, yc - y, color);
            }
        }
        else
        {
            pset(xc + x, yc + y, color);
            pset(xc - x, yc + y, color);
            pset(xc + x, yc - y, color);
            pset(xc - x, yc - y, color);
        }

        if (sigma >= 0)
        {
            sigma += fb2 * (1 - x);
            x--;
        }
        sigma += a2 * (4 * y + 6);
        y++;
    }
}

static void draw_rect(int x0, int y0, int x1, int y1, int* color, bool fill)
{
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

#if 0
static int utf8_decode(const char *s, int *index)
{
    unsigned char c = s[*index];
    int codepoint = 0;
    int num_bytes = 0;

    if ((c & 0x80) == 0)
    {
        // 1-byte (ASCII).
        codepoint = c;
        num_bytes = 1;
    }
    else if ((c & 0xE0) == 0xC0)
    {
        // 2-byte.
        codepoint = (s[*index] & 0x1F) << 6 | (s[*index + 1] & 0x3F);
        num_bytes = 2;
    }
    else if ((c & 0xF0) == 0xE0)
    {
        // 3-byte.
        codepoint = (s[*index] & 0x0F) << 12 | (s[*index + 1] & 0x3F) << 6 | (s[*index + 2] & 0x3F);
        num_bytes = 3;
    }
    else if ((c & 0xF8) == 0xF0)
    {
        // 4-byte.
        codepoint = (s[*index] & 0x07) << 18 | (s[*index + 1] & 0x3F) << 12 |
                    (s[*index + 2] & 0x3F) << 6 | (s[*index + 3] & 0x3F);
        num_bytes = 4;
    }
    else
    {
        // Invalid UTF-8.
        codepoint = '?'; // Replace with '?'
        num_bytes = 1;
    }

    *index += num_bytes - 1; // Move index forward by extra bytes processed.
    return codepoint;
}
#endif

// Audio functions.

static int pico8_music(lua_State* L)
{
    TO_BE_DONE;
}

static int pico8_sfx(lua_State* L)
{
    TO_BE_DONE;
}

// Cart data functions.

static int pico8_cartdata(lua_State* L)
{
    TO_BE_DONE;
}

static int pico8_dget(lua_State* L)
{
    TO_BE_DONE;
}

static int pico8_dset(lua_State* L)
{
    TO_BE_DONE;
}

// Flow-control functions.

static int pico8_time(lua_State* L)
{
    lua_pushnumber(L, seconds_since_start);
    return 1;
}

// Graphics functions.

static int pico8_camera(lua_State* L)
{
    TO_BE_DONE;
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
    TO_BE_DONE;
}

static int pico8_cls(lua_State* L)
{
    int color = fix32_to_int32(luaL_optinteger(L, 1, 0));
    uint8_t color_pair = (color & 0x0F) << 4 | (color & 0x0F);

    SDL_memset(&pico8_ram[0x6000], color_pair, 0x2000);

    // Clear clip rectangle.

    // tbd.

    // Reset cursor position.
    pico8_ram[0x5f26] = 0x00;
    pico8_ram[0x5f27] = 0x00;

    return 0;
}

static int pico8_color(lua_State* L)
{
    pico8_ram[0x5f25] = fix32_to_uint8(luaL_optinteger(L, 1, 6));
    return 0;
}

static int pico8_cursor(lua_State* L)
{
    pico8_ram[0x5f26] = fix32_to_uint8(luaL_checkunsigned(L, 1)); // X.
    pico8_ram[0x5f27] = fix32_to_uint8(luaL_checkunsigned(L, 2)); // Y.

    if (lua_gettop(L) == 3)
    {
        pico8_ram[0x5f25] = fix32_to_uint8(luaL_optinteger(L, 3, 0));
    }

    return 0;
}

static int pico8_fget(lua_State* L)
{
    TO_BE_DONE;
}

static int pico8_fillp(lua_State* L)
{
    uint32_t pattern = 0;

    if (lua_type(L, 1) == LUA_TNUMBER)
    {
        pattern = fix32_to_uint32(luaL_optnumber(L, 1, 0));
    }
    else if (lua_type(L, 1) == LUA_TSTRING)
    {
        const char* str = luaL_checkstring(L, 1);
        uint8_t fillc = str[0];

        switch (fillc)
        {
            default:
            case 128: // Solid.
                pattern = 0b0000000000000000;
                break;
            case 129: // Checkerboard.
                pattern = 0b0101101001011010;
                break;
            case 130: // Jelpi.
                pattern = 0b0101000100011111;
                break;
            case 131: // Down key.
                pattern = 0b0000000000000011;
                break;
            case 132: // Dot pattern.
                pattern = 0b0111110101111101;
                break;
            case 133: // Throwing star.
                pattern = 0b1011100000011101;
                break;
            case 134: // Ball.
                pattern = 0b1111100110011111;
                break;
            case 135: // Heart.
                pattern = 0b0101000110111111;
                break;
        }
    }

    pico8_ram[0x5f31] = (pattern & 0xFF00) >> 8;
    pico8_ram[0x5f32] = pattern & 0x00FF;

    return 0;
}

static int pico8_flip(lua_State* L)
{
    // We might not need this function.
    return 0;
}

static int pico8_fset(lua_State* L)
{
    TO_BE_DONE;
}

static int pico8_line(lua_State* L)
{
    int x0 = fix32_to_int(luaL_checknumber(L, 1));
    int y0 = fix32_to_int(luaL_checknumber(L, 2));
    int x1 = fix32_to_int(luaL_checknumber(L, 3));
    int y1 = fix32_to_int(luaL_checknumber(L, 4));
    int color;
    if (lua_gettop(L) == 5)
    {
        color = fix32_to_int(luaL_checkinteger(L, 5));
        draw_line(x0, y0, x1, y1, &color);
    }
    else
    {
        draw_line(x0, y0, x1, y1, NULL);
    }
    return 0;
}

static int pico8_oval(lua_State* L)
{
    int x0 = fix32_to_int(luaL_checknumber(L, 1));
    int y0 = fix32_to_int(luaL_checknumber(L, 2));
    int x1 = fix32_to_int(luaL_checknumber(L, 3));
    int y1 = fix32_to_int(luaL_checknumber(L, 4));
    int color;
    if (lua_gettop(L) == 5)
    {
        color = fix32_to_int(luaL_checkinteger(L, 5));
        draw_oval(x0, y0, x1, y1, &color, false);
    }
    else
    {
        draw_oval(x0, y0, x1, y1, NULL, false);
    }
    return 0;
}

static int pico8_ovalfill(lua_State* L)
{
    int x0 = fix32_to_int(luaL_checknumber(L, 1));
    int y0 = fix32_to_int(luaL_checknumber(L, 2));
    int x1 = fix32_to_int(luaL_checknumber(L, 3));
    int y1 = fix32_to_int(luaL_checknumber(L, 4));
    int color;
    if (lua_gettop(L) == 5)
    {
        color = fix32_to_int(luaL_checkinteger(L, 5));
        draw_oval(x0, y0, x1, y1, &color, true);
    }
    else
    {
        draw_oval(x0, y0, x1, y1, NULL, true);
    }
    return 0;
}

static int pico8_pal(lua_State* L)
{
    TO_BE_DONE;
}

static int pico8_palt(lua_State* L)
{
    TO_BE_DONE;
}

static int pico8_pget(lua_State* L)
{
    int x = (int)fix32_to_int32(luaL_checknumber(L, 1));
    int y = (int)fix32_to_int32(luaL_checknumber(L, 2));

    lua_pushinteger(L, pget(x, y));
    return 1;
}

static int pico8_print(lua_State* L)
{
    const char* text = luaL_checkstring(L, 1);
    int len = SDL_strlen(text);
    int argc = lua_gettop(L);
    uint8_t cursor_x = pico8_ram[0x5f26];
    uint8_t x_cursor = cursor_x;
    uint8_t cursor_y = pico8_ram[0x5f27];
    uint8_t color = pico8_ram[0x5f25];

    if (argc == 4)
    {
        cursor_x = fix32_to_uint8(luaL_checkunsigned(L, 2));
        cursor_y = fix32_to_uint8(luaL_checkunsigned(L, 3));
        color = fix32_to_uint8(luaL_checkunsigned(L, 4));
    }
    else if (argc == 3)
    {
        cursor_x = fix32_to_uint8(luaL_checkunsigned(L, 2));
        cursor_y = fix32_to_uint8(luaL_checkunsigned(L, 3));
    }
    else if (argc == 2)
    {
        cursor_x = fix32_to_uint8(luaL_checkunsigned(L, 2));
    }

    for (int i = 0; text[i] != '\0'; i++)
    {
        if (text[i] == '\t') // 9, tab.
        {
            cursor_x += 16;
            continue;
        }
        else if (text[i] == '\n') // 10, newline.
        {
            cursor_x = 0;
            cursor_y += 6;
            continue;
        }
        else if (text[1] == '\b') // 8, backspace.
        {
            cursor_x -= 4;
            continue;
        }
        else if (text[i] == '\r') // 13, carriage return.
        {
            cursor_x = 0;
            continue;
        }

        uint8_t w, h;
        blit_char_to_screen(text[i], cursor_x, cursor_y, color, &w, &h);
        cursor_x += (w == 3) ? w + 1 : w - 1;
    }

    cursor_y += 6;

    pico8_ram[0x5f26] = x_cursor;
    pico8_ram[0x5f27] = cursor_y;
    lua_pushnumber(L, cursor_x);

    return 1;
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
    TO_BE_DONE;
}

static int pico8_spr(lua_State* L)
{
    uint8_t n = fix32_to_uint8(luaL_checkunsigned(L, 1));
    int32_t x = fix32_to_int32(luaL_optunsigned(L, 2, 0));
    int32_t y = fix32_to_int32(luaL_optunsigned(L, 3, 0));
    uint8_t w = luaL_optunsigned(L, 4, 1);
    uint8_t h = luaL_optunsigned(L, 5, 1);
    bool flip_x = lua_toboolean(L, 6);
    bool flip_y = lua_toboolean(L, 7);

    uint8_t width = w * 8;
    uint8_t height = h * 8;

    uint16_t sprite_x_base = (n & 0xF) << 2;
    uint16_t sprite_y_base = (n >> 4) * 512;

    for (uint8_t dy = 0; dy < height; dy++)
    {
        uint8_t sy = flip_y ? (height - 1 - dy) : dy;
        uint16_t sprite_row_addr = sprite_y_base + (sy << 6);

        for (uint8_t dx = 0; dx < width; dx++)
        {
            uint8_t sx = flip_x ? (width - 1 - dx) : dx;
            uint16_t sprite_addr = sprite_row_addr + sprite_x_base + (sx >> 1);
            uint8_t byte = pico8_ram[sprite_addr];

            uint8_t color = (sx & 1) ? (byte >> 4) : (byte & 0x0F);

            if (color)
            {
                uint16_t screen_addr = 0x6000 + ((y + dy) << 6) + ((x + dx) >> 1);
                uint8_t *screen_byte = &pico8_ram[screen_addr];

                if ((x + dx) & 1)
                {
                    *screen_byte = (*screen_byte & 0x0F) | (color << 4);
                }
                else
                {
                    *screen_byte = (*screen_byte & 0xF0) | color;
                }
            }
        }
    }

    return 0;
}

static int pico8_sset(lua_State* L)
{
    TO_BE_DONE;
}

static int pico8_sspr(lua_State* L)
{
    TO_BE_DONE;
}

static int pico8_tline(lua_State* L)
{
    TO_BE_DONE;
}

// Map functions.

static int pico8_map(lua_State* L)
{
    TO_BE_DONE;
}

static int pico8_mget(lua_State* L)
{
    TO_BE_DONE;
}

static int pico8_mset(lua_State* L)
{
    TO_BE_DONE;
}

static int pico8_mapdraw(lua_State* L)
{
    TO_BE_DONE;
}

// Input functions.

static int pico8_btn(lua_State* L)
{
    TO_BE_DONE;
}

static int pico8_btnp(lua_State* L)
{
    TO_BE_DONE;
}

// Math functions.

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

// Memory functions.

static int pico8_memcpy(lua_State* L)
{
    uint16_t dest_addr = fix32_to_uint16(luaL_checkunsigned(L, 1));
    uint16_t source_addr = fix32_to_uint16(luaL_checkunsigned(L, 2));
    uint32_t len = fix32_to_uint32(luaL_checkunsigned(L, 3));

    // Clamp length so that both source and destination stay within RAM_SIZE.
    if (source_addr >= RAM_SIZE)
    {
        len = 0; // Nothing to copy if source is out of bounds.
    }
    else if (source_addr + len > RAM_SIZE)
    {
        len = RAM_SIZE - source_addr;
    }
    if (dest_addr >= RAM_SIZE)
    {
        len = 0; // Nothing to copy if destination is out of bounds.
    }
    else if (dest_addr + len > RAM_SIZE)
    {
        len = RAM_SIZE - dest_addr;
    }
    if (len > 0)
    {
        SDL_memcpy(&pico8_ram[dest_addr], &pico8_ram[source_addr], len);
    }

    return 0;
}

static int pico8_memset(lua_State* L)
{
    uint16_t addr = fix32_to_uint16(luaL_checkunsigned(L, 1));
    uint8_t value = fix32_to_uint8(luaL_checkinteger(L, 2));
    uint32_t len = fix32_to_uint32(luaL_checkunsigned(L, 3));

    // Clamp length so that source stays within RAM_SIZE.
    if (addr >= RAM_SIZE)
    {
        len = 0; // Nothing to set if source is out of bounds.
    }
    else if (addr + len > RAM_SIZE)
    {
        len = RAM_SIZE - addr;
    }
    if (len > 0)
    {
        SDL_memset(&pico8_ram[addr], value, len);
    }

    return 0;
}

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
        uint8_t data = pico8_ram[addr + i];
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
        uint16_t data = (uint16_t)pico8_ram[addr + i] << 8 | (uint16_t)pico8_ram[addr + i + 1];
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
        uint32_t data = (uint32_t)pico8_ram[addr + i] << 24 | (uint32_t)pico8_ram[addr + i + 1] << 16 | (uint32_t)pico8_ram[addr + i + 2] << 8 | (uint32_t)pico8_ram[addr + i + 3];
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
        pico8_ram[addr + i] = data;
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
        pico8_ram[addr] = (data >> 8) & 0xFF;
        pico8_ram[addr + 1] = data & 0xFF;
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
        pico8_ram[addr] = (data >> 24) & 0xFF;
        pico8_ram[addr + 1] = (data >> 16) & 0xFF;
        pico8_ram[addr + 2] = (data >> 8) & 0xFF;
        pico8_ram[addr + 3] = data & 0xFF;
        addr += 4;
    }

    return 0;
}

// System functions.

static int pico8_menuitem(lua_State* L)
{
    TO_BE_DONE;
}

static int pico8_extcmd(lua_State* L)
{
    TO_BE_DONE;
}

static int pico8_run(lua_State* L)
{
    TO_BE_DONE;
}

// Table functions.

static int pico8_add(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_settop(L, 3);

    if (lua_isnoneornil(L, 3))
    {
        lua_pushinteger(L, fix32_to_int32(luaL_len(L, 1)) + 1);
    }
    else
    {
        luaL_checktype(L, 3, LUA_TNUMBER);
    }

    lua_pushvalue(L, 2);
    lua_settable(L, 1);

    lua_pushvalue(L, 2);
    return 1;
}

static int pico8_all_iter(lua_State* L)
{
    luaL_checktype(L, lua_upvalueindex(1), LUA_TTABLE);

    lua_settop(L, 0);
    lua_pushvalue(L, lua_upvalueindex(1));
    unsigned int index = fix32_to_uint32(lua_tointeger(L, lua_upvalueindex(2)));

    while (1)
    {
        index++;
        lua_pushinteger(L, index);
        lua_replace(L, lua_upvalueindex(2));

        lua_rawgeti(L, 1, index);
        if (!lua_isnil(L, -1))
        {
            return 1;
        }
        lua_pop(L, 1);

        if (index > lua_rawlen(L, 1))
        {
            return 0;
        }
    }
}

static int pico8_all(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TTABLE);

    lua_pushvalue(L, 1);
    lua_pushinteger(L, 0);
    lua_pushcclosure(L, pico8_all_iter, 2);

    return 1;
}


static int pico8_count(lua_State* L)
{
    TO_BE_DONE;
}

static int pico8_del(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_checkany(L, 2);

    lua_pushnil(L);
    while (lua_next(L, 1) != 0)
    {
        if (lua_compare(L, -1, 2, LUA_OPEQ))
        {
            int index = fix32_to_int32(lua_tointeger(L, -2));
            fix32_t removed_object = lua_tonumber(L, -1);

            // Shift elements down.
            while (true)
            {
                lua_rawgeti(L, 1, index + 1);
                if (lua_isnil(L, -1))
                {
                    lua_pop(L, 1);
                    break;
                }
                lua_rawseti(L, 1, index);
                index++;
            }
            lua_pushnil(L);
            lua_rawseti(L, 1, index);

            lua_pushnumber(L, removed_object);
            return 1;
        }
        lua_pop(L, 1);
    }

    lua_pushnil(L);
    return 1;
}

static int pico8_foreach(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    lua_pushnil(L);
    while (lua_next(L, 1) != 0)
    {
        lua_pushvalue(L, 2);
        lua_pushvalue(L, -2);
        lua_call(L, 1, 0);
        lua_pop(L, 1);
    }

    return 0;
}

static int pico8_ipairs(lua_State* L)
{
    TO_BE_DONE;
}

static int pico8_pairs(lua_State* L)
{
    TO_BE_DONE;
}

static int pico8_pack(lua_State* L)
{
    TO_BE_DONE;
}

static int pico8_unpack(lua_State* L)
{
    TO_BE_DONE;
}

static int pico8_setmetatable(lua_State* L)
{
    TO_BE_DONE;
}

// Debug functions.

static int pico8_crc32(lua_State* L)
{
    uint32_t start = fix32_to_uint32(luaL_checknumber(L, 1));
    uint32_t length = fix32_to_uint32(luaL_checknumber(L, 2));
    uint32_t crc = crc32(pico8_ram, start, length);

    lua_pushunsigned(L, fix32_from_uint32(crc));
    return 1;
}

static int pico8_init_crc32(lua_State* L)
{
    init_crc32();
    return 0;
}

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
        SDL_Log("\r%s", s);
        lua_pop(L, 1);
    }
    return 0;
}

// API Registration.

void init_api(lua_State* L)
{
    // Audio.
    lua_pushcfunction(L, pico8_music);
    lua_setglobal(L, "music");
    lua_pushcfunction(L, pico8_sfx);
    lua_setglobal(L, "sfx");

    // Cart data.
    lua_pushcfunction(L, pico8_cartdata);
    lua_setglobal(L, "cartdata");
    lua_pushcfunction(L, pico8_dget);
    lua_setglobal(L, "dget");
    lua_pushcfunction(L, pico8_dset);
    lua_setglobal(L, "dset");

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

    // Input.
    lua_pushcfunction(L, pico8_btn);
    lua_setglobal(L, "btn");
    lua_pushcfunction(L, pico8_btnp);
    lua_setglobal(L, "btnp");

    // Map.
    lua_pushcfunction(L, pico8_map);
    lua_setglobal(L, "map");
    lua_pushcfunction(L, pico8_mget);
    lua_setglobal(L, "mget");
    lua_pushcfunction(L, pico8_mset);
    lua_setglobal(L, "mset");
    lua_pushcfunction(L, pico8_mapdraw);
    lua_setglobal(L, "mapdraw");

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
    lua_pushcfunction(L, pico8_memcpy);
    lua_setglobal(L, "memcpy");
    lua_pushcfunction(L, pico8_memset);
    lua_setglobal(L, "memset");
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

    // System
    lua_pushcfunction(L, pico8_menuitem);
    lua_setglobal(L, "menuitem");
    lua_pushcfunction(L, pico8_extcmd);
    lua_setglobal(L, "extcmd");
    lua_pushcfunction(L, pico8_run);
    lua_setglobal(L, "run");

    // Tables.
    lua_pushcfunction(L, pico8_add);
    lua_setglobal(L, "add");
    lua_pushcfunction(L, pico8_all);
    lua_setglobal(L, "all");
    lua_pushcfunction(L, pico8_count);
    lua_setglobal(L, "count");
    lua_pushcfunction(L, pico8_del);
    lua_setglobal(L, "del");
    lua_pushcfunction(L, pico8_foreach);
    lua_setglobal(L, "foreach");
    lua_pushcfunction(L, pico8_ipairs);
    lua_setglobal(L, "ipairs");
    lua_pushcfunction(L, pico8_pairs);
    lua_setglobal(L, "pairs");
    lua_pushcfunction(L, pico8_pack);
    lua_setglobal(L, "pack");
    lua_pushcfunction(L, pico8_unpack);
    lua_setglobal(L, "unpack");
    lua_pushcfunction(L, pico8_setmetatable);
    lua_setglobal(L, "setmetatable");

    // Debug.
    lua_pushcfunction(L, pico8_crc32);
    lua_setglobal(L, "crc32");
    lua_pushcfunction(L, pico8_init_crc32);
    lua_setglobal(L, "init_crc32");
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
