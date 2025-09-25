/*
**      cdecl -- C gibberish translator
**      src/c_sglob.h
**
**      Copyright (C) 2021-2025  Paul J. Lucas
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

#ifndef cdecl_c_sglob_H
#define cdecl_c_sglob_H

/**
 * @file
 * Declares functions for dealing with "sglob" (C++ scoped name glob) objects,
 * e.g., `S::T::*`, that are used to match snames (C++ scoped names).
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "types.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>
#include <stddef.h>                     /* for size_t */

/// @endcond

/**
 * @defgroup sglob-group Scoped Globs
 * Functions for dealing with "sglob" (C++ scoped name glob) objects, e.g.,
 * `S::T::*`, that are used to match snames (C++ scoped names).
 *
 * As a special case, the first glob may be `**` that is used to match any
 * scope.
 *
 * @note For C, an sglob is simply a single glob, e.g., `x*`.
 *
 * @sa \ref sname-group
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * C++ scoped name glob, e.g., `S::T::x*`.
 */
struct c_sglob {
  size_t  count;                        ///< Number of scopes.
  char  **pattern;                      ///< Array[count] of glob patterns.
  bool    match_in_any_scope;           ///< Match in any scope?
};

////////// extern functions ///////////////////////////////////////////////////

/**
 * Cleans-up all memory associated with \a sglob but does _not_ free \a sglob
 * itself.
 *
 * @param sglob The scoped glob to clean up.  If NULL, does nothing.
 *
 * @sa c_sglob_init()
 */
void c_sglob_cleanup( c_sglob_t *sglob );

/**
 * Gets whether \a sglob is empty.
 *
 * @param sglob The scoped glob to check.
 * @return Returns `true` only if \a sglob is empty.
 */
NODISCARD
inline bool c_sglob_empty( c_sglob_t const *sglob ) {
  return sglob->count == 0;
}

/**
 * Initializes \a sglob.
 *
 * @param sglob The scoped glob to initialize.
 *
 * @note This need not be called for either global or `static` scoped globs.
 *
 * @sa c_sglob_cleanup()
 */
inline void c_sglob_init( c_sglob_t *sglob ) {
  *sglob = (c_sglob_t){ 0 };
}

/**
 * Parses the glob string \a s into \a sglob.
 *
 * @param s The glob string to parse.  If NULL, empty, or all whitespace, does
 * nothing; if not, it _must_ be a valid glob string.
 * @param rv_sglob The scoped glob to parse into.  It _must_ be in an
 * initialized state. The caller is responsible for calling c_sglob_cleanup().
 *
 * @warning This function assumes \a s, if non-NULL, non-empty, and non-all-
 * whitespace, is a valid glob string returned by the lexer.  This function
 * does _not_ do a full syntax-checking parse so an invalid glob string may not
 * be detected.
 *
 * @sa c_sglob_cleanup()
 */
void c_sglob_parse( char const *s, c_sglob_t *rv_sglob );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_c_sglob_H */
/* vim:set et sw=2 ts=2: */
