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

#include <ctype.h>
#include <string.h>

#define lpico8lib_c
#define LUA_LIB

#include "lua.h"
#include "lauxlib.h"
#include "llimits.h"
#include "lobject.h"
#include "trigtables.h"
#include "fix32.h"

static int pico8_max(lua_State *l) {
    lua_pushnumber(l, fix32_max(lua_tonumber(l, 1), lua_tonumber(l, 2)));
    return 1;
}

static int pico8_min(lua_State *l) {
    lua_pushnumber(l, fix32_min(lua_tonumber(l, 1), lua_tonumber(l, 2)));
    return 1;
}

static int pico8_mid(lua_State *l) {
    fix32_t x = lua_tonumber(l, 1);
    fix32_t y = lua_tonumber(l, 2);
    fix32_t z = lua_tonumber(l, 3);
    lua_pushnumber(l, x > y ? (y > z ? y : fix32_min(x, z)) : (x > z ? x : fix32_min(y, z)));
    return 1;
}

static int pico8_ceil(lua_State *l) {
    lua_pushnumber(l, fix32_ceil(lua_tonumber(l, 1)));
    return 1;
}

static int pico8_flr(lua_State *l) {
    lua_pushnumber(l, fix32_floor(lua_tonumber(l, 1)));
    return 1;
}

static fix32_t sin_helper(fix32_t x) {
    // Reduce angle to 0x0 … 0x0.3fff, with the following PICO-8 rules:
    //  - sin(x) = -sin(x + 0.5)
    //  - sin(x) equals sin(~x) rather than sin(-x)
    //  - the last two bits are rounded
    // We use a lookup table of sin(x)-4x generated by PICO-8 to ensure
    // that we get the exact same results.
    int32_t a = (((x & 0x4000) ? ~x : x) & 0x3fff) + 2;
    fix32_t ret = (a >> 2 << 4) + sintable[a >> 2];
    return (x & 0x8000) ? ret : -ret;
}

static int pico8_cos(lua_State *l) {
    lua_pushnumber(l, sin_helper(fix32_sub(lua_tonumber(l, 1), 0x4000)));
    return 1;
}

static int pico8_sin(lua_State *l) {
    lua_pushnumber(l, sin_helper(lua_tonumber(l, 1)));
    return 1;
}

static int pico8_atan2(lua_State* l) {
    lua_Number x = lua_tonumber(l, 1);
    lua_Number y = lua_tonumber(l, 2);

    fix32_t bits = 0x4000;
    if ((fix32_t)x) {
        // Use std::abs() instead of fix32::abs() to emulate PICO-8’s behaviour
        // with e.g. atan2(0x8000, 0x8000.0001)
        int64_t q = (((int64_t)abs((fix32_t)y) << 16) / (abs((fix32_t)x)));

        if (q > 0x10000) {
            bits -= atantable[((int64_t)1 << 32) / q >> 5];
        }
        else {
            bits = atantable[q >> 5];
        }
    }
    if ((fix32_t)x < 0) { bits = 0x8000 - bits;  }
    if ((fix32_t)y > 0) { bits = -bits & 0xffff; }

    // Emulate PICO-8 bug with atan2(1, 0x8000)
    if ((fix32_t)x && (fix32_t)y == (int32_t)0x80000000) {
        bits = -bits & 0xffff;
    }

    lua_pushnumber(l, (lua_Number)bits);
    return 1;
}

static int pico8_sqrt(lua_State *l) {
    int64_t root = 0, x = ((int64_t)lua_tonumber(l, 1)) << 16;
    if (x > 0) {
        int64_t a;
        for (a = ((int64_t)1) << 46; a; a >>= 2, root >>= 1) {
            if (x >= a + root) {
                x -= a + root;
                root += a << 1;
            }
        }
    }
    lua_pushnumber(l, (int32_t)root);
    return 1;
}

static int pico8_abs(lua_State *l) {
    lua_pushnumber(l, fix32_abs(lua_tonumber(l, 1)));
    return 1;
}

static int pico8_sgn(lua_State *l) {
    int32_t number = fix32_to_int32(lua_tonumber(l, 1));
    fix32_t result;

    if (number >= 0) {
        result = fix32_from_int32(1);
    } else {
        result = fix32_from_int32(-1);
    }

    lua_pushnumber(l, result);
    return 1;
}

