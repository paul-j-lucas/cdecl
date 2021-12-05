/*
**      cdecl -- C gibberish translator
**      src/strbuf.h
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
#include <stdlib.h>                     /* for free(3) */
#include <string.h>

_GL_INLINE_HEADER_BEGIN
#ifndef STRBUF_INLINE
# define STRBUF_INLINE _GL_INLINE
#endif /* STRBUF_INLINE */

/// @endcond

/**
 * @defgroup strbuf-group String Buffer
 * A type and functions for manipulating a string buffer.
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * %strbuf maintains a C-style string that additionally knows its length and
 * capacity and can grow automatically when appended to.
 */
struct strbuf {
  char   *str;                          ///< String.
  size_t  len;                          ///< Length of \a str.
  size_t  cap;                          ///< Capacity of \a str.
};
typedef struct strbuf strbuf_t;

////////// extern functions ///////////////////////////////////////////////////

/**
 * Concatenates \a format and the `printf`-style arguments onto the end of \a
 * sbuf growing the buffer if necessary.
 *
 * @param sbuf A pointer to the strbuf to concatenate onto.
 * @param format The `printf()` style format string.
 * @param ... The `printf()` arguments.
 *
 * @sa strbuf_catc()
 * @sa strbuf_cats()
 * @sa strbuf_catsn()
 */
PJL_PRINTF_LIKE_FUNC(2)
void strbuf_catf( strbuf_t *sbuf, char const *format, ... );

/**
 * Concatenates \a s_len bytes of \a s onto the end of \a sbuf growing the
 * buffer if necessary.
 *
 * @param sbuf A pointer to the strbuf to concatenate onto.
 * @param s The string to concatenate.
 * @param s_len The number of bytes of \a s to concatenate.
 *
 * @sa strbuf_catc()
 * @sa strbuf_catf()
 * @sa strbuf_cats()
 */
void strbuf_catsn( strbuf_t *sbuf, char const *s, size_t s_len );

/**
 * Concatenates \a c onto the end of \a sbuf growing the buffer if necessary.
 *
 * @param sbuf A pointer to the strbuf to concatenate onto.
 * @param c The character to concatenate.
 *
 * @sa strbuf_catf()
 * @sa strbuf_cats()
 * @sa strbuf_catsn()
 */
STRBUF_INLINE
void strbuf_catc( strbuf_t *sbuf, char c ) {
  strbuf_catsn( sbuf, &c, 1 );
}

/**
 * Concatenates \a s onto the end of \a sbuf growing the buffer if necessary.
 *
 * @param sbuf A pointer to the strbuf to concatenate onto.
 * @param s The string to concatenate.
 *
 * @sa strbuf_catc()
 * @sa strbuf_catf()
 * @sa strbuf_catsn()
 */
STRBUF_INLINE
void strbuf_cats( strbuf_t *sbuf, char const *s ) {
  strbuf_catsn( sbuf, s, strlen( s ) );
}

/**
 * Cleans-up all memory associated with \a sbuf but does _not_ free \a sbuf
 * itself.
 *
 * @param sbuf A pointer to the strbuf to clean up.
 *
 * @sa strbuf_init()
 * @sa strbuf_reset()
 * @sa strbuf_take()
 */
void strbuf_cleanup( strbuf_t *sbuf );

/**
 * Initializes a strbuf.
 *
 * @param sbuf A pointer to the strbuf to initialize.
 *
 * @sa strbuf_cleanup()
 * @sa strbuf_reset()
 * @sa strbuf_take()
 */
STRBUF_INLINE
void strbuf_init( strbuf_t *sbuf ) {
  MEM_ZERO( sbuf );
}

/**
 * Ensures at least \a res_len additional bytes of capacity exist in \a sbuf.
 *
 * @param sbuf A pointer to the strbuf to reserve \a res_len additional bytes
 * for.
 * @param res_len The number of additional bytes to reserve.
 * @return Returns `true` only if a memory reallocation was necessary.
 */
PJL_NOWARN_UNUSED_RESULT
bool strbuf_reserve( strbuf_t *sbuf, size_t res_len );

/**
 * Resets \a sbuf by setting the string to zero length.
 * @param sbuf A pointer to the strbuf to reset.
 *
 * @note This function is more efficient than strbuf_cleanup() when used
 * repeatedly on the same strbuf.
 *
 * @sa strbuf_cleanup()
 * @sa strbuf_init()
 * @sa strbuf_take()
 */
void strbuf_reset( strbuf_t *sbuf );

/**
 * Possibly concatenates \a sep_len bytes of \a sep onto the end of \a sbuf
 * growing the buffer if necessary.
 *
 * @param sbuf A pointer to the strbuf to concatenate onto.
 * @param sep The separator string to concatenate.
 * @param sep_len The number of bytes of \a sep to concatenate.
 * @param sep_flag A pointer to a flag to determine whether \a sep should be
 * concatenated prior to \a s: if `false`, \a sep is _not_ concatenated and it
 * is set to `true`; if `true`, \a sep is concatenated.
 *
 * @sa strbuf_sepc_cats()
 * @sa strbuf_sepc_catsn()
 * @sa strbuf_sepsn_cats()
 * @sa strbuf_sepsn_catsn()
 */
