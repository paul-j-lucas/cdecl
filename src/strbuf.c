/*
**      cdecl -- C gibberish translator
**      src/strbuf.c
**
**      Copyright (C) 2021-2023  Paul J. Lucas
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
#define STRBUF_H_INLINE _GL_EXTERN_INLINE
/// @endcond
#include "strbuf.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>                     /* for free(3) */

/// @endcond

/**
 * @addtogroup strbuf-group
 * @{
 */

////////// local functions ////////////////////////////////////////////////////

/**
 * Calculates the next power of 2 &gt; \a n.
 *
 * @param n The initial value.
 * @return Returns said power of 2.
 */
NODISCARD
static size_t next_pow_2( size_t n ) {
  if ( n == 0 )
    return 1;
  while ( (n & (n - 1)) != 0 )
    n &= n - 1;
  return n << 1;
}

////////// extern functions ///////////////////////////////////////////////////

void strbuf_cleanup( strbuf_t *sbuf ) {
  assert( sbuf != NULL );
  free( sbuf->str );
  strbuf_init( sbuf );
}

void strbuf_paths( strbuf_t *sbuf, char const *component ) {
  assert( sbuf != NULL );
  assert( component != NULL );

  size_t comp_len = strlen( component );
  if ( comp_len == 0 )
    return;

  if ( sbuf->len > 0 ) {
    if ( sbuf->str[ sbuf->len - 1 ] == '/' ) {
      if ( component[0] == '/' ) {
        ++component;
        --comp_len;
      }
    } else {
      if ( component[0] != '/' ) {
        strbuf_reserve( sbuf, comp_len + 1 );
        strbuf_putc( sbuf, '/' );
      }
    }
  }

  strbuf_putsn( sbuf, component, comp_len );
}

void strbuf_printf( strbuf_t *sbuf, char const *format, ... ) {
  assert( sbuf != NULL );
  assert( format != NULL );

  char *buf;
  size_t buf_rem;

  if ( sbuf->str == NULL ) {
    //
    // Avoid the undefined behavior of adding an offset to a null pointer.  We
    // have to check for this only in this function since we don't initially
    // call strbuf_reserve() like in every other function.
    //
    buf = NULL;
    buf_rem = 0;
  } else {
    buf = sbuf->str + sbuf->len;
    buf_rem = sbuf->cap - sbuf->len;
  }

  //
  // Attempt to concatenate onto the existing buffer: vsnprintf() returns the
  // number of characters that _would_ have been printed if the buffer were
  // unlimited.
  //
  va_list args;
  va_start( args, format );
  int raw_len = vsnprintf( buf, buf_rem, format, args );
  va_end( args );
  PERROR_EXIT_IF( raw_len < 0, EX_IOERR );

  //
  // Then reserve that number of characters: if strbuf_reserve() returns false,
  // it means the buffer was already big enough and so all the characters were
  // put into it by vsnprintf() which means we're done; otherwise, it means the
  // buffer wasn't big enough so all the characters didn't fit, but the buffer
  // was grown so they _will_ fit if we vsnprintf() again.
  //
  size_t const args_len = STATIC_CAST( size_t, raw_len );
  if ( strbuf_reserve( sbuf, args_len ) ) {
    buf = sbuf->str + sbuf->len;
    buf_rem = sbuf->cap - sbuf->len;
    va_start( args, format );
    raw_len = vsnprintf( buf, buf_rem, format, args );
    va_end( args );
    PERROR_EXIT_IF( raw_len < 0, EX_IOERR );
  }

  sbuf->len += args_len;
}

void strbuf_putsn( strbuf_t *sbuf, char const *s, size_t s_len ) {
  assert( s != NULL );
  strbuf_reserve( sbuf, s_len );

  // Use memcpy() to eliminate "'strncpy' output truncated before terminating
  // nul copying 1 byte from a string of the same length" warning.
  memcpy( sbuf->str + sbuf->len, s, s_len );

  sbuf->len += s_len;
  sbuf->str[ sbuf->len ] = '\0';
}

bool strbuf_reserve( strbuf_t *sbuf, size_t res_len ) {
  assert( sbuf != NULL );
  size_t const buf_rem = sbuf->cap - sbuf->len;
  if ( res_len < buf_rem )
    return false;
  //
  // We don't need to add +1 for the terminating '\0' since next_pow_2(n) is
  // guaranteed to be at least n+1.
  //
  sbuf->cap = next_pow_2( sbuf->len + res_len );
  REALLOC( sbuf->str, char, sbuf->cap );
  return true;
}

void strbuf_reset( strbuf_t *sbuf ) {
  assert( sbuf != NULL );
  if ( sbuf->str != NULL ) {
    sbuf->str[0] = '\0';
    sbuf->len = 0;
  }
}

void strbuf_sepsn( strbuf_t *sbuf, char const *sep, size_t sep_len,
                   bool *sep_flag ) {
  assert( sep_flag != NULL );
  if ( true_or_set( sep_flag ) )
    strbuf_putsn( sbuf, sep, sep_len );
}

void strbuf_sepsn_putsn( strbuf_t *sbuf, char const *sep, size_t sep_len,
                         bool *sep_flag, char const *s, size_t s_len ) {
  strbuf_sepsn( sbuf, sep, sep_len, sep_flag );
  strbuf_putsn( sbuf, s, s_len );
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
