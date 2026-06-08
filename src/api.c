/** @file api.c
 *
 *  A portable PICO-8 emulator written in C.
 *
 *  Copyright (c) 2025-2026, Michael Fitzmayer. All rights reserved.
 *  SPDX-License-Identifier: MIT
 *
 **/

#include <SDL3/SDL.h>
#include <stdbool.h>
#include <stdint.h>
#include <wchar.h>

#include "z8lua/lauxlib.h"
#include "z8lua/lua.h"
#include "z8lua/fix32.h"
#include "auxiliary.h"
#include "config.h"
#include "memory.h"
#include "p8scii.h"

#define TO_BE_DONE \
    static bool warning_printed = false; \
    if (!warning_printed) { \
        SDL_Log("%s not yet implemented.", __func__); \
        warning_printed = true; \
    } \
    return 0

static fix32_t seed_lo, seed_hi;

static fix32_t seconds_since_start;

// Frame timing info provided by core.c (frame_start in ms, frame_ms target in ms).
// These are declared in api.h and set inside core.c each frame.
uint32_t pico8_frame_start = 0;
uint32_t pico8_frame_ms = 0;

// Input state: tracks how many consecutive frames each button has been held.
// Index [player][button], players 0-1, buttons 0-5.
static uint8_t btn_held_frames[2][6];

// Auxiliary functions.

// Helper function to apply the camera offset from memory
// Camera values at 0x5f29-0x5f2c are stored as two signed 16-bit integers (pixel values)
// 0x5f29-0x5f2a: camera X (int16_t in pixels)
// 0x5f2b-0x5f2c: camera Y (int16_t in pixels)
static void apply_camera_offset(int* x, int* y)
{
	int cam_x = 0;
	int cam_y = 0;

	// Read camera X as 16-bit signed little-endian integer
	uint16_t cam_x_unsigned = pico8_ram[0x5f29] | ((uint16_t)pico8_ram[0x5f2a] << 8);
	int16_t cam_x_int = (int16_t)cam_x_unsigned;

	// Read camera Y as 16-bit signed little-endian integer
	uint16_t cam_y_unsigned = pico8_ram[0x5f2b] | ((uint16_t)pico8_ram[0x5f2c] << 8);
	int16_t cam_y_int = (int16_t)cam_y_unsigned;

	// Return as integer pixel offsets
	cam_x = cam_x_int;
	cam_y = cam_y_int;

	*x -= cam_x;
	*y -= cam_y;
}

static void pset(int x, int y, int* color)
{
	if (((unsigned)x | (unsigned)y) >= 128)
	{
		return;
	}

	uint16_t pattern = (pico8_ram[0x5f31] << 8) | pico8_ram[0x5f32];
	uint8_t p8_color = color ? (*color & 0x0F) : pico8_ram[0x5f25];

	/* Apply draw palette remapping to the color */
	uint8_t pal_entry = pico8_ram[0x5f00 + p8_color];
	//if (pal_entry & 0x10)
	//{
	//	/* Color is transparent in the draw palette, skip drawing */
	//	return;
	//}
	p8_color = pal_entry & 0x0F;

	uint16_t addr = 0x6000 + (y << 6) + (x >> 1);

	/* Fast path: no fill-pattern active (the default state).
	 * A single OR of both bytes lets the compiler use one branch
	 * with no extra memory reads beyond what the slow path needs. */
	if (pattern == 0)
	{
		if (x & 1)
		{
			pico8_ram[addr] = (pico8_ram[addr] & 0x0F) | (p8_color << 4);
		}
		else
		{
			pico8_ram[addr] = (pico8_ram[addr] & 0xF0) | p8_color;
		}
		return;
	}

	int row_shift = 12 - ((y & 3) << 2);
	uint8_t nibble = (pattern >> row_shift) & 0x0F;

	if (nibble & (1 << (3 - (x & 3))))
	{
		return;
	}

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

	apply_camera_offset((int*)&x, (int*)&y);

	uint16_t addr = 0x6000 + (y << 6) + (x >> 1);
	uint8_t shift = (x & 1) ? 4 : 0;
	uint8_t p8_color = (pico8_ram[addr] >> shift) & 0x0F;
	return p8_color;
}

static void hline(int x0, int x1, int y, int* color)
{
	if ((unsigned)y >= 128) return;
	if (x0 > x1) { int t = x0; x0 = x1; x1 = t; }
	if (x0 < 0) x0 = 0;
	if (x1 > 127) x1 = 127;
	if (x0 > x1) return;

	uint16_t pattern = (pico8_ram[0x5f31] << 8) | pico8_ram[0x5f32];
	uint8_t p8_color = color ? (*color & 0x0F) : pico8_ram[0x5f25];

	if (pattern == 0)
	{
		/* Apply draw palette remapping to the color */
		uint8_t pal_entry = pico8_ram[0x5f00 + p8_color];
		p8_color = pal_entry & 0x0F;

		uint8_t* row = &pico8_ram[0x6000 + (y << 6)];
		uint8_t color_pair = p8_color | (p8_color << 4);
		int bx0 = x0 >> 1;
		int bx1 = x1 >> 1;

		if (x0 & 1)
		{
			row[bx0] = (row[bx0] & 0x0F) | (p8_color << 4);
			bx0++;
		}
		if (!(x1 & 1))
		{
			row[bx1] = (row[bx1] & 0xF0) | p8_color;
			bx1--;
		}
		if (bx0 <= bx1)
		{
			SDL_memset(&row[bx0], color_pair, bx1 - bx0 + 1);
		}
	}
	else
	{
		for (int x = x0; x <= x1; x++)
		{
			pset(x, y, color);
		}
	}
}

