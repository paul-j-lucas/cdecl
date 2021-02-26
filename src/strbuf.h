/*
**      cdecl -- C gibberish translator
**      src/strbuf.h
**
**      Copyright (C) 2021  Paul J. Lucas, et al.
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

#ifndef cdecl_strbuf_H
#define cdecl_strbuf_H

/**
 * @file
 * Declares a string buffer type and functions for manipulating it.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>
#include <stddef.h>                     /* for size_t */
#include <stdlib.h>

_GL_INLINE_HEADER_BEGIN
#ifndef C_STRBUF_INLINE
# define C_STRBUF_INLINE _GL_INLINE
#endif /* C_STRBUF_INLINE */

/// @endcond

/**
 * @defgroup util-group Utility Macros & Functions
 * Declares utility constants, macros, and functions.
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * %strbuf maintains a C-style string that additionally knows its length and
 * capacity.
 *
 * @sa strbuf_catc()
 * @sa strbuf_cats()
 * @sa strbuf_catsepc()
 * @sa strbuf_catseps()
 * @sa strbuf_free()
 * @sa strbuf_init()
 * @sa strbuf_take()
 */
struct strbuf {
  char   *str;                          ///< String.
  size_t  str_len;                      ///< Length of \a str.
  size_t  buf_cap;                      ///< Capacity of \a str.
};
typedef struct strbuf strbuf_t;

////////// extern functions ///////////////////////////////////////////////////

/**
 * Concatenates \a s_len bytes of \a s onto the end of \a sbuf growing the
 * buffer if necessary.
 *
 * @param sbuf A pointer to the strbuf to concatenate onto.
 * @param s The string to concatenate.
 * @param s_len The number of bytes of \a s to concatenate; if -1, the length
 * of \a s is calculated and used.
 *
 * @sa strbuf_catc()
 */
void strbuf_cats( strbuf_t *sbuf, char const *s, ssize_t s_len );

/**
 * Possibly concatenates \a sep_len bytes of \a sep followed by \a s_len bytes
 * of \a s onto the enf of \a sbuf growing the buffer if necessary.
 *
 * @param sbuf A pointer to the strbuf to concatenate onto.
 * @param sep The separator string to concatenate.
 * @param sep_len The number of bytes of \a sep to concatenate; if -1, the
 * length of \a sep is calculated and used.
 * @param sep_flag A pointer to a flag to determine whether \a sep should be
 * concatenated prior to \a s: if `false`, \a sep is _not_ concatenated and it
 * is set to `true`; if `true`, \a sep is concatenated.
 * @param s The string to concatenate.
 * @param s_len The number of bytes of \a s to concatenate; if -1, the length
 * of \a s is calculated and used.
 *
 * @sa strbuf_catsepc()
 */
void strbuf_catseps( strbuf_t *sbuf, char const *sep, ssize_t sep_len,
                     bool *sep_flag, char const *s, ssize_t s_len );

/**
 * Possibly concatenates \a sep followed by \a s_len bytes of \a s onto the enf
 * of \a sbuf growing the buffer if necessary.
 *
 * @param sbuf A pointer to the strbuf to concatenate onto.
 * @param sep The separator character to concatenate.
 * @param sep_flag A pointer to a flag to determine whether \a sep should be
 * concatenated prior to \a s: if `false`, \a sep is _not_ concatenated and it
 * is set to `true`; if `true`, \a sep is concatenated.
 * @param s The string to concatenate.
 * @param s_len The number of bytes of \a s to concatenate; if -1, the length
 * of \a s is calculated and used.
 *
 * @sa strbuf_catseps()
 */
C_STRBUF_INLINE
void strbuf_catsepc( strbuf_t *sbuf, char sep, bool *sep_flag, char const *s,
                     ssize_t s_len ) {
  strbuf_catseps( sbuf, &sep, 1, sep_flag, s, s_len );
}

/**
 * Concatenates \a c onto the end of \a sbuf growing the buffer if necessary.
 *
 * @param sbuf A pointer to the strbuf to concatenate onto.
 * @param c The character to concatenate.
 *
 * @sa strbuf_cats()
 */
C_STRBUF_INLINE
void strbuf_catc( strbuf_t *sbuf, char c ) {
  strbuf_cats( sbuf, &c, 1 );
}

/**
 * Initializes a strbuf.
 *
 * @param sbuf A pointer to the strbuf to initialize.
 *
 * @sa strbuf_free()
 * @sa strbuf_take()
 */
C_STRBUF_INLINE
void strbuf_init( strbuf_t *sbuf ) {
  MEM_ZERO( sbuf );
}

/**
 * Frees a strbuf.
 *
 * @param sbuf A pointer to the strbuf to free.
 *
 * @sa strbuf_init()
 * @sa strbuf_take()
 */
C_STRBUF_INLINE
void strbuf_free( strbuf_t *sbuf ) {
  free( sbuf->str );
  strbuf_init( sbuf );
}

/**
 * Reinitializes \a sbuf, but returns its string.
 *
 * @param sbuf A pointer to the strbuf to take from.
 * @return Returns said string.  The caller is responsible for deleting it.
 *
 * @sa strbuf_free()
 * @sa strbuf_init()
 */
C_STRBUF_INLINE PJL_WARN_UNUSED_RESULT
char* strbuf_take( strbuf_t *sbuf ) {
  char *const str = sbuf->str;
  strbuf_init( sbuf );
  return str;
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

_GL_INLINE_HEADER_END

#endif /* cdecl_strbuf_H */
/* vim:set et sw=2 ts=2: */
