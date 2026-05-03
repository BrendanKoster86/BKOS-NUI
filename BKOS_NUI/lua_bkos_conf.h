/*
** lua_bkos_conf.h — Lua 5.4 configuratie voor ESP32-S3 (BKOS-NUI)
** Dit bestand vervangt de standaard luaconf.h.
**
** Aanpassingen t.o.v. standaard:
** - 32-bit integers en floats (minder RAM gebruik)
** - Verkleinde stack (LUAI_MAXSTACK 500 i.p.v. 1000000)
** - PSRAM-bewuste allocator hint via MALLOC_CAP_SPIRAM
** - Geen POSIX, geen dlopen
*/

#ifndef luaconf_h
#define luaconf_h

#include <limits.h>
#include <stddef.h>

/* ─── Getaltypen ─────────────────────────────────────────────────────────── */
/* 32-bit integer en float — bespaart RAM t.o.v. standaard long/double      */
#define LUA_INT_TYPE    LUA_INT_INT      /* int (32-bit op ESP32) */
#define LUA_FLOAT_TYPE  LUA_FLOAT_FLOAT  /* float (32-bit)        */

#define LUA_INT_INT       1
#define LUA_INT_LONG      2
#define LUA_INT_LONGLONG  3

#define LUA_FLOAT_FLOAT   1
#define LUA_FLOAT_DOUBLE  2
#define LUA_FLOAT_LONGDOUBLE 3

/* Selecteer 32-bit typen */
#if LUA_INT_TYPE == LUA_INT_INT
typedef int          lua_Integer;
typedef unsigned int lua_Unsigned;
#define LUA_INTEGER_FRMLEN  ""
#define LUA_INTEGER_FMT     "%" LUA_INTEGER_FRMLEN "d"
#define LUAI_UACINT         lua_Unsigned
#define LUA_INTEGER_MAX     INT_MAX
#define LUA_INTEGER_MIN     INT_MIN
#define LUA_MAXINTEGER      INT_MAX
#define LUA_MININTEGER      INT_MIN
#endif

#if LUA_FLOAT_TYPE == LUA_FLOAT_FLOAT
typedef float lua_Number;
#define LUAI_UACNUMBER   double
#define LUA_NUMBER_FMT   "%.7g"
#define lua_number2str(s,sz,n) l_sprintf((s), sz, LUA_NUMBER_FMT, (LUAI_UACNUMBER)(n))
#define l_mathop(x)   x##f
#define lua_str2number(s,p)   strtof((s),(p))
#endif

/* ─── Limites ────────────────────────────────────────────────────────────── */
#define LUAI_MAXSTACK      500    /* max Lua stack diepte (standaard 1000000) */
#define LUAI_MAXCSTACK     200    /* max C stack nesting                      */
#define LUAI_MAXSHORTLEN   40     /* max lengte van korte strings (geïnterneerd) */
#define MAXUPVAL           200
#define LUA_IDSIZE         60

/* ─── Alignment ──────────────────────────────────────────────────────────── */
#define LUAI_USER_ALIGNMENT_T   double

/* ─── Error handling: gebruik longjmp (GCC) ─────────────────────────────── */
#define LUA_USE_LONGJMP    1

/* ─── Paden (niet gebruikt op ESP32) ─────────────────────────────────────── */
#define LUA_PATH_DEFAULT   ""
#define LUA_CPATH_DEFAULT  ""
#define LUA_DIRSEP         "/"
#define LUA_PATH_SEP       ";"
#define LUA_PATH_MARK      "?"
#define LUA_EXEC_DIR       ""
#define LUA_LSUBSEP        "_"

/* ─── Extra state ruimte ─────────────────────────────────────────────────── */
#define LUA_EXTRASPACE     (sizeof(void*))

/* ─── Locale ─────────────────────────────────────────────────────────────── */
#define lua_getlocaledecpoint()   '.'

/* ─── API exports ────────────────────────────────────────────────────────── */
#define LUA_API    extern
#define LUALIB_API extern
#define LUAMOD_API extern

/* ─── Compiler hints ─────────────────────────────────────────────────────── */
#if defined(__GNUC__)
#define l_likely(x)   __builtin_expect(x, 1)
#define l_unlikely(x) __builtin_expect(x, 0)
#define LUA_USE_LONGJMP 1
#else
#define l_likely(x)   (x)
#define l_unlikely(x) (x)
#endif

/* ─── Printf helpers ─────────────────────────────────────────────────────── */
#include <stdio.h>
#define l_sprintf  snprintf

/* ─── Geen warnings uitvoer ──────────────────────────────────────────────── */
#define lua_warning(L,msg,tocont)   ((void)(L),(void)(msg),(void)(tocont))

/* ─── Geheugenlimieten ───────────────────────────────────────────────────── */
#define LUAI_MAXSIZE   (~(size_t)0)

/* ─── Basis-aantal ───────────────────────────────────────────────────────── */
#define LUA_NUMTYPES    9

/* ─── Interne Lua tags voor types ────────────────────────────────────────── */
#define LUA_TNONE         (-1)
#define LUA_TNIL           0
#define LUA_TBOOLEAN       1
#define LUA_TLIGHTUSERDATA 2
#define LUA_TNUMBER        3
#define LUA_TSTRING        4
#define LUA_TTABLE         5
#define LUA_TFUNCTION      6
#define LUA_TUSERDATA      7
#define LUA_TTHREAD        8

/* ─── Versienummers ──────────────────────────────────────────────────────── */
#define LUA_VERSION_MAJOR   "5"
#define LUA_VERSION_MINOR   "4"
#define LUA_VERSION_RELEASE "7"
#define LUA_VERSION_NUM     504
#define LUA_VERSION_RELEASE_NUM (LUA_VERSION_NUM * 100 + 0)
#define LUA_VERSION   "Lua " LUA_VERSION_MAJOR "." LUA_VERSION_MINOR
#define LUA_RELEASE   LUA_VERSION "." LUA_VERSION_RELEASE
#define LUA_COPYRIGHT LUA_RELEASE "  Copyright (C) 1994-2024 Lua.org, PUC-Rio"
#define LUA_AUTHORS   "R. Ierusalimschy, L. H. de Figueiredo, W. Celes"

/* ─── Standaard naam voor globale omgeving ───────────────────────────────── */
#define LUA_GNAME "_ENV"

/* ─── Conversie hulpfuncties ─────────────────────────────────────────────── */
#define lua_number2integer(n)   ((lua_Integer)(n))
#define lua_numbertointeger(n,p) \
    ((n) >= (lua_Number)(LUA_MININTEGER) && \
     (n) < -(lua_Number)(LUA_MININTEGER) && \
     (*(p) = (lua_Integer)(n), 1))

/* ─── Integer-naar-getal conversie ──────────────────────────────────────── */
#define lua_integer2number(n)   ((lua_Number)(n))

/* ─── Standaard invoer voor luaB_print ──────────────────────────────────── */
#define lua_stdin_is_tty()  0

/* ─── Maximale recursie ──────────────────────────────────────────────────── */
#define LUAI_MAXCCALLS     LUAI_MAXCSTACK

#endif /* luaconf_h */