static void draw_circle(int cx, int cy, int radius, int* color, bool fill)
{
	/* Fast paths for the small radii (r=0,1,2). */
	if (fill)
	{
		if (radius == 0) { pset(cx, cy, color); return; }
		if (radius == 1)
		{
			hline(cx - 1, cx + 1, cy, color);
			hline(cx, cx, cy - 1, color);
			hline(cx, cx, cy + 1, color);
			return;
		}
		if (radius == 2)
		{
			hline(cx - 1, cx + 1, cy - 2, color);
			hline(cx - 2, cx + 2, cy - 1, color);
			hline(cx - 2, cx + 2, cy, color);
			hline(cx - 2, cx + 2, cy + 1, color);
			hline(cx - 1, cx + 1, cy + 2, color);
			return;
		}
	}

	int x = 0;
	int y = radius;
	int d = 3 - 2 * radius;

	while (x <= y)
	{
		if (fill)
		{
			hline(cx - x, cx + x, cy + y, color);
			hline(cx - x, cx + x, cy - y, color);
			hline(cx - y, cx + y, cy + x, color);
			hline(cx - y, cx + y, cy - x, color);
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
	/* Vertical fast path - big_chest particle lines are always x0==x1. */
	if (x0 == x1)
	{
		if ((unsigned)x0 >= 128) return;
		if (y0 > y1) { int t = y0; y0 = y1; y1 = t; }
		if (y0 < 0) y0 = 0;
		if (y1 > 127) y1 = 127;
		uint8_t p8_color = color ? (*color & 0x0F) : pico8_ram[0x5f25];
		uint16_t pattern = (pico8_ram[0x5f31] << 8) | pico8_ram[0x5f32];
		if (pattern == 0)
		{
			uint8_t mask = (x0 & 1) ? 0x0F : 0xF0;
			uint8_t shift = (x0 & 1) ? 4 : 0;
			uint8_t bits = (uint8_t)(p8_color << shift);
			for (int y = y0; y <= y1; y++)
			{
				uint8_t* p = &pico8_ram[0x6000 + (y << 6) + (x0 >> 1)];
				*p = (*p & mask) | bits;
			}
		}
		else
		{
			for (int y = y0; y <= y1; y++)
				pset(x0, y, color);
		}
		return;
	}

	/* Horizontal fast path. */
	if (y0 == y1)
	{
		hline(x0, x1, y0, color);
		return;
	}

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
			hline(xc - x, xc + x, yc + y, color);
			hline(xc - x, xc + x, yc - y, color);
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
			hline(xc - x, xc + x, yc + y, color);
			hline(xc - x, xc + x, yc - y, color);
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
			hline(x0, x1, y, color);
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

// Debugging functions.

static int pico8_assert(lua_State* L)
{
	TO_BE_DONE;
}

static int pico8_printh(lua_State* L)
{
	TO_BE_DONE;
}

static int pico8_stat(lua_State* L)
{
	uint32_t id = fix32_to_uint32(luaL_checkunsigned(L, 1));

	static bool once = false;
	static uint32_t inv_frame_ms_q16; // Q16.16 reciprocal

	if (!once)
	{
		// 65536 == 1<<16 (Q16.16)
		inv_frame_ms_q16 = (pico8_frame_ms != 0) ? ((1u << 16) / pico8_frame_ms) : 0;
		once = true;
	}

	switch (id)
	{
	case 1:
	{
		// Return CPU usage as fraction [0,1].
		// Usage = elapsed_time_since_frame_start / frame_ms.
		if (pico8_frame_ms == 0)
		{
			lua_pushnumber(L, 0);
		}
		else
		{
			uint32_t now = (uint32_t)SDL_GetTicks();
			uint32_t delta = now - pico8_frame_start;

			// Q16.16 fixed-point usage = (delta / frame_ms)
			uint32_t usage_fp = delta * inv_frame_ms_q16;

			// clamp to 4.0 in Q16.16
			uint32_t max_fp = (4u << 16);
			if (usage_fp > max_fp)
			{
				usage_fp = max_fp;
			}

			// push to Lua without any custom float conversion
			lua_pushnumber(L, (lua_Number)usage_fp * (lua_Number)(1.0 / 65536.0));
		}
		break;
	}
	case 26:
		lua_pushnumber(L, 0);
		break;
	}

	return 1;
}

static int pico8_stop(lua_State* L)
{
	TO_BE_DONE;
}

static int pico8_trace(lua_State* L)
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
	// Read the current camera offset from memory (16-bit signed integers in pixels)
	uint16_t prev_x_unsigned = pico8_ram[0x5f29] | ((uint16_t)pico8_ram[0x5f2a] << 8);
	int16_t prev_x_int = (int16_t)prev_x_unsigned;

	uint16_t prev_y_unsigned = pico8_ram[0x5f2b] | ((uint16_t)pico8_ram[0x5f2c] << 8);
	int16_t prev_y_int = (int16_t)prev_y_unsigned;

	// If arguments are provided, set the new camera offset.
	// Otherwise, reset camera to (0, 0)
	int new_x = 0;
	int new_y = 0;

	if (lua_gettop(L) >= 1)
	{
		fix32_t input = luaL_optnumber(L, 1, 0);
		new_x = fix32_to_int(input);
	}

	if (lua_gettop(L) >= 2)
	{
		fix32_t input = luaL_optnumber(L, 2, 0);
		new_y = fix32_to_int(input);
	}

	// Always store the new camera values (whether they came from arguments or defaults to 0)
	int16_t value_x = (int16_t)new_x;
	pico8_ram[0x5f29] = (uint8_t)(value_x & 0xFF);
	pico8_ram[0x5f2a] = (uint8_t)((value_x >> 8) & 0xFF);

	int16_t value_y = (int16_t)new_y;
	pico8_ram[0x5f2b] = (uint8_t)(value_y & 0xFF);
	pico8_ram[0x5f2c] = (uint8_t)((value_y >> 8) & 0xFF);

	// Return the previous camera offset as an x,y tuple (fix32_t)
	// Convert from integer pixels back to fix32 format  
	lua_pushnumber(L, fix32_from_int((int)prev_x_int));
	lua_pushnumber(L, fix32_from_int((int)prev_y_int));

	return 2;
}

static int pico8_circ(lua_State* L)
{
	int cx = fix32_to_int(luaL_checknumber(L, 1));
	int cy = fix32_to_int(luaL_checknumber(L, 2));

	apply_camera_offset((int*)&cx, (int*)&cy);

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

	apply_camera_offset((int*)&cx, (int*)&cy);

	int radius = fix32_to_int(luaL_optnumber(L, 3, fix32_value(4, 0)));
	int color;

	if (lua_gettop(L) >= 4)
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
	uint8_t n = (uint8_t)(lua_tonumber(L, 1) >> 16);
	uint8_t flags = pico8_ram[0x3000 + n];

	if (lua_gettop(L) >= 2)
	{
		int f = (int)(lua_tonumber(L, 2) >> 16);
		if ((unsigned)f > 7)
		{
			lua_pushboolean(L, 0);
		}
		else
		{
			lua_pushboolean(L, (flags >> f) & 1);
		}
	}
	else
	{
		lua_pushnumber(L, fix32_from_uint8(flags));
	}

	return 1;
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

	apply_camera_offset((int*)&x0, (int*)&y0);
	apply_camera_offset((int*)&x1, (int*)&y1);

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

	apply_camera_offset((int*)&x0, (int*)&y0);
	apply_camera_offset((int*)&x1, (int*)&y1);

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

	apply_camera_offset(&x0, &y0);
	apply_camera_offset(&x1, &y1);

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
	int argc = lua_gettop(L);

	if (argc == 0)
	{
		// Reset draw palette (identity + color 0 transparent) and display palette (identity).
		for (int i = 0; i < 16; i++)
		{
			pico8_ram[0x5f00 + i] = (uint8_t)i | (i == 0 ? 0x10 : 0x00);
			pico8_ram[0x5f10 + i] = (uint8_t)i;
		}
	}
	else if (argc >= 2)
	{
		/* Some carts compute indices that can occasionally be out-of-range
		 * and yield nil when indexing tables; historically PICO-8 did not
		 * crash in these situations. Treat a missing/nil target color as
		 * an identity (no-op) mapping.
		 */
		int c0 = fix32_to_int(luaL_checknumber(L, 1)) & 0x0F;
		int c1;
		if (lua_isnoneornil(L, 2))
		{
			c1 = c0;
		}
		else
		{
			c1 = fix32_to_int(luaL_checknumber(L, 2)) & 0x0F;
		}
		int p = fix32_to_int(luaL_optnumber(L, 3, 0));

		if (p == 1)
		{
			// Display palette remap (0x5f10).
			pico8_ram[0x5f10 + c0] = (pico8_ram[0x5f10 + c0] & 0xF0) | (uint8_t)c1;
		}
		else
		{
			// Draw palette remap (0x5f00), preserve transparency bit.
			pico8_ram[0x5f00 + c0] = (pico8_ram[0x5f00 + c0] & 0xF0) | (uint8_t)c1;
		}
	}

	return 0;
}

static int pico8_palt(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc == 0)
	{
		// Reset: color 0 transparent, all others opaque.
		for (int i = 0; i < 16; i++)
		{
			pico8_ram[0x5f00 + i] = (pico8_ram[0x5f00 + i] & 0xEF) | (i == 0 ? 0x10 : 0x00);
		}
	}
	else if (argc == 1)
	{
		// 16-bit bitmask: bit 0 (LSB) = color 15, bit 15 (MSB) = color 0.
		uint16_t mask = (uint16_t)fix32_to_uint32(luaL_checknumber(L, 1));
		for (int i = 0; i < 16; i++)
		{
			int transparent = (mask >> (15 - i)) & 1;
			pico8_ram[0x5f00 + i] = (pico8_ram[0x5f00 + i] & 0xEF) | (transparent ? 0x10 : 0x00);
		}
	}
	else
	{
		// Two args: col, t.
		int col = fix32_to_int(luaL_checkinteger(L, 1)) & 0x0F;
		bool transparent = lua_toboolean(L, 2);
		pico8_ram[0x5f00 + col] = (pico8_ram[0x5f00 + col] & 0xEF) | (transparent ? 0x10 : 0x00);
	}

	return 0;
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
		else if (text[i] == '\b') // 8, backspace.
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
		int x = cursor_x, y = cursor_y;
		apply_camera_offset((int*)&x, (int*)&y);

		blit_char_to_screen((unsigned char)text[i], x, y, color, &w, &h);
		cursor_x += w + 1;
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

	apply_camera_offset((int*)&x, (int*)&y);

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

	apply_camera_offset(&x0, &y0);
	apply_camera_offset(&x1, &y1);

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

	apply_camera_offset(&x0, &y0);
	apply_camera_offset(&x1, &y1);

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
	int x = fix32_to_int32(luaL_checknumber(L, 1)) & 0x7F;
	int y = fix32_to_int32(luaL_checknumber(L, 2)) & 0x7F;

	uint16_t addr = (uint16_t)(y << 6) + (uint16_t)(x >> 1);
	uint8_t byte = pico8_ram[addr];
	uint8_t color = (x & 1) ? (byte >> 4) : (byte & 0x0F);

	lua_pushunsigned(L, fix32_value(color, 0));
	return 1;
}

static void draw_sprite_n(uint8_t n, int32_t x, int32_t y, uint8_t w, uint8_t h, bool flip_x, bool flip_y)
{
	int32_t width = w * 8;
	int32_t height = h * 8;

	// Compute the visible (clipped) pixel range once, avoiding per-pixel bounds checks.
	int32_t dx_start = (x < 0) ? -x : 0;
	int32_t dy_start = (y < 0) ? -y : 0;
	int32_t dx_end = (x + width > 128) ? 128 - x : width;
	int32_t dy_end = (y + height > 128) ? 128 - y : height;

	if (dx_start >= dx_end || dy_start >= dy_end)
	{
		return;
	}

	uint16_t sprite_x_base = (n & 0xF) << 2;
	uint16_t sprite_y_base = (n >> 4) * 512;

	// Build a local color map from the palette RAM once per sprite call.
	// Bit 4 = transparent flag, bits 0-3 = remapped color.
	uint8_t color_map[16];
	for (int i = 0; i < 16; i++)
	{
		color_map[i] = pico8_ram[0x5f00 + i];
	}

	for (int32_t dy = dy_start; dy < dy_end; dy++)
	{
		int32_t sy = flip_y ? (height - 1 - dy) : dy;
		uint16_t sprite_row_addr = sprite_y_base + ((uint16_t)sy << 6);
		uint16_t screen_row_addr = 0x6000 + ((uint16_t)(y + dy) << 6);

		int32_t dx = dx_start;

		for (; dx < dx_end; dx++)
		{
			int32_t sx = flip_x ? (width - 1 - dx) : dx;
			uint16_t sprite_addr = sprite_row_addr + sprite_x_base + ((uint16_t)sx >> 1);
			uint8_t byte = pico8_ram[sprite_addr];
			uint8_t pal = color_map[(sx & 1) ? ((byte >> 4) & 0x0F) : (byte & 0x0F)];

			if (!(pal & 0x10))
			{
				uint8_t mapped_color = pal & 0x0F;
				int32_t px = x + dx;
				uint16_t screen_addr = screen_row_addr + ((uint16_t)px >> 1);
				uint8_t* screen_byte = &pico8_ram[screen_addr];
				if (px & 1)
				{
					*screen_byte = (*screen_byte & 0x0F) | (mapped_color << 4);
				}
				else
				{
					*screen_byte = (*screen_byte & 0xF0) | mapped_color;
				}
			}
		}
	}
}

static int pico8_spr(lua_State* L)
{
	/* Be defensive: some carts may attempt to call spr(nil) when an object's
	 * spr field is missing. Instead of raising a Lua type error, silently
	 * ignore the call which matches the robustness of the original PICO-8. */
	if (lua_isnoneornil(L, 1) || !lua_isnumber(L, 1))
	{
		return 0;
	}

	uint8_t n = fix32_to_uint8(luaL_checkunsigned(L, 1));
	int32_t x = fix32_to_int32(luaL_optunsigned(L, 2, 0));
	int32_t y = fix32_to_int32(luaL_optunsigned(L, 3, 0));
	uint8_t w = fix32_to_uint8(luaL_optunsigned(L, 4, fix32_value(1, 0)));
	uint8_t h = fix32_to_uint8(luaL_optunsigned(L, 5, fix32_value(1, 0)));
	bool flip_x = lua_toboolean(L, 6);
	bool flip_y = lua_toboolean(L, 7);

	apply_camera_offset((int*)&x, (int*)&y);

	draw_sprite_n(n, x, y, w, h, flip_x, flip_y);

	return 0;
}

static int pico8_sset(lua_State* L)
{
	TO_BE_DONE;
}

static int pico8_sspr(lua_State* L)
{
	int sx = fix32_to_int(luaL_checknumber(L, 1));
	int sy = fix32_to_int(luaL_checknumber(L, 2));
	int sw = fix32_to_int(luaL_checknumber(L, 3));
	int sh = fix32_to_int(luaL_checknumber(L, 4));
	int dx = fix32_to_int(luaL_checknumber(L, 5));
	int dy = fix32_to_int(luaL_checknumber(L, 6));
	int dw = fix32_to_int(luaL_optnumber(L, 7, fix32_from_int(sw)));
	int dh = fix32_to_int(luaL_optnumber(L, 8, fix32_from_int(sh)));
	bool flip_x = lua_toboolean(L, 9);
	bool flip_y = lua_toboolean(L, 10);

	apply_camera_offset((int*)&dx, (int*)&dy);

	if (sw <= 0 || sh <= 0 || dw == 0 || dh == 0)
	{
		return 0;
	}

	// Negative dw/dh: PICO-8 treats them as a flip + draw with abs size.
	if (dw < 0) { flip_x = !flip_x; dw = -dw; dx -= dw; }
	if (dh < 0) { flip_y = !flip_y; dh = -dh; dy -= dh; }

	for (int py = 0; py < dh; py++)
	{
		// Map destination row to source row (nearest neighbour).
		int src_y = flip_y ? (sh - 1 - (py * sh / dh)) : (py * sh / dh);
		src_y += sy;

		for (int px = 0; px < dw; px++)
		{
			// Map destination column to source column (nearest neighbour).
			int src_x = flip_x ? (sw - 1 - (px * sw / dw)) : (px * sw / dw);
			src_x += sx;

			// Read sprite-sheet pixel (4-bit packed, 2 pixels per byte).
			uint16_t sheet_addr = (uint16_t)((src_y & 0x7F) << 6) + (uint16_t)((src_x & 0x7F) >> 1);
			uint8_t  sheet_byte = pico8_ram[sheet_addr];
			uint8_t  color = (src_x & 1) ? (sheet_byte >> 4) : (sheet_byte & 0x0F);

			// Apply draw palette: transparency and remapping.
			uint8_t pal_entry = pico8_ram[0x5f00 + color];
			if (pal_entry & 0x10)
			{
				continue;
			}
			uint8_t mapped_color = pal_entry & 0x0F;

			int32_t screen_x = dx + px;
			int32_t screen_y = dy + py;
			if (screen_x < 0 || screen_x >= 128 || screen_y < 0 || screen_y >= 128)
			{
				continue;
			}

			uint16_t screen_addr = 0x6000 + ((uint16_t)screen_y << 6) + ((uint16_t)screen_x >> 1);
			uint8_t* screen_byte = &pico8_ram[screen_addr];
			if (screen_x & 1)
			{
				*screen_byte = (*screen_byte & 0x0F) | (mapped_color << 4);
			}
			else
			{
				*screen_byte = (*screen_byte & 0xF0) | mapped_color;
			}
		}
	}

	return 0;
}

static int pico8_tline(lua_State* L)
{
	TO_BE_DONE;
}

// Map functions.

static uint8_t map_get(int col, int row)
{
	if ((unsigned)col >= 128 || (unsigned)row >= 64)
	{
		return 0;
	}
	if (row < 32)
	{
		return pico8_ram[0x2000 + row * 128 + col];
	}
	else
	{
		return pico8_ram[0x1000 + (row - 32) * 128 + col];
	}
}

static void map_set(int col, int row, uint8_t sprite)
{
	if ((unsigned)col >= 128 || (unsigned)row >= 64)
	{
		return;
	}
	if (row < 32)
	{
		pico8_ram[0x2000 + row * 128 + col] = sprite;
	}
	else
	{
		pico8_ram[0x1000 + (row - 32) * 128 + col] = sprite;
	}
}

static int pico8_map(lua_State* L)
{
	int celx = fix32_to_int(luaL_optnumber(L, 1, 0));
	int cely = fix32_to_int(luaL_optnumber(L, 2, 0));
	int sx = fix32_to_int(luaL_optnumber(L, 3, 0));
	int sy = fix32_to_int(luaL_optnumber(L, 4, 0));
	int celw = fix32_to_int(luaL_optnumber(L, 5, fix32_value(128, 0)));
	int celh = fix32_to_int(luaL_optnumber(L, 6, fix32_value(64, 0)));
	uint8_t layer = 0;

	apply_camera_offset((int*)&sx, (int*)&sy);

	if (lua_gettop(L) >= 7)
	{
		layer = (uint8_t)fix32_to_uint32(luaL_checknumber(L, 7));
	}

	for (int ty = 0; ty < celh; ty++)
	{
		for (int tx = 0; tx < celw; tx++)
		{
			uint8_t sprite = map_get(celx + tx, cely + ty);

			if (sprite == 0)
			{
				continue;
			}

			if (layer != 0 && (pico8_ram[0x3000 + sprite] & layer) != layer)
			{
				continue;
			}

			draw_sprite_n(sprite, sx + tx * 8, sy + ty * 8, 1, 1, false, false);
		}
	}

	return 0;
}

static int pico8_mget(lua_State* L)
{
	int col = (int)(lua_tonumber(L, 1) >> 16);
	int row = (int)(lua_tonumber(L, 2) >> 16);
	lua_pushnumber(L, fix32_from_uint8(map_get(col, row)));
	return 1;
}

static int pico8_mset(lua_State* L)
{
	int col = fix32_to_int(luaL_checknumber(L, 1));
	int row = fix32_to_int(luaL_checknumber(L, 2));
	uint8_t spr = (uint8_t)fix32_to_uint32(luaL_checknumber(L, 3));
	map_set(col, row, spr);
	return 0;
}

static int pico8_mapdraw(lua_State* L)
{
	return pico8_map(L);
}

// Input functions.

// Parse a button argument which may be either a numeric value or a single-byte
// string containing a special glyph. Returns button index 0..5 on success,
// or -1 on error.
static int parse_button_arg(lua_State* L, int idx)
{
	if (lua_type(L, idx) == LUA_TSTRING)
	{
		const char* str = luaL_checkstring(L, idx);
		uint8_t glyph = (uint8_t)str[0];

		switch (glyph)
		{
		case 131: // Down key.
			return 3;
		case 139: // Left key.
			return 0;
		case 142: // O key.
			return 4;
		case 145: // Right key.
			return 1;
		case 148: // Up key.
			return 2;
		case 151: // X key.
			return 5;
		default:
			return -1;
		}
	}

	/* Fallback: numeric button index (PICO-8 uses fix32 numbers). */
	return fix32_to_int(luaL_checknumber(L, idx));
}

static int pico8_btn(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc == 0)
	{
		lua_pushunsigned(L, fix32_value(pico8_ram[0x5f4c], 0));
		return 1;
	}

	int b = parse_button_arg(L, 1);

	int p = (argc >= 2) ? fix32_to_int(luaL_checknumber(L, 2)) : 0;

	if ((unsigned)p > 1 || b < 0 || (unsigned)b > 5)
	{
		lua_pushboolean(L, 0);
		return 1;
	}

	lua_pushboolean(L, (pico8_ram[0x5f4c + p] & (1 << b)) != 0);
	return 1;
}

