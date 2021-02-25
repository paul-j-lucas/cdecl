/*
**      cdecl -- C gibberish translator
**      src/strbuf.c
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

/**
 * @file
 * Defines functions for manipulating a string buffer.
 */

// local
#include "pjl_config.h"                 /* must go first */
/// @cond DOXYGEN_IGNORE
#define C_STRBUF_INLINE _GL_EXTERN_INLINE
/// @endcond
#include "strbuf.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <string.h>

/// @endcond

////////// local functions ////////////////////////////////////////////////////

/**
 * Calculates the next power of 2 &gt; \a n.
 *
 * @param n The initial value.
 * @return Returns said power of 2.
 */
static size_t next_pow_2( size_t n ) {
  if ( n == 0 )
    return 1;
  while ( (n & (n - 1)) != 0 )
    n &= n - 1;
  return n << 1;
}

////////// extern functions ///////////////////////////////////////////////////

void strbuf_cats( strbuf_t *sbuf, char const *s, ssize_t s_len_in ) {
  assert( sbuf != NULL );
  assert( s != NULL );

  size_t const s_len = s_len_in == -1 ? strlen( s ) : (size_t)s_len_in;
  size_t const buf_rem = sbuf->buf_cap - sbuf->str_len;

  if ( s_len >= buf_rem ) {
    size_t const new_len = sbuf->str_len + s_len;
    sbuf->buf_cap = next_pow_2( new_len );
    REALLOC( sbuf->str, char, sbuf->buf_cap );
  }
  strncpy( sbuf->str + sbuf->str_len, s, s_len );
  sbuf->str_len += s_len;
  sbuf->str[ sbuf->str_len ] = '\0';
}

void strbuf_catseps( strbuf_t *sbuf, char const *sep, ssize_t sep_len,
                     bool *sep_flag, char const *s, ssize_t s_len ) {
  if ( true_or_set( sep_flag ) )
    strbuf_cats( sbuf, sep, sep_len );
  strbuf_cats( sbuf, s, s_len );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