static int pico8_band(lua_State *l) {
    lua_pushnumber(l, lua_tonumber(l, 1) & lua_tonumber(l, 2));
    return 1;
}

static int pico8_bor(lua_State *l) {
    lua_pushnumber(l, lua_tonumber(l, 1) | lua_tonumber(l, 2));
    return 1;
}

static int pico8_bxor(lua_State *l) {
    lua_pushnumber(l, lua_tonumber(l, 1) ^ lua_tonumber(l, 2));
    return 1;
}

static int pico8_bnot(lua_State *l) {
    lua_pushnumber(l, fix32_not(lua_tonumber(l, 1)));
    return 1;
}

static int pico8_shl(lua_State *l) {
    lua_pushnumber(l, fix32_shl(lua_tonumber(l, 1), fix32_to_int(lua_tonumber(l, 2))));
    return 1;
}

static int pico8_lshr(lua_State *l) {
    lua_pushnumber(l, fix32_lshr(lua_tonumber(l, 1), fix32_to_int(lua_tonumber(l, 2))));
    return 1;
}

static int pico8_shr(lua_State *l) {
    lua_pushnumber(l, fix32_shr(lua_tonumber(l, 1), fix32_to_int(lua_tonumber(l, 2))));
    return 1;
}

static int pico8_rotl(lua_State *l) {
    lua_pushnumber(l, fix32_rotl(lua_tonumber(l, 1), fix32_to_int(lua_tonumber(l, 2))));
    return 1;
}

static int pico8_rotr(lua_State *l) {
    lua_pushnumber(l, fix32_rotr(lua_tonumber(l, 1), fix32_to_int(lua_tonumber(l, 2))));
    return 1;
}

static int pico8_tostr(lua_State *l) {
    char buffer[20];
    char const *s = buffer;

    uint16_t flags = 0;
    if (lua_isboolean(l, 2)) {
        flags = lua_toboolean(l, 2) ? 1 : 0;
    }
    else if (lua_isnumber(l, 2)) {
        flags = lua_tointeger(l, 2);
    }

    switch (lua_type(l, 1))
    {
        case LUA_TNONE:
            buffer[0] = '\0';
            break;
        case LUA_TNUMBER: {
            fix32_t x = lua_tonumber(l, 1);
            if (flags) {
                uint32_t b = (uint32_t)x;
                if ((flags & 0x3) == 0x3) {
                    sprintf(buffer, "0x%04x%04x", (b >> 16) & 0xffff, b & 0xffff);
                }
                else if ((flags & 0x2) == 0x2) {
                    sprintf(buffer, "%d", b);
                }
                else {
                    sprintf(buffer, "0x%04x.%04x", (b >> 16) & 0xffff, b & 0xffff);
                }
            } else {
                sprintf(buffer, "%f", fix32_to_double(x));
            }
            break;
        }
        case LUA_TSTRING:
            lua_pushvalue(l, 1);
            return 1;
        case LUA_TBOOLEAN: s = lua_toboolean(l, 1) ? "true" : "false"; break;
        case LUA_TTABLE:
            if (luaL_callmeta(l, 1, "__tostring")) {
                luaL_tolstring(l, 1, NULL);
                return 1;
            }
            //[[fallthrough]];
        case LUA_TFUNCTION:
            if (flags) {
                luaL_tolstring(l, 1, NULL);
                return 1;
            }
            //[[fallthrough]];
        default: sprintf(buffer, "[%s]", luaL_typename(l, 1)); break;
    }
    lua_pushstring(l, s);
    return 1;
}

static int pico8_tonum(lua_State *l) {
    fix32_t ret;
    switch (lua_type(l, 1))
    {
        case LUA_TSTRING: {
            char const *s = lua_tostring(l, 1);
            uint16_t flags = lua_gettop(l) >= 2 ? lua_tointeger(l, 2) : 0;
            if (flags & 0x1) {
                char buffer[9];
                uint32_t bits;
                size_t i;
                for (i = 0; s[i] != '\0'; ++i) {
                    buffer[i] = isxdigit(s[i]) ? s[i] : '0';
                }
                bits = strtol(buffer, NULL, 16);
                if (flags & 0x2) bits >>= 16;
                lua_pushnumber(l, bits);
                return 1;
            }
            else if (flags & 0x2) {
                uint32_t bits = strtol(s, NULL, 10);
                bits >>= 16;
                lua_pushnumber(l, bits);
                return 1;
            }
            if (!luaO_str2d(s, strlen(s), &ret)) return 0;
            break;
        }
        case LUA_TNUMBER: ret = lua_tonumber(l, 1); break;
        case LUA_TBOOLEAN: ret = lua_toboolean(l, 1) ? 1 : 0; break;
        default: return 0;
    }
    lua_pushnumber(l, ret);
    return 1;
}

