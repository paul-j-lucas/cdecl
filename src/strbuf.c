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
#include <stdarg.h>

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

void strbuf_catf( strbuf_t *sbuf, char const *format, ... ) {
  assert( sbuf != NULL );
  assert( format != NULL );

  va_list args;

  size_t buf_rem = sbuf->cap - sbuf->len;
  va_start( args, format );
  int rv = vsnprintf( sbuf->str + sbuf->len, buf_rem, format, args );
  va_end( args );
  IF_EXIT( rv < 0, EX_IOERR );

  if ( strbuf_reserve( sbuf, (size_t)rv ) ) {
    buf_rem = sbuf->cap - sbuf->len;
    va_start( args, format );
    rv = vsnprintf( sbuf->str + sbuf->len, buf_rem, format, args );
    va_end( args );
    IF_EXIT( rv < 0, EX_IOERR );
  }

  sbuf->len += (size_t)rv;
}

void strbuf_catsn( strbuf_t *sbuf, char const *s, size_t s_len ) {
  assert( s != NULL );
  strbuf_reserve( sbuf, s_len );
  strncpy( sbuf->str + sbuf->len, s, s_len );
  sbuf->len += s_len;
  sbuf->str[ sbuf->len ] = '\0';
}

bool strbuf_reserve( strbuf_t *sbuf, size_t res_len ) {
  assert( sbuf != NULL );
  size_t const buf_rem = sbuf->cap - sbuf->len;
  if ( res_len >= buf_rem ) {
    size_t const new_len = sbuf->len + res_len;
    sbuf->cap = next_pow_2( new_len );
    REALLOC( sbuf->str, char, sbuf->cap );
    return true;
  }
  return false;
}

void strbuf_sepsn_catsn( strbuf_t *sbuf, char const *sep, size_t sep_len,
                         bool *sep_flag, char const *s, size_t s_len ) {
  assert( sep_flag != NULL );
  if ( true_or_set( sep_flag ) )
    strbuf_catsn( sbuf, sep, sep_len );
  strbuf_catsn( sbuf, s, s_len );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