// Test-only helper: set_btnp_frames(b, f [, p])
// Sets btn_held_frames[p][b] = f so tests can exercise btnp repeat logic
// without requiring a running emulator loop.
static int pico8_set_btnp_frames(lua_State* L)
{
	int b = fix32_to_int(luaL_checkinteger(L, 1));
	int f = fix32_to_int(luaL_checkinteger(L, 2));
	int p = (lua_gettop(L) >= 3) ? fix32_to_int(luaL_checkinteger(L, 3)) : 0;

	if (p < 0 || p > 1 || b < 0 || b > 5)
	{
		return 0;
	}
	btn_held_frames[p][b] = (uint8_t)f;
	return 0;
}

static int pico8_btnp(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc == 0)
	{
		uint8_t result = 0;
		for (int b = 0; b < 6; b++)
		{
			uint8_t f = btn_held_frames[0][b];
			if (f == 1 || (f > 15 && ((f - 15) % 4 == 0)))
			{
				result |= (1 << b);
			}
		}
		lua_pushunsigned(L, fix32_value(result, 0));
		return 1;
	}

	int b = parse_button_arg(L, 1);

	int p = (argc >= 2) ? fix32_to_int(luaL_checknumber(L, 2)) : 0;
	if (p < 0 || p > 1 || b < 0 || b > 5)
	{
		lua_pushboolean(L, 0);
		return 1;
	}

	uint8_t f = btn_held_frames[p][b];
	lua_pushboolean(L, f == 1 || (f > 15 && ((f - 15) % 4 == 0)));
	return 1;
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

	/* Clamp length so we don't read past RAM end. */
	if (addr >= RAM_SIZE) {
		return 0;
	}
	unsigned int max_len = RAM_SIZE - addr;
	if (len > max_len) {
		len = max_len;
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

	/* Each peek2 entry is 2 bytes. Clamp by available 16-bit words. */
	if (addr >= RAM_SIZE)
	{
		return 0;
	}
	unsigned int max_words = (RAM_SIZE - addr) / 2;
	if (len > max_words)
	{
		len = max_words;
	}

	for (unsigned int i = 0; i < len; i++)
	{
		unsigned int pos = addr + i * 2;
		uint16_t data = (uint16_t)pico8_ram[pos] << 8 | (uint16_t)pico8_ram[pos + 1];
		lua_pushnumber(L, fix32_from_uint16(data));
	}

	return len;
}