void strbuf_sepsn( strbuf_t *sbuf, char const *sep, size_t sep_len,
                   bool *sep_flag );

/**
 * Possibly concatenates \a sep_len bytes of \a sep followed by \a s_len bytes
 * of \a s onto the end of \a sbuf growing the buffer if necessary.
 *
 * @param sbuf A pointer to the strbuf to concatenate onto.
 * @param sep The separator string to concatenate.
 * @param sep_len The number of bytes of \a sep to concatenate.
 * @param sep_flag A pointer to a flag to determine whether \a sep should be
 * concatenated prior to \a s: if `false`, \a sep is _not_ concatenated and it
 * is set to `true`; if `true`, \a sep is concatenated.
 * @param s The string to concatenate.
 * @param s_len The number of bytes of \a s to concatenate.
 *
 * @sa strbuf_sepsn()
 * @sa strbuf_sepc_cats()
 * @sa strbuf_sepc_catsn()
 * @sa strbuf_sepsn_cats()
 */
void strbuf_sepsn_catsn( strbuf_t *sbuf, char const *sep, size_t sep_len,
                         bool *sep_flag, char const *s, size_t s_len );

/**
 * Possibly concatenates \a sep_len bytes of \a sep followed by \a s onto the
 * end of \a sbuf growing the buffer if necessary.
 *
 * @param sbuf A pointer to the strbuf to concatenate onto.
 * @param sep The separator string to concatenate.
 * @param sep_len The number of bytes of \a sep to concatenate.
 * @param sep_flag A pointer to a flag to determine whether \a sep should be
 * concatenated prior to \a s: if `false`, \a sep is _not_ concatenated and it
 * is set to `true`; if `true`, \a sep is concatenated.
 * @param s The string to concatenate.
 *
 * @sa strbuf_sepc_cats()
 * @sa strbuf_sepc_catsn()
 * @sa strbuf_sepsn()
 * @sa strbuf_sepsn_catsn()
 */
STRBUF_INLINE
void strbuf_sepsn_cats( strbuf_t *sbuf, char const *sep, size_t sep_len,
                        bool *sep_flag, char const *s ) {
  strbuf_sepsn_catsn( sbuf, sep, sep_len, sep_flag, s, strlen( s ) );
}

/**
 * Possibly concatenates \a sep followed by \a s_len bytes of \a s onto the end
 * of \a sbuf growing the buffer if necessary.
 *
 * @param sbuf A pointer to the strbuf to concatenate onto.
 * @param sep The separator character to concatenate.
 * @param sep_flag A pointer to a flag to determine whether \a sep should be
 * concatenated prior to \a s: if `false`, \a sep is _not_ concatenated and it
 * is set to `true`; if `true`, \a sep is concatenated.
 * @param s The string to concatenate.
 * @param s_len The number of bytes of \a s to concatenate.
 *
 * @sa strbuf_sepc_cats()
 * @sa strbuf_sepsn()
 * @sa strbuf_sepsn_cats()
 * @sa strbuf_sepsn_catsn()
 */
STRBUF_INLINE
void strbuf_sepc_catsn( strbuf_t *sbuf, char sep, bool *sep_flag, char const *s,
                        size_t s_len ) {
  strbuf_sepsn_catsn( sbuf, &sep, 1, sep_flag, s, s_len );
}

/**
 * Possibly concatenates \a sep followed by \a s onto the end of \a sbuf
 * growing the buffer if necessary.
 *
 * @param sbuf A pointer to the strbuf to concatenate onto.
 * @param sep The separator character to concatenate.
 * @param sep_flag A pointer to a flag to determine whether \a sep should be
 * concatenated prior to \a s: if `false`, \a sep is _not_ concatenated and it
 * is set to `true`; if `true`, \a sep is concatenated.
 * @param s The string to concatenate.
 *
 * @sa strbuf_sepc_catsn()
 * @sa strbuf_sepsn()
 * @sa strbuf_sepsn_cats()
 * @sa strbuf_sepsn_catsn()
 */
STRBUF_INLINE
void strbuf_sepc_cats( strbuf_t *sbuf, char sep, bool *sep_flag,
                       char const *s ) {
  strbuf_sepsn_catsn( sbuf, &sep, 1, sep_flag, s, strlen( s ) );
}

/**
 * Reinitializes \a sbuf, but returns its string.
 *
 * @param sbuf A pointer to the strbuf to take from.
 * @return Returns said string.  The caller is responsible for freeing it.
 *
 * @sa strbuf_cleanup()
 * @sa strbuf_init()
 * @sa strbuf_reset()
 */
STRBUF_INLINE PJL_WARN_UNUSED_RESULT
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
