/*
**      cdecl -- C gibberish translator
**      src/c_lang.h
**
**      Copyright (C) 2017-2019  Paul J. Lucas, et al.
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

#ifndef cdecl_c_lang_H
#define cdecl_c_lang_H

/**
 * @file
 * Declares constants, types, and functions for C/C++ language versions.
 */

// local
#include "cdecl.h"                      /* must go first */
#include "typedefs.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>

_GL_INLINE_HEADER_BEGIN
#ifndef CDECL_LANG_INLINE
# define CDECL_LANG_INLINE _GL_INLINE
#endif /* CDECL_LANG_INLINE */

/// @endcond

///////////////////////////////////////////////////////////////////////////////

// languages supported
#define LANG_NONE     (0u)              /**< No languages. */
#define LANG_ALL      (~LANG_NONE)      /**< All supported languages. */

#define LANG_C_OLD    LANG_C_KNR        /**< Oldest supported C language. */
#define LANG_C_KNR    (1u << 0)         /**< K&R (pre-ANSI) C. */
#define LANG_C_89     (1u << 1)         /**< C 89 (first ANSI C). */
#define LANG_C_95     (1u << 2)         /**< C 95. */
#define LANG_C_99     (1u << 3)         /**< C 99. */
#define LANG_C_11     (1u << 4)         /**< C 11. */
#define LANG_C_18     (1u << 5)         /**< C 18. */
#define LANG_C_NEW    LANG_C_18         /**< Newest supported C language. */
#define LANG_C_ALL    LANG_MAX(C_NEW)   /**< All C languages. */

#define LANG_CPP_OLD  LANG_CPP_98       /**< Oldest supported C++ language. */
#define LANG_CPP_98   (1u << 8)         /**< C++ 98. */
#define LANG_CPP_03   (1u << 9)         /**< C++ 03. */
#define LANG_CPP_11   (1u << 10)        /**< C++ 11. */
#define LANG_CPP_14   (1u << 11)        /**< C++ 14. */
#define LANG_CPP_17   (1u << 12)        /**< C++ 17. */
#define LANG_CPP_20   (1u << 13)        /**< C++ 20. */
#define LANG_CPP_NEW  LANG_CPP_20       /**< Newest supported C++ language. */
#define LANG_CPP_ALL  LANG_MIN(CPP_OLD) /**< All C++ languages. */

// bit masks
#define LANG_C_MASK   0x00FF            /**< C languages bitmask. */
#define LANG_CPP_MASK 0xFF00            /**< C++ languages bitmask. */

/** Maximum allowed language, C & C++. */
#define LANG_MAX(L)   ((LANG_ ## L) | ((LANG_ ## L) - 1u))

/** Minimum allowed language, C & C++. */
#define LANG_MIN(L)   (~((LANG_ ## L) - 1u))

/** Minimum allowed language, C only. */
#define LANG_C_MIN(L) (LANG_MIN( C_ ## L ) & LANG_C_MASK)

/**
 * A mapping between a language name and its corresponding `c_lang_id_t`.
 */
struct c_lang {
  char const *name;                     ///< Language name.
  c_lang_id_t lang_id;                  ///< Language bit.
};

/**
 * Array of `c_lang` for all supported languages. The last entry is
 * `{ NULL, LANG_NONE }`.
 */
extern c_lang_t const C_LANG[];

////////// extern functions ///////////////////////////////////////////////////

/**
 * Gets the <code>\ref c_lang_id_t</code> corresponding to the given string
 * (case insensitive).
 *
 * @param name The language name to get the corresponding <code>\ref
 * c_lang_id_t</code> for.
 * @return Returns said language or <code>\ref LANG_NONE</code> if \a name
 * doesn't correspond to any supported language.
 */
c_lang_id_t c_lang_find( char const *name );

/**
 * Gets whether \a lang_id is any version of C++.
 *
 * @param lang_id The bitwise-or of language(s) to check.
 * @return Returns `true` only if \a lang_id is C++.
 */
CDECL_LANG_INLINE bool c_lang_is_cpp( c_lang_id_t lang_id ) {
  return (lang_id & LANG_CPP_MASK) != LANG_NONE;
}

/**
 * Shorthand for the common case of getting whether the current language is any
 * version of C++.
 *
 * @return Returns `true` only if the current language is C++.
 */
#define C_LANG_IS_CPP()           c_lang_is_cpp( opt_lang )

/**
 * Gets the oldest language of the given set.
 *
 * @param lang_id The bitwise-or of language(s).
 * @return Returns said language.
 */
CDECL_LANG_INLINE c_lang_id_t c_lang_oldest( c_lang_id_t lang_id ) {
  return lang_id & ~(lang_id - 1u);
}

/**
 * Gets the printable name of \a lang_id.
 *
 * @param lang_id The <code>\ref c_lang_id_t</code> to get the name of.
 * @return Returns said name.
 */
char const* c_lang_name( c_lang_id_t lang_id );

/**
 * Shorthand for the common case of getting the name of the current language.
 *
 * @return Returns said name.
 */
#define C_LANG_NAME()             c_lang_name( opt_lang )

/**
 * Sets the current language and the corresponding prompt.
 *
 * @param lang_id The language to set.
 */
void c_lang_set( c_lang_id_t lang_id );

///////////////////////////////////////////////////////////////////////////////

_GL_INLINE_HEADER_END

#endif /* cdecl_c_lang_H */
/* vim:set et sw=2 ts=2: */
