/*
**      cdecl -- C gibberish translator
**      src/strbuf.h
**
**      Copyright (C) 2021-2024  Paul J. Lucas
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

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>
#include <stddef.h>                     /* for size_t */
#include <string.h>                     /* for memset(3) */

_GL_INLINE_HEADER_BEGIN
#ifndef STRBUF_H_INLINE
# define STRBUF_H_INLINE _GL_INLINE
#endif /* STRBUF_H_INLINE */

/// @endcond

/**
 * @defgroup strbuf-group String Buffer
 * A type and functions for manipulating a string buffer.
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * <code>%strbuf</code> maintains a C-style string that additionally knows its
 * length and capacity and can grow automatically when appended to.
 */
struct strbuf {
  char   *str;                          ///< String.
  size_t  len;                          ///< Length of \a str.
  size_t  cap;                          ///< Capacity of \a str.
};
typedef struct strbuf strbuf_t;

////////// extern functions ///////////////////////////////////////////////////

/**
 * Cleans-up all memory associated with \a sbuf but does _not_ free \a sbuf
 * itself.
 *
 * @param sbuf A pointer to the \ref strbuf to clean up.
 *
 * @sa strbuf_init()
 * @sa strbuf_reset()
 * @sa strbuf_take()
 */
void strbuf_cleanup( strbuf_t *sbuf );

/**
 * Initializes a \ref strbuf.
 *
 * @param sbuf A pointer to the \ref strbuf to initialize.
 *
 * @note This need not be called for either global or `static` buffers.
 *
 * @sa strbuf_cleanup()
 * @sa strbuf_reset()
 * @sa strbuf_take()
 */
STRBUF_H_INLINE
void strbuf_init( strbuf_t *sbuf ) {
  *sbuf = (strbuf_t){ 0 };
}

/**
 * Appends \a component onto \a sbuf containing a path ensuring that exactly
 * one `/` separates them.
 *
 * @param sbuf A pointer to the \ref strbuf to append onto.
 * @param component The component to append.
 *
 * @sa strbuf_printf()
 * @sa strbuf_puts()
 * @sa strbuf_putsn()
 */
void strbuf_paths( strbuf_t *sbuf, char const *component );

/**
 * Using \a format, appends the `printf`-style arguments onto the end of \a
 * sbuf growing the buffer if necessary.
 *
 * @param sbuf A pointer to the \ref strbuf to append onto.
 * @param format The `printf()` style format string.
 * @param ... The `printf()` arguments.
 *
 * @sa strbuf_paths()
 * @sa strbuf_putc()
 * @sa strbuf_puts()
 * @sa strbuf_putsn()
 */
PJL_PRINTF_LIKE_FUNC(2)
void strbuf_printf( strbuf_t *sbuf, char const *format, ... );

/**
 * Appends \a s_len bytes of \a s onto the end of \a sbuf growing the buffer if
 * necessary.
 *
 * @param sbuf A pointer to the \ref strbuf to append onto.
 * @param s The string to append.
 * @param s_len The number of bytes of \a s to append.
 *
 * @sa strbuf_paths()
 * @sa strbuf_putc()
 * @sa strbuf_printf()
 * @sa strbuf_puts()
 */
void strbuf_putsn( strbuf_t *sbuf, char const *s, size_t s_len );

/**
 * Appends \a c onto the end of \a sbuf growing the buffer if necessary.
 *
 * @param sbuf A pointer to the \ref strbuf to append onto.
 * @param c The character to append.
 *
 * @sa strbuf_printf()
 * @sa strbuf_puts()
 * @sa strbuf_putsn()
 */
STRBUF_H_INLINE
void strbuf_putc( strbuf_t *sbuf, char c ) {
  strbuf_putsn( sbuf, &c, 1 );
}

/**
 * Appends \a s onto the end of \a sbuf growing the buffer if necessary.
 *
 * @param sbuf A pointer to the \ref strbuf to append onto.
 * @param s The string to append.
 *
 * @sa strbuf_paths()
 * @sa strbuf_putc()
 * @sa strbuf_printf()
 * @sa strbuf_putsn()
 */
STRBUF_H_INLINE
void strbuf_puts( strbuf_t *sbuf, char const *s ) {
  strbuf_putsn( sbuf, s, strlen( s ) );
}

/**
 * Appends \a s, quoted with \a quote and with non-space whitespace,
 * backslashes, and \a quote escaped, onto the end of \a sbuf growing the
 * buffer if necessary.
 *
 * @param sbuf A pointer to the \ref strbuf to append onto.
 * @param quote The quote character to use, either `'` or `"`.
 * @param s The string to put.
 */
void strbuf_puts_quoted( strbuf_t *sbuf, char quote, char const *s );

/**
 * Ensures at least \a res_len additional bytes of capacity exist in \a sbuf.
 *
 * @param sbuf A pointer to the \ref strbuf to reserve \a res_len additional
 * bytes for.
 * @param res_len The number of additional bytes to reserve.
 * @return Returns `true` only if a memory reallocation was necessary.
 */
PJL_DISCARD
bool strbuf_reserve( strbuf_t *sbuf, size_t res_len );

/**
 * Resets \a sbuf by setting the string to zero length.
 *
 * @param sbuf A pointer to the \ref strbuf to reset.
 *
 * @note This function is more efficient than strbuf_cleanup() when used
 * repeatedly on the same \ref strbuf.
 * @note However, strbuf_cleanup() _must_ still be called when \a sbuf is no
 * longer needed.
 *
 * @sa strbuf_cleanup()
 * @sa strbuf_init()
 * @sa strbuf_take()
 */