static int pico8_chr(lua_State *l) {
    char s[248];
    size_t i;
    size_t numargs = lua_gettop(l);
    if (numargs > sizeof(s)) numargs = sizeof(s);
    for (i = 0; i < numargs; i++) {
        s[i] = (char)(uint8_t)lua_tonumber(l, i + 1);
    }
    lua_pushlstring(l, s, numargs);
    return 1;
}

static int pico8_ord(lua_State *l) {
    size_t len;
    int i;
    int n = 0;
    int count = 1;
    char const *s = luaL_checklstring(l, 1, &len);
    if (!lua_isnone(l, 3)) {
        if (!lua_isnumber(l, 3)) return 0;
        count = (int)lua_tonumber(l, 3);
    }
    if (!lua_isnone(l, 2)) {
        if (!lua_isnumber(l, 2)) return 0;
        n = (int)lua_tonumber(l, 2) - 1;
    }
    if (n < 0 || (size_t)n >= len || count < 1) {
        return 0;
    }
    if ((size_t)(n + count) > len) {
        count = len - n;
    }
    lua_checkstack(l, count);
    for (i = 0; i < count; ++i) {
        lua_pushnumber(l, (uint8_t)s[n + i]);
    }
    return count;
}

static int pico8_split(lua_State *l) {
    size_t count = 0, hlen;
    char const* haystack;
    int size = 0;
    char needle = ',';
    int convert;
    char const* end;
    char const* parser;

    if (lua_isnil(l, 1)) {
        return 0;
    }
    count = 0, hlen;
    haystack = luaL_checklstring(l, 1, &hlen);
    if (!haystack) {
        return 0;
    }
    lua_newtable(l);
    if (lua_isnumber(l, 2)) {
        size = (int)lua_tonumber(l, 2);
        if (size <= 0)
            size = 1;
    } else if (lua_isstring(l, 2)) {
        needle = *lua_tostring(l, 2);
    }
    convert = lua_isnone(l, 3) || lua_toboolean(l, 3);
    end = haystack + hlen + (!size && needle);
    for (parser = haystack; parser < end; ) {
        fix32_t num;
        char saved;
        char const *next = size ? parser + size
                         : needle ? (char const*)memchr(parser, needle, end - parser) : parser + 1;
        if (!next || next > end) {
            next = haystack + hlen;
        }
        saved = *next;
        *(char *)next = '\0';
        if (convert && luaO_str2d(parser, next - parser, &num)) {
            lua_pushnumber(l, num);
        } else {
            lua_pushlstring(l, parser, next - parser);
        }
        *(char *)next = saved;
        lua_rawseti(l, -2, (int)(++count));
        parser = next + (!size && needle);
    }
    return 1;
}

static const luaL_Reg pico8lib[] = {
  {"max",   pico8_max},
  {"min",   pico8_min},
  {"mid",   pico8_mid},
  {"ceil",  pico8_ceil},
  {"flr",   pico8_flr},
  {"cos",   pico8_cos},
  {"sin",   pico8_sin},
  {"atan2", pico8_atan2},
  {"sqrt",  pico8_sqrt},
  {"abs",   pico8_abs},
  {"sgn",   pico8_sgn},
  {"band",  pico8_band},
  {"bor",   pico8_bor},
  {"bxor",  pico8_bxor},
  {"bnot",  pico8_bnot},
  {"shl",   pico8_shl},
  {"shr",   pico8_shr},
  {"lshr",  pico8_lshr},
  {"rotl",  pico8_rotl},
  {"rotr",  pico8_rotr},
  {"tostr", pico8_tostr},
  {"tonum", pico8_tonum},
  {"chr",   pico8_chr},
  {"ord",   pico8_ord},
  {"split", pico8_split},
  {NULL, NULL}
};

LUAMOD_API int luaopen_pico8 (lua_State *L) {
  lua_pushglobaltable(L);
  luaL_setfuncs(L, pico8lib, 0);
  return 1;
}
