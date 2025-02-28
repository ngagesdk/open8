//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2016–2024 Sam Hocevar <sam@hocevar.net>
//
//  This program is free software. It comes without any warranty, to
//  the extent permitted by applicable law. You can redistribute it
//  and/or modify it under the terms of the Do What the Fuck You Want
//  to Public License, Version 2, as published by the WTFPL Task Force.
//  See http://www.wtfpl.net/ for more details.
//

#ifndef FIX32_H
#define FIX32_H

#include <stdint.h> // int32_t, int64_t, …
#include <math.h> // pow()

#ifdef __SYMBIAN32__
static inline long long llabs(long long x) {
    if (x < 0) {
        if (x == (-9223372036854775807LL - 1)) {
            return 9223372036854775807LL;
        }
        return -x;
    }
    return x;
}
#endif

typedef int32_t fix32_t;
#define FIX32_MAX INT32_MAX
#define FIX32_MIN INT32_MIN

static inline fix32_t fix32_value(int16_t i, uint16_t f);
static inline fix32_t fix32_from_string(const char* s, char** endptr);
static inline fix32_t fix32_from_double(double d);
static inline double fix32_to_double(fix32_t x);
static inline fix32_t fix32_from_int(int i);
static inline int fix32_to_int(fix32_t x);
static inline fix32_t fix32_from_int8(int8_t x);
static inline fix32_t fix32_from_uint8(uint8_t x);
static inline fix32_t fix32_from_int16(int16_t x);
static inline fix32_t fix32_from_int32(int32_t x);
static inline fix32_t fix32_from_uint16(uint16_t x);
static inline fix32_t fix32_from_uint32(uint32_t x);
static inline fix32_t fix32_from_int64(int64_t x);
static inline fix32_t fix32_from_uint64(uint64_t x);
static inline int8_t fix32_to_int8(fix32_t x);
static inline uint8_t fix32_to_uint8(fix32_t x);
static inline int16_t fix32_to_int16(fix32_t x);
static inline uint16_t fix32_to_uint16(fix32_t x);
static inline int32_t fix32_to_int32(fix32_t x);
static inline uint32_t fix32_to_uint32(fix32_t x);
static inline int64_t fix32_to_int64(fix32_t x);
static inline uint64_t fix32_to_uint64(fix32_t x);
static inline int fix32_to_bool(fix32_t x);
static inline int fix32_eq(fix32_t a, fix32_t b);
static inline int fix32_ne(fix32_t a, fix32_t b);
static inline int fix32_lt(fix32_t a, fix32_t b);
static inline int fix32_gt(fix32_t a, fix32_t b);
static inline int fix32_le(fix32_t a, fix32_t b);
static inline int fix32_ge(fix32_t a, fix32_t b);
static inline fix32_t fix32_inc(fix32_t *x);
static inline fix32_t fix32_dec(fix32_t *x);
static inline fix32_t fix32_post_inc(fix32_t *x);
static inline fix32_t fix32_post_dec(fix32_t *x);
static inline fix32_t fix32_pos(fix32_t x);
static inline fix32_t fix32_neg(fix32_t x);
static inline fix32_t fix32_not(fix32_t x);
static inline fix32_t fix32_add(fix32_t a, fix32_t b);
static inline fix32_t fix32_sub(fix32_t a, fix32_t b);
static inline fix32_t fix32_and(fix32_t a, fix32_t b);
static inline fix32_t fix32_or(fix32_t a, fix32_t b);
static inline fix32_t fix32_xor(fix32_t a, fix32_t b);
static inline fix32_t fix32_mul(fix32_t a, fix32_t b);
static inline fix32_t fix32_div(fix32_t a, fix32_t b);
static inline fix32_t fix32_mod(fix32_t a, fix32_t b);
static inline fix32_t fix32_shl(fix32_t x, int y);
static inline fix32_t fix32_shr(fix32_t x, int y);
static inline fix32_t fix32_add_assign(fix32_t *a, fix32_t b);
static inline fix32_t fix32_sub_assign(fix32_t *a, fix32_t b);
static inline fix32_t fix32_and_assign(fix32_t *a, fix32_t b);
static inline fix32_t fix32_or_assign(fix32_t *a, fix32_t b);
static inline fix32_t fix32_xor_assign(fix32_t *a, fix32_t b);
static inline fix32_t fix32_mul_assign(fix32_t *a, fix32_t b);
static inline fix32_t fix32_div_assign(fix32_t *a, fix32_t b);
static inline fix32_t fix32_mod_assign(fix32_t *a, fix32_t b);
static inline fix32_t fix32_abs(fix32_t a);
static inline fix32_t fix32_min(fix32_t a, fix32_t b);
static inline fix32_t fix32_max(fix32_t a, fix32_t b);
static inline fix32_t fix32_ceil(fix32_t x);
static inline fix32_t fix32_floor(fix32_t x);
static inline fix32_t fix32_pow(fix32_t x, fix32_t y);
static inline fix32_t fix32_lshr(fix32_t x, int y);
static inline fix32_t fix32_rotl(fix32_t x, int y);
static inline fix32_t fix32_rotr(fix32_t x, int y);

