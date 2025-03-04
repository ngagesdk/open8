/*
** String formatting for floating-point numbers.
** Copyright (C) 2005-2025 Mike Pall. See Copyright Notice in luajit.h
** Contributed by Peter Cawley.
*/

#include <stdio.h>

#define lj_strfmt_num_c
#define LUA_CORE

#include "lj_obj.h"
#include "lj_buf.h"
#include "lj_str.h"
#include "lj_strfmt.h"

#include "fix32.h"

int lua_number2str(char* s, fix32_t n) {
    int i = sprintf(s, "%1.4f", fix32_to_double(n));
    while (i > 0 && s[i - 1] == '0') {
		s[--i] = '\0';
	}
    if (i > 0 && s[i - 1] == '.') {
		s[--i] = '\0';
	}
    return i;
}

/* Add formatted floating-point number to buffer. */
SBuf *lj_strfmt_putfnum(SBuf *sb, SFormat sf, lua_Number n)
{
  static char buf[STRFMT_MAXBUF_NUM];
  sb->w = buf;
  lua_number2str(sb->w, n);
  return sb;
}

/* -- Conversions to strings ---------------------------------------------- */

/* Convert number to string. */
GCstr * LJ_FASTCALL lj_strfmt_num(lua_State *L, cTValue *o)
{
  char buf[STRFMT_MAXBUF_NUM];

  lua_number2str(buf, o->n);
  MSize len = strnlen(buf, STRFMT_MAXBUF_NUM);

  return lj_str_new(L, buf, len);
}

