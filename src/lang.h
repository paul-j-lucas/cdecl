/*
**      cdecl -- C gibberish translator
**      src/lang.h
**
**      Copyright (C) 2017  Paul J. Lucas, et al.
**
**      This program is free software: you can redistribute it and/or modify
**      it under the terms of the GNU General Public License as published by
**      the Free Software Foundation, either version 3 of the License, or
**      (at your option) any later version.
**
**      This program is distributed in the hope that it will be useful,
**      but WITHOUT ANY WARRANTY; without even the implied warranty of
**      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**      GNU General Public License for more details.
**
**      You should have received a copy of the GNU General Public License
**      along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef cdecl_lang_H
#define cdecl_lang_H

/**
 * @file
 * Declares constants, types, and functions for C/C++ language versions.
 */

///////////////////////////////////////////////////////////////////////////////

// languages supported
#define LANG_NONE     (0u)
#define LANG_ALL      (~LANG_NONE)
#define LANG_MAX(L)   ((LANG_ ## L) | ((LANG_ ## L) - 1u))
#define LANG_MIN(L)   (~((LANG_ ## L) - 1u))

#define LANG_C_MIN    LANG_C_KNR
#define LANG_C_KNR    (1u << 0)
#define LANG_C_89     (1u << 1)
#define LANG_C_95     (1u << 2)
#define LANG_C_99     (1u << 3)
#define LANG_C_11     (1u << 4)
#define LANG_C_MAX    LANG_C_11
#define LANG_C_ALL    LANG_MAX(C_MAX)

#define LANG_CPP_MIN  LANG_CPP_98
#define LANG_CPP_98   (1u << 5)
#define LANG_CPP_03   (1u << 6)
#define LANG_CPP_11   (1u << 7)
#define LANG_CPP_14   (1u << 8)
#define LANG_CPP_MAX  LANG_CPP_14
#define LANG_CPP_ALL  LANG_MIN(CPP_MIN)

/**
 * Bitmask for combination of languages.
 */
typedef unsigned c_lang_t;

/**
 * A mapping between a language name and its corresponding c_lang_t.
 */
struct c_lang_info {
  char const *name;
  c_lang_t    lang;
};
typedef struct c_lang_info c_lang_info_t;

// extern constants
extern c_lang_info_t const C_LANG_INFO[];

////////// extern functions ///////////////////////////////////////////////////

/**
 * Gets the language corresponding to the given string (case insensitive).
 *
 * @param s The null-terminated string to get the language for.
 * @return Returns said language or \c LANG_NONE if \a s doesn't correspond to
 * any supported language.
 */
c_lang_t c_lang_find( char const *s );

/**
 * Gets the printable name of the given language.
 *
 * @param lang The language to get the name of.
 * @return Returns said name.
 */
char const* c_lang_name( c_lang_t lang );

///////////////////////////////////////////////////////////////////////////////

#endif /* cdecl_lang_H */
/* vim:set et sw=2 ts=2: */