static inline fix32_t fix32_value(int16_t i, uint16_t f)
{
    return (fix32_t)(((uint32_t)i << 16) | ((uint32_t)f & 0xffffu));
}

static inline fix32_t fix32_from_string(const char* s, char** endptr)
{
    return fix32_from_double(strtod(s, endptr));
}

static inline fix32_t fix32_from_double(double d) {
    return (int32_t)(int64_t)(d * 65536.0);
}

static inline double fix32_to_double(fix32_t x) {
    return (double)x * (1.0 / 65536.0);
}

static inline fix32_t fix32_from_int(int i) {
    return (int)(i << 16);
}

static inline int fix32_to_int(fix32_t x) {
    return (int)(x >> 16);
}

// Conversions up to int16_t are safe.
static inline fix32_t fix32_from_int8(int8_t x) {
    return (int32_t)(x << 16);
}

static inline fix32_t fix32_from_uint8(uint8_t x) {
    return (int32_t)(x << 16);
}

static inline fix32_t fix32_from_int16(int16_t x) {
    return (int32_t)(x << 16);
}

static inline fix32_t fix32_from_int32(int32_t x) {
    return (int32_t)(x << 16);
}

static inline fix32_t fix32_from_uint16(uint16_t x) {
    return (int32_t)(x << 16);
}

static inline fix32_t fix32_from_uint32(uint32_t x) {
    return (int32_t)(x << 16);
}

static inline fix32_t fix32_from_int64(int64_t x) {
    return (int32_t)(x << 16);
}

static inline fix32_t fix32_from_uint64(uint64_t x) {
    return (int32_t)(x << 16);
}

static inline int8_t fix32_to_int8(fix32_t x) {
    return (int8_t)(x >> 16);
}

static inline uint8_t fix32_to_uint8(fix32_t x) {
    return (uint8_t)(x >> 16);
}

static inline int16_t fix32_to_int16(fix32_t x) {
    return (int16_t)(x >> 16);
}

static inline uint16_t fix32_to_uint16(fix32_t x) {
    return (uint16_t)(x >> 16);
}

static inline int32_t fix32_to_int32(fix32_t x) {
    return (int32_t)(x >> 16);
}

static inline uint32_t fix32_to_uint32(fix32_t x) {
    return (uint32_t)(x >> 16);
}

static inline int64_t fix32_to_int64(fix32_t x) {
    return (int64_t)(x >> 16);
}

static inline uint64_t fix32_to_uint64(fix32_t x) {
    return (uint64_t)(x >> 16);
}

// Comparisons
static inline int fix32_to_bool(fix32_t x) {
    return x != 0;
}

static inline int fix32_eq(fix32_t a, fix32_t b) {
    return a == b;
}

static inline int fix32_ne(fix32_t a, fix32_t b) {
    return a != b;
}

static inline int fix32_lt(fix32_t a, fix32_t b) {
    return a < b;
}

static inline int fix32_gt(fix32_t a, fix32_t b) {
    return a > b;
}

static inline int fix32_le(fix32_t a, fix32_t b) {
    return a <= b;
}

static inline int fix32_ge(fix32_t a, fix32_t b) {
    return a >= b;
}

static inline fix32_t fix32_inc(fix32_t *x) {
    *x += 0x10000;
    return *x;
}

static inline fix32_t fix32_dec(fix32_t *x) {
    *x -= 0x10000;
    return *x;
}

static inline fix32_t fix32_post_inc(fix32_t *x) {
    fix32_t ret = *x;
    fix32_inc(x);
    return ret;
}

static inline fix32_t fix32_post_dec(fix32_t *x) {
    fix32_t ret = *x;
    fix32_dec(x);
    return ret;
}

// Math operations
static inline fix32_t fix32_pos(fix32_t x) {
    return x;
}

static inline fix32_t fix32_neg(fix32_t x) {
    return -x;
}

static inline fix32_t fix32_not(fix32_t x) {
    return ((~x & 0xFFFF) << 16) | 0xFFFF;
}