void strbuf_reset( strbuf_t *sbuf );

/**
 * Possibly appends \a sep_len bytes of \a sep onto the end of \a sbuf growing
 * the buffer if necessary.
 *
 * @param sbuf A pointer to the \ref strbuf to append onto.
 * @param sep The separator string to append.
 * @param sep_len The number of bytes of \a sep to append.
 * @param sep_flag A pointer to a flag to determine whether \a sep should be
 * appended: if `false`, \a sep is _not_ appended and it is set to `true`; if
 * `true`, \a sep is appended.
 *
 * @sa strbuf_sepc_puts()
 * @sa strbuf_sepc_putsn()
 * @sa strbuf_sepsn_puts()
 * @sa strbuf_sepsn_putsn()
 */
void strbuf_sepsn( strbuf_t *sbuf, char const *sep, size_t sep_len,
                   bool *sep_flag );

/**
 * Possibly appends \a sep_len bytes of \a sep followed by \a s_len bytes of \a
 * s onto the end of \a sbuf growing the buffer if necessary.
 *
 * @param sbuf A pointer to the \ref strbuf to append onto.
 * @param sep The separator string to append.
 * @param sep_len The number of bytes of \a sep to append.
 * @param sep_flag A pointer to a flag to determine whether \a sep should be
 * appended before to \a s: if `false`, \a sep is _not_ appended and it is set
 * to `true`; if `true`, \a sep is appended.
 * @param s The string to append.
 * @param s_len The number of bytes of \a s to append.
 *
 * @sa strbuf_sepsn()
 * @sa strbuf_sepc_puts()
 * @sa strbuf_sepc_putsn()
 * @sa strbuf_sepsn_puts()
 */
void strbuf_sepsn_putsn( strbuf_t *sbuf, char const *sep, size_t sep_len,
                         bool *sep_flag, char const *s, size_t s_len );

/**
 * Possibly appends \a sep_len bytes of \a sep followed by \a s onto the end of
 * \a sbuf growing the buffer if necessary.
 *
 * @param sbuf A pointer to the \ref strbuf to append onto.
 * @param sep The separator string to append.
 * @param sep_len The number of bytes of \a sep to append.
 * @param sep_flag A pointer to a flag to determine whether \a sep should be
 * appended prior to \a s: if `false`, \a sep is _not_ appended and it is set
 * to `true`; if `true`, \a sep is appended.
 * @param s The string to append.
 *
 * @sa strbuf_sepc_puts()
 * @sa strbuf_sepc_putsn()
 * @sa strbuf_sepsn()
 * @sa strbuf_sepsn_putsn()
 */
STRBUF_H_INLINE
void strbuf_sepsn_puts( strbuf_t *sbuf, char const *sep, size_t sep_len,
                        bool *sep_flag, char const *s ) {
  strbuf_sepsn_putsn( sbuf, sep, sep_len, sep_flag, s, strlen( s ) );
}

/**
 * Possibly appends \a sep followed by \a s_len bytes of \a s onto the end of
 * \a sbuf growing the buffer if necessary.
 *
 * @param sbuf A pointer to the \ref strbuf to append onto.
 * @param sep The separator character to append.
 * @param sep_flag A pointer to a flag to determine whether \a sep should be
 * appended prior to \a s: if `false`, \a sep is _not_ appended and it is set
 * to `true`; if `true`, \a sep is appended.
 * @param s The string to append.
 * @param s_len The number of bytes of \a s to append.
 *
 * @sa strbuf_sepc_puts()
 * @sa strbuf_sepsn()
 * @sa strbuf_sepsn_puts()
 * @sa strbuf_sepsn_putsn()
 */
STRBUF_H_INLINE
void strbuf_sepc_putsn( strbuf_t *sbuf, char sep, bool *sep_flag, char const *s,
                        size_t s_len ) {
  strbuf_sepsn_putsn( sbuf, &sep, 1, sep_flag, s, s_len );
}

/**
 * Possibly appends \a sep followed by \a s onto the end of \a sbuf growing the
 * buffer if necessary.
 *
 * @param sbuf A pointer to the \ref strbuf to append onto.
 * @param sep The separator character to append.
 * @param sep_flag A pointer to a flag to determine whether \a sep should be
 * appended prior to \a s: if `false`, \a sep is _not_ appended and it is set
 * to `true`; if `true`, \a sep is appended.
 * @param s The string to append.
 *
 * @sa strbuf_sepc_putsn()
 * @sa strbuf_sepsn()
 * @sa strbuf_sepsn_puts()
 * @sa strbuf_sepsn_putsn()
 */
STRBUF_H_INLINE
void strbuf_sepc_puts( strbuf_t *sbuf, char sep, bool *sep_flag,
                       char const *s ) {
  strbuf_sepsn_putsn( sbuf, &sep, 1, sep_flag, s, strlen( s ) );
}

/**
 * Reinitializes \a sbuf, but returns its string.
 *
 * @param sbuf A pointer to the \ref strbuf to take from.
 * @return Returns said string.  The caller is responsible for freeing it.
 *
 * @sa strbuf_cleanup()
 * @sa strbuf_init()
 * @sa strbuf_reset()
 */
NODISCARD STRBUF_H_INLINE
char* strbuf_take( strbuf_t *sbuf ) {
  char *const rv_str = sbuf->str;
  strbuf_init( sbuf );
  return rv_str;
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

_GL_INLINE_HEADER_END

#endif /* cdecl_strbuf_H */
/* vim:set et sw=2 ts=2: */
