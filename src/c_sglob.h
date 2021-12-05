/*
**      cdecl -- C gibberish translator
**      src/c_sglob.h
**
**      Copyright (C) 2021  Paul J. Lucas
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
 *
 * As a special case, the first glob may be `**` that is used to match any
 * scope.
 *
 * @note For C, an sglob is simply a single glob, e.g., `x*`.
 * @sa c_sname_match()
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "types.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>
#include <stddef.h>                     /* for size_t */

_GL_INLINE_HEADER_BEGIN
#ifndef C_SGLOB_INLINE
# define C_SGLOB_INLINE _GL_INLINE
#endif /* C_SGLOB_INLINE */

/// @endcond

/**
 * @defgroup sglob-group Scoped Globs
 * Functions for accessing and manipulating C++ scoped globs.
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
 * Initializes \a sglob.
 *
 * @param sglob The scoped glob to initialize.
 *
 * @note This need not be called for either global or `static` scoped names.
 *
 * @sa c_sglob_cleanup()
 */
C_SGLOB_INLINE
void c_sglob_init( c_sglob_t *sglob ) {
  MEM_ZERO( sglob );
}

/**
 * Parses the glob string \a s into \a sglob.
 *
 * @param s The glob string to parse.  May be NULL.  If not, it _must_ be a
 * valid glob string.
 * @param sglob The scoped glob to parse into.  The caller is responsible for
 * calling c_glob_cleanup().
 *
 * @sa c_sglob_cleanup()
 */
void c_sglob_parse( char const *s, c_sglob_t *sglob );

///////////////////////////////////////////////////////////////////////////////

/** @} */

_GL_INLINE_HEADER_END

#endif /* cdecl_c_sglob_H */
/* vim:set et sw=2 ts=2: */