static inline fix32_t fix32_add(fix32_t a, fix32_t b) {
    return a + b;
}

static inline fix32_t fix32_sub(fix32_t a, fix32_t b) {
    return a - b;
}

static inline fix32_t fix32_and(fix32_t a, fix32_t b) {
    return a & b;
}

static inline fix32_t fix32_or(fix32_t a, fix32_t b) {
    return a | b;
}

static inline fix32_t fix32_xor(fix32_t a, fix32_t b) {
    return a ^ b;
}

static inline fix32_t fix32_mul(fix32_t a, fix32_t b) {
    return (int32_t)(((int64_t)a * b) >> 16);
}

static inline fix32_t fix32_div(fix32_t a, fix32_t b) {
    if (b == 0x10000) {
        return a; // Special case: division by 1 (0x10000)
    }

    if (b) {
        int64_t result = ((int64_t)a * 0x10000) / b;
        if (llabs(result) <= 0x7FFFFFFF) {
            return (fix32_t)result;
        }
    }

    // Return 0x80000001 (not 0x80000000) for -Inf, mimicking PICO-8 behavior.
    return ((a ^ b) >= 0) ? 0x7FFFFFFF : 0x80000001;
}

static inline fix32_t fix32_mod(fix32_t a, fix32_t b) {
    int32_t result;
    b = fix32_abs(b);
    result = b ? a % b : 0;
    return result >= 0 ? result : result + b;
}

static inline fix32_t fix32_shl(fix32_t x, int y) {
    return y < 0 ? fix32_lshr(x, -y) : (y >= 32 ? 0 : x << y);
}

static inline fix32_t fix32_shr(fix32_t x, int y) {
    return y < 0 ? fix32_shl(x, -y) : (x >> (y >= 31 ? 31 : y));
}

static inline fix32_t fix32_add_assign(fix32_t *a, fix32_t b) {
    *a = fix32_add(*a, b);
    return *a;
}

static inline fix32_t fix32_sub_assign(fix32_t *a, fix32_t b) {
    *a = fix32_sub(*a, b);
    return *a;
}

static inline fix32_t fix32_and_assign(fix32_t *a, fix32_t b) {
    *a = fix32_and(*a, b);
    return *a;
}

static inline fix32_t fix32_or_assign(fix32_t *a, fix32_t b) {
    *a = fix32_or(*a, b);
    return *a;
}

static inline fix32_t fix32_xor_assign(fix32_t *a, fix32_t b) {
    *a = fix32_xor(*a, b);
    return *a;
}

static inline fix32_t fix32_mul_assign(fix32_t *a, fix32_t b) {
    *a = fix32_mul(*a, b);
    return *a;
}

static inline fix32_t fix32_div_assign(fix32_t *a, fix32_t b) {
    *a = fix32_div(*a, b);
    return *a;
}

static inline fix32_t fix32_mod_assign(fix32_t *a, fix32_t b) {
    *a = fix32_mod(*a, b);
    return *a;
}

// Free functions

// PICO-8 0.2.3 changelog: abs(0x8000) should be 0x7fff.ffff
static inline fix32_t fix32_abs(fix32_t a) {
    if (a == 0x80000000) {
        return 0x7fffffff;
    }
    return a >= 0 ? a : a << 1 == 0 ? fix32_not(a) : fix32_neg(a);
}

static inline fix32_t fix32_min(fix32_t a, fix32_t b) {
    return fix32_lt(a, b) ? a : b;
}

static inline fix32_t fix32_max(fix32_t a, fix32_t b) {
    return fix32_gt(a, b) ? a : b;
}

static inline fix32_t fix32_ceil(fix32_t x) {
    return fix32_neg(fix32_floor(fix32_neg(x)));
}

static inline fix32_t fix32_floor(fix32_t x) {
    return x & 0xffff0000;
}

static inline fix32_t fix32_pow(fix32_t x, fix32_t y) {
    return fix32_from_double(pow(fix32_to_double(x), fix32_to_double(y)));
}

static inline fix32_t fix32_lshr(fix32_t x, int y) {
    return y < 0 ? fix32_shl(x, -y) : (y >= 32 ? 0 : (uint32_t)x >> y);
}

static inline fix32_t fix32_rotl(fix32_t x, int y) {
    y &= 0x1f;
    return (x << y) | ((uint32_t)x >> (32 - y));
}

static inline fix32_t fix32_rotr(fix32_t x, int y) {
    y &= 0x1f;
    return ((uint32_t)x >> y) | (x << (32 - y));
}

#endif // FIX32_H