static int pico8_peek4(lua_State* L)
{
	uint16_t addr = fix32_to_uint16(luaL_checkunsigned(L, 1));
	unsigned int len = fix32_to_uint32(luaL_optunsigned(L, 2, fix32_value(1, 0)));

	/* Each peek4 entry is 4 bytes. Clamp by available 32-bit words. */
	if (addr >= RAM_SIZE)
	{
		return 0;
	}
	unsigned int max_words = (RAM_SIZE - addr) / 4;
	if (len > max_words)
	{
		len = max_words;
	}

	for (unsigned int i = 0; i < len; i++)
	{
		unsigned int pos = addr + i * 4;
		uint32_t data = (uint32_t)pico8_ram[pos] << 24 | (uint32_t)pico8_ram[pos + 1] << 16 | (uint32_t)pico8_ram[pos + 2] << 8 | (uint32_t)pico8_ram[pos + 3];
		lua_pushnumber(L, fix32_from_uint32(data));
	}
	return len;
}

static int pico8_poke(lua_State* L)
{
	uint16_t addr = fix32_to_uint16(luaL_checkunsigned(L, 1));
	unsigned int num_args = lua_gettop(L);

	for (unsigned int i = 0; i < num_args - 1; i++)
	{
		if (addr + i >= RAM_SIZE)
		{
			break;
		}
		uint8_t data = fix32_to_uint8(luaL_checkinteger(L, 2 + i));
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

// String functions.
static int pico8_sub(lua_State* L)
{
	TO_BE_DONE;
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
	luaL_checkany(L, 2);

	int index;

	if (lua_isnoneornil(L, 3))
	{
		index = (int)lua_rawlen(L, 1) + 1;
	}
	else
	{
		index = fix32_to_int(luaL_checkinteger(L, 3));
	}

	lua_pushvalue(L, 2);
	lua_rawseti(L, 1, index);

	lua_pushvalue(L, 2);
	return 1;
}

static int pico8_all_iter(lua_State* L)
{
	luaL_checktype(L, lua_upvalueindex(1), LUA_TTABLE);

	lua_settop(L, 0);
	lua_pushvalue(L, lua_upvalueindex(1));
	int index = fix32_to_int(lua_tointeger(L, lua_upvalueindex(2)));

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

		if ((size_t)index > lua_rawlen(L, 1))
		{
			return 0;
		}
	}
}

static int pico8_all(lua_State* L)
{
	luaL_checktype(L, 1, LUA_TTABLE);

	/* Make a compact snapshot copy of the array portion of the table.
	 * This makes the iterator resilient to mutations (del/add) of the
	 * original table while iterating, matching PICO-8 semantics.
	 * The copy packs non-nil values into a new dense table. */
	size_t len = lua_rawlen(L, 1);
	lua_newtable(L); /* copy table */
	int copy_index = lua_gettop(L);

	size_t dest = 0;
	for (size_t i = 1; i <= len; i++)
	{
		lua_rawgeti(L, 1, (int)i); /* push original[i] */
		if (!lua_isnil(L, -1))
		{
			dest++;
			lua_rawseti(L, copy_index, (int)dest); /* copy[dest] = value, pops value */
		}
		else
		{
			lua_pop(L, 1);
		}
	}

	/* Push the snapshot and an initial index 0 as upvalues for the iterator. */
	lua_pushvalue(L, copy_index); /* push copy */
	lua_pushinteger(L, 0);
	lua_pushcclosure(L, pico8_all_iter, 2);

	/* remove the temporary copy table left on the stack so only the
	 * iterator closure remains as the return value */
	lua_remove(L, copy_index);

	return 1;
}

static int pico8_count(lua_State* L)
{
	luaL_checktype(L, 1, LUA_TTABLE);
	lua_pushnumber(L, fix32_from_int((int)lua_rawlen(L, 1)));
	return 1;
}

static int pico8_del(lua_State* L)
{
	luaL_checktype(L, 1, LUA_TTABLE);
	luaL_checkany(L, 2);

	int len = (int)lua_rawlen(L, 1);

	for (int i = 1; i <= len; i++)
	{
		lua_rawgeti(L, 1, i); /* push tbl[i] */
		if (lua_rawequal(L, -1, 2))
		{
			/* Shift tbl[i+1..len] down by one. */
			for (int j = i; j < len; j++)
			{
				lua_rawgeti(L, 1, j + 1); /* push tbl[j+1] */
				lua_rawseti(L, 1, j); /* tbl[j] = tbl[j+1], pops */
			}

			/* Nil out the last slot. */
			lua_pushnil(L);
			lua_rawseti(L, 1, len);

			/* The matched value is still on the stack - return it. */
			return 1;
		}
		lua_pop(L, 1); /* discard tbl[i] */
	}

	lua_pushnil(L);
	return 1;
}

static int pico8_foreach(lua_State* L)
{
	luaL_checktype(L, 1, LUA_TTABLE);
	luaL_checktype(L, 2, LUA_TFUNCTION);

	/* Make a compact snapshot copy of the array portion of the table so
	 * that mutations (add/del) performed by the callback do not affect the
	 * iteration order or cause duplicate/skip behaviour. This matches PICO-8
	 * semantics and mirrors the approach used by pico8_all(). */
	size_t len = lua_rawlen(L, 1);
	lua_newtable(L); /* copy */
	int copy_index = lua_gettop(L);

	size_t dest = 0;
	for (size_t i = 1; i <= len; i++)
	{
		lua_rawgeti(L, 1, (int)i); /* push original[i] */
		if (!lua_isnil(L, -1))
		{
			dest++;
			lua_rawseti(L, copy_index, (int)dest); /* copy[dest] = value, pops value */
		}
		else
		{
			lua_pop(L, 1);
		}
	}

	int copy_len = (int)lua_rawlen(L, copy_index);
	for (int i = 1; i <= copy_len; i++)
	{
		lua_pushvalue(L, 2);      /* fn */
		lua_rawgeti(L, copy_index, i); /* item */
		/* item should never be nil in our dense copy, but check defensively */
		if (lua_isnil(L, -1))
		{
			lua_pop(L, 2);
		}
		else
		{
			lua_call(L, 1, 0);
		}
	}

	/* remove the temporary copy table */
	lua_pop(L, 1);

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
	// Draw state defaults.
	for (int i = 0; i < 16; i++)
	{
		pico8_ram[0x5f00 + i] = (uint8_t)i | (i == 0 ? 0x10 : 0x00);
	}
	pico8_ram[0x5f25] = 0x06; // Default draw color: light gray.

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

	// Debugging.
	lua_pushcfunction(L, pico8_assert);
	lua_setglobal(L, "assert");
	lua_pushcfunction(L, pico8_printh);
	lua_setglobal(L, "printh");
	lua_pushcfunction(L, pico8_stat);
	lua_setglobal(L, "stat");
	lua_pushcfunction(L, pico8_stop);
	lua_setglobal(L, "stop");
	lua_pushcfunction(L, pico8_trace);
	lua_setglobal(L, "trace");

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
	lua_pushcfunction(L, pico8_set_btnp_frames);
	lua_setglobal(L, "set_btnp_frames");

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

	// Strings.
	lua_pushcfunction(L, pico8_sub);
	lua_setglobal(L, "sub");

	// System.
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

void update_input(void)
{
	for (int p = 0; p < 2; p++)
	{
		uint8_t state = 0;

		if (p == 0)
		{
			// Keyboard input for player 0.
			int num_keys = 0;
			const bool* keys = SDL_GetKeyboardState(&num_keys);
			if (keys[SDL_SCANCODE_LEFT])
			{
				state |= (1 << 0);
			}
			if (keys[SDL_SCANCODE_RIGHT])
			{
				state |= (1 << 1);
			}
			if (keys[SDL_SCANCODE_UP])
			{
				state |= (1 << 2);
			}
			if (keys[SDL_SCANCODE_DOWN])
			{
				state |= (1 << 3);
			}
#ifdef __SYMBIAN32__
			if (keys[SDL_SCANCODE_5] || keys[SDL_SCANCODE_KP_5])
			{
				state |= (1 << 4);
			}
			if (keys[SDL_SCANCODE_7] || keys[SDL_SCANCODE_KP_7])
			{
				state |= (1 << 5);
			}
#else
			if (keys[SDL_SCANCODE_Z] || keys[SDL_SCANCODE_Y] || keys[SDL_SCANCODE_C])
			{
				state |= (1 << 4);
			}
			if (keys[SDL_SCANCODE_X] || keys[SDL_SCANCODE_V])
			{
				state |= (1 << 5);
			}
#endif
		}

		// Gamepad input: first connected gamepad -> player 0, second -> player 1.
		SDL_Gamepad* gp = SDL_GetGamepadFromPlayerIndex(p);
		if (gp)
		{
			if (SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_DPAD_LEFT))
			{
				state |= (1 << 0);
			}
			if (SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_DPAD_RIGHT))
			{
				state |= (1 << 1);
			}
			if (SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_DPAD_UP))
			{
				state |= (1 << 2);
			}
			if (SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_DPAD_DOWN))
			{
				state |= (1 << 3);
			}
			if (SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_EAST))
			{
				state |= (1 << 4);
			}
			if (SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_SOUTH))
			{
				state |= (1 << 5);
			}
		}

		pico8_ram[0x5f4c + p] = state;

		for (int b = 0; b < 6; b++)
		{
			if (state & (1 << b))
			{
				if (btn_held_frames[p][b] < 255)
				{
					btn_held_frames[p][b]++;
				}
			}
			else
			{
				btn_held_frames[p][b] = 0;
			}
		}
	}
}

void update_time(void)
{
	static Uint64 start_time = 0;
	if (start_time == 0)
	{
		start_time = SDL_GetTicks();
	}

	Uint64 current_time = SDL_GetTicks();
	Uint64 elapsed_time_ticks = current_time - start_time;
	fix32_t elapsed_time = fix32_from_int((Sint32)elapsed_time_ticks) / 1000;
	seconds_since_start = elapsed_time;
}
