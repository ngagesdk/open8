/*
** String scanning.
** Copyright (C) 2005-2025 Mike Pall. See Copyright Notice in luajit.h
*/

#include <math.h>

#define lj_strscan_c
#define LUA_CORE

#include "lj_obj.h"
#include "lj_char.h"
#include "lj_strscan.h"

#include <string.h> // for strncmp
#include <stdlib.h> // for strtod, strtoul
#include "fix32.h"

/* Scan string containing a number. Returns format. Returns value in o. */
StrScanFmt lj_strscan_scan(const uint8_t *p, MSize len, TValue *o, uint32_t opt)
{
    char* endptr;

    // Check for hexadecimal format.
    if (len > 2 && p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
        unsigned long hex_value = strtoul((const char*)p, &endptr, 16);
        if (endptr == (const char*)p + len) {
            o->u64 = hex_value;
            o->n = (double)hex_value;
            return STRSCAN_NUM;
        }
        return STRSCAN_ERROR;
    }

    // Check for binary format.
    if (len > 2 && p[0] == '0' && (p[1] == 'b' || p[1] == 'B')) {
        uint64_t bin_value = 0;
        for (MSize i = 2; i < len; i++) {
            if (p[i] == '0' || p[i] == '1') {
                bin_value = (bin_value << 1) | (p[i] - '0');
            } else {
                return STRSCAN_ERROR;
            }
        }
        o->u64 = bin_value;
        o->n = fix32_from_double((double)bin_value);
        return STRSCAN_NUM;
    }

    // Default to floating point parsing.
    double n = strtod((const char*)p, &endptr);
    if (endptr == (const char*)p) {
        return STRSCAN_ERROR;
    }
    o->u64 = 0;
    o->n = fix32_from_double(n);
    return STRSCAN_NUM;
}

int LJ_FASTCALL lj_strscan_num(GCstr *str, TValue *o)
{
  StrScanFmt fmt = lj_strscan_scan((const uint8_t *)strdata(str), str->len, o,
				   STRSCAN_OPT_TONUM);
  lj_assertX(fmt == STRSCAN_ERROR || fmt == STRSCAN_NUM, "bad scan format");
  return (fmt != STRSCAN_ERROR);
}
